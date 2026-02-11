/*
 * ATmega8 – 1.2× frekvenční násobič:
 *  - Měření vstupu na PD6 (ICP1, Timer1 input capture).
 *  - Výstup na PD7 (OC2, Timer2 CTC, toggle).
 *
 * Autor: M365 Copilot (2026)
 */

#ifndef F_CPU
#define F_CPU 16000000UL   // nastav podle tvého MCU (8 MHz nebo 16 MHz typicky)
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdbool.h>

/* --- Parametry násobení --- 
 * Chceme f_out = (NUM / DEN) * f_in. Pro +20 % = 6/5.
 * (Obecně tak můžeš snadno změnit násobek.)
 */
#define MULT_NUM   6u  // čitatel   (1.2 = 6)
#define MULT_DEN   5u  // jmenovatel (1.2 = 5)

/* --- Timer1 (měření vstupní periody) --- */
#define T1_PRESC       8u     // prescaler Timer1 (CS11=1) => 1 tik = presc/F_CPU
#define T1_CS_BITS     (1<<CS11)

#define UART_BAUD     115200UL

/* --- Timer2 (generování výstupu) --- 
 * Timer2 má prescalery: 1,8,32,64,128,256,1024.
 * Budeme vybírat dynamicky tak, aby OCR2 vyšlo ~10..250.
 */
typedef struct {
    uint16_t presc;
    uint8_t  cs_bits;  // CS22..CS20 bit pattern
} t2_presc_t;

static const t2_presc_t T2_PRESCALERS[] = {
    {   1,  (1<<CS20)                            }, // 001
    {   8,  (1<<CS21)                            }, // 010
    {  32,  (1<<CS21) | (1<<CS20)               }, // 011
    {  64,  (1<<CS22)                            }, // 100
    { 128,  (1<<CS22) | (1<<CS20)               }, // 101
    { 256,  (1<<CS22) | (1<<CS21)               }, // 110
    {1024,  (1<<CS22) | (1<<CS21) | (1<<CS20)   }  // 111
};
#define T2_PRESC_COUNT  (sizeof(T2_PRESCALERS)/sizeof(T2_PRESCALERS[0]))

/* Doporučené limity pro OCR2, aby měl výstup rozumnou kvalitu */
#define OCR2_MIN  10u
#define OCR2_MAX  250u

/* --- Stavové proměnné měření --- */
static volatile uint32_t t1_ovf = 0;         // 32bit čas – počitadlo overflowů
static volatile uint32_t last_capture = 0;   // poslední timestamp
static volatile uint32_t period_ticks = 0;   // naměřená perioda v tikech Timer1 (32bit)
static volatile bool     period_ready = false;

/* Jednoduché vyhlazení (EMA) – snižuje jitter měření */
static uint32_t ema_period_ticks = 0;        // exponenciální průměr periody
#define EMA_SHIFT 3                          // 1/8 nová hodnota (větší = pomalejší reakce)

/* --- Pomocné: bezpečný rozdíl 32bit časů --- */
static inline uint32_t ticks_diff(uint32_t now, uint32_t prev) {
    return (now - prev); // 32bit wrap je v unsigned dobře definovaný
}

static inline void uart_tx(uint8_t b){ while(!(UCSRA&(1<<UDRE))){} UDR=b; }
static void uart_init(void){
#if UART_USE_U2X
    UCSRA = (1<<U2X);
    uint16_t ubrr=(uint16_t)(F_CPU/(8UL*UART_BAUD)-1UL);
#else
    UCSRA = 0;
    uint16_t ubrr=(uint16_t)(F_CPU/(16UL*UART_BAUD)-1UL);
#endif
    UBRRH=(uint8_t)(ubrr>>8); UBRRL=(uint8_t)ubrr;
    UCSRB=(1<<TXEN)|(1<<RXEN)|(1<<RXCIE);               // TX, RX, RX IRQ
    UCSRC=(1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0);             // 8N1
}

/* --- ISR: overflow Timer1 --- */
ISR(TIMER1_OVF_vect) {
    t1_ovf++;
    //uart_tx(0x42);
}

/* --- ISR: input capture na Timer1 (PD6/ICP1, hrana nahoru) --- */
ISR(TIMER1_CAPT_vect) {
    uart_tx(0x43);
    // slož 32bit timestamp: high odvozeno z overflow counteru, low = ICR1
    uint32_t now = (t1_ovf << 16) | (uint16_t)ICR1;

    uint32_t dt = ticks_diff(now, last_capture);
    last_capture = now;

    if (dt > 0) {
        // EMA vyhlazení: ema += (dt - ema) / 2^EMA_SHIFT
        if (ema_period_ticks == 0) {
            ema_period_ticks = dt; // první vzorek
        } else {
            ema_period_ticks += (int32_t)(dt - ema_period_ticks) >> EMA_SHIFT;
        }
        period_ticks = ema_period_ticks;
        period_ready = true;
    }
}

/* --- Inicializace Timer1 pro měření periody na ICP1 --- */
static void timer1_input_init(void) {
    // PD6 jako vstup (ICP1)
    DDRD &= ~(1<<DDD6);

    // Timer1: prescaler 8, capture na rising edge, noise canceler ON
    TCCR1A = 0;
    TCCR1B = (1<<ICNC1) | (1<<ICES1) | T1_CS_BITS;
    

    // Povol přerušení: capture + overflow
    TIMSK |= (1<<TICIE1) | (1<<TOIE1);

    // Nuluji čítače
    TCNT1 = 0;
    t1_ovf = 0;
    last_capture = 0;
}

/* --- Inicializace Timer2 pro výstup na OC2 (PD7) --- */
static void timer2_output_init(void) {
    // PD7 jako výstup (OC2)
    DDRD |= (1<<DDD7);

    // CTC režim (WGM21=1), toggle OC2 na compare (COM20=1)
    // Prescaler nastavíme až při výpočtu (CS22..CS20).
    TCCR2 = (1<<WGM21) | (1<<COM20);
    OCR2  = 127; // nějaká startovní hodnota
}

/* --- Přepočet perioda->OCR2 a volba prescaleru pro Timer2 ---
 * Vychází z rovnice:
 *    f_out = (MULT_NUM/MULT_DEN) * f_in
 *    f_out = F_CPU / (2 * presc2 * (1 + OCR2))
 *    f_in  = F_CPU / (presc1 * period_ticks)
 * =>
 *    OCR2 ≈ (presc1 * period_ticks * MULT_DEN) / (2 * presc2 * MULT_NUM) - 1
 */
static void t2_set_from_period(uint32_t period_t1_ticks) {
    if (period_t1_ticks == 0) return;

    uint8_t chosen_cs = 0;
    uint8_t chosen_ocr = 127;
    bool ok = false;

    for (uint8_t i = 0; i < T2_PRESC_COUNT; ++i) {
        uint32_t presc2 = T2_PRESCALERS[i].presc;

        // výpočet v 32 bitech: ((T1_PRESC * period * MULT_DEN) / (2 * presc2 * MULT_NUM)) - 1
        // abychom se vyhnuli float, počítáme jako: num = T1_PRESC * period * MULT_DEN
        // denom = 2 * presc2 * MULT_NUM
        uint64_t num   = (uint64_t)T1_PRESC * (uint64_t)period_t1_ticks * (uint64_t)MULT_DEN;
        uint32_t denom = (uint32_t)(2u * presc2 * (uint32_t)MULT_NUM);

        uint32_t ocr_calc = (uint32_t)(num / denom);
        if (ocr_calc > 0) { ocr_calc -= 1; }

        if (ocr_calc >= OCR2_MIN && ocr_calc <= OCR2_MAX) {
            chosen_cs  = T2_PRESCALERS[i].cs_bits;
            chosen_ocr = (uint8_t)ocr_calc;
            ok = true;
            break;
        }
    }

    if (!ok) {
        // Když se nic nevejde do rozsahu OCR2_MIN..MAX, tak
        // - pokud ocr < OCR2_MIN, zvolíme nejmenší prescaler (nejvyšší f_out) a ořízneme nahoru,
        // - pokud ocr > OCR2_MAX, zvolíme největší prescaler a ořízneme dolů.
        // (Prakticky: držíme se „co nejblíž“ požadované hodnotě.)
        // Zkusíme první prescaler (nejmenší):
        {
            uint32_t presc2 = T2_PRESCALERS[0].presc;
            uint64_t num   = (uint64_t)T1_PRESC * (uint64_t)period_t1_ticks * (uint64_t)MULT_DEN;
            uint32_t denom = (uint32_t)(2u * presc2 * (uint32_t)MULT_NUM);
            int32_t  ocr   = (int32_t)(num / denom) - 1;
            if (ocr < (int32_t)OCR2_MIN) {
                chosen_cs  = T2_PRESCALERS[0].cs_bits;
                chosen_ocr = OCR2_MIN;
            } else {
                // nebo poslední prescaler (největší)
                uint8_t last = (uint8_t)(T2_PRESC_COUNT - 1);
                presc2 = T2_PRESCALERS[last].presc;
                denom  = (uint32_t)(2u * presc2 * (uint32_t)MULT_NUM);
                ocr    = (int32_t)(num / denom) - 1;
                chosen_cs  = T2_PRESCALERS[last].cs_bits;
                if (ocr > (int32_t)OCR2_MAX) chosen_ocr = OCR2_MAX;
                else                         chosen_ocr = (uint8_t)ocr;
            }
        }
    }

    // Aplikuj nastavení na Timer2 (zachovat WGM21|COM20)
    uint8_t base = (1<<WGM21) | (1<<COM20);
    TCCR2 = base | chosen_cs;
    OCR2  = chosen_ocr;
}

/* --- Hlavní --- */
int main(void) {
    timer1_input_init();
    timer2_output_init();
    uart_init();
    sei();

    uart_tx(0x41);
    // Volitelně: „watchdog“ na ztrátu vstupního signálu (pauza výstupu)
    // Zde pro jednoduchost neimplementováno – lze doplnit přes měření doby bez nového capture.

    while (1) {
        if (period_ready) {
            uint32_t p = period_ticks;  // lokální kopie
            period_ready = false;
            t2_set_from_period(p);
        }
        // další logika dle potřeby...
    }
}
