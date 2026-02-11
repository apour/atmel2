/*
 * ATmega8 @ 16 MHz  (LITE verze)
 * - Měření vstupu:  PD6 / ICP1 (Timer1 Input Capture)
 * - Výstup:        PD7 / OC2  (Timer2 CTC + toggle)
 * - Trimr:         ADC0 / PC0  → S ∈ [0.50, 1.50] (±50 %), režim ADC nebo MANUAL
 * - UART:          PD1/TXD, PD0/RXD, 115200 Bd (8-N-1), krátké příkazy
 *
 * Příkazy (ukončené '\n'):
 *   ?          – help
 *   G          – print status
 *   MA         – MODE ADC
 *   MM         – MODE MANUAL
 *   S <val>    – SET S (např. "S 1.20" nebo "S 120%")
 *   R <a> <b>  – SET RANGE pro trimr (např. "R 0.50 1.50")
 *   O 1|0      – OUT ON/OFF
 */

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include <stdbool.h>

/* ================== Uživatelské nastavení (LITE) ================== */
#define UART_BAUD     115200UL
#define UART_USE_U2X  1   // 1: dělič 8 (U2X), 0: dělič 16
#define RX_BUF_SIZE   96
#define LINE_BUF_SIZE 72

/* Timer1: měření (default prescaler = 8) */
static volatile uint16_t g_t1_presc   = 8;
static volatile uint8_t  g_t1_cs_bits = (1<<CS11);

/* EMA filtry (krátké, nastavitelné v kódu) */
static volatile uint8_t  g_adc_ema_shift    = 3; // 1/8
static volatile uint8_t  g_period_ema_shift = 3; // 1/8

/* Scale S = Q10 / 1024, rozsah pro trimr (default ±50 %) */
#define SCALE_DEN     1024u
static volatile uint16_t g_scale_min_q10 = 512;   // 0.50
static volatile uint16_t g_scale_max_q10 = 1536;  // 1.50
static volatile uint16_t g_scale_manual_q10 = 1024; // 1.00
typedef enum { MODE_ADC = 0, MODE_MANUAL = 1 } scale_mode_t;
//static volatile scale_mode_t g_mode = MODE_ADC;
static volatile scale_mode_t g_mode = MODE_MANUAL;

/* Výstup ON/OFF */
static volatile bool g_out_enabled = true;

/* Stav měření / generování */
static volatile uint32_t t1_ovf = 0;
static volatile uint32_t last_capture = 0;
static volatile uint32_t period_ticks = 0;
static volatile bool     period_ready = false;
static volatile uint16_t adc_filt = 0;
static volatile bool     adc_updated = false;

static volatile uint8_t  g_dbg_ocr2 = 127;
static volatile uint16_t g_dbg_p2   = 32;

/* ================== UART TX/RX (LITE) ================== */
static inline void uart_tx(uint8_t b){ while(!(UCSRA&(1<<UDRE))){} UDR=b; }
static void uart_print_P(PGM_P p){ char c; while((c=pgm_read_byte(p++))) uart_tx((uint8_t)c); }
static void uart_print_u32(uint32_t v){
    char buf[11]; uint8_t i=0; if(!v){ uart_tx('0'); return; }
    while(v && i<sizeof(buf)){ buf[i++] = '0'+(v%10u); v/=10u; } while(i--) uart_tx(buf[i]);
}
static void uart_print_u16(uint16_t v){ uart_print_u32(v); }
static void uart_print_fixed_q10(uint16_t q10){
    /* tisk X.YYY (3 dez.) */
    uint32_t x1000 = ((uint32_t)q10*1000u + 512u) >> 10;
    uint32_t ip = x1000/1000u, fp = x1000%1000u;
    uart_print_u32(ip); uart_tx('.'); uart_tx('0'+(fp/100u)%10u);
    uart_tx('0'+(fp/10u)%10u); uart_tx('0'+(fp%10u));
}

/* RX ring + line */
static volatile uint8_t rx_buf[RX_BUF_SIZE];
static volatile uint8_t rx_head=0, rx_tail=0;
ISR(USART_RXC_vect){
    uint8_t d=UDR; uint8_t nh=(uint8_t)(rx_head+1)%RX_BUF_SIZE;
    if(nh!=rx_tail){ rx_buf[rx_head]=d; rx_head=nh; } // jinak drop
}
static bool rx_get(uint8_t* c){
    if(rx_head==rx_tail) return false; *c=rx_buf[rx_tail];
    rx_tail=(uint8_t)(rx_tail+1)%RX_BUF_SIZE; return true;
}

/* ================== Pomocné ================== */
static inline uint32_t ticks_diff(uint32_t now, uint32_t prev){ return (now-prev); }
static inline uint16_t clamp_u16(uint16_t v, uint16_t lo, uint16_t hi){
    if(v<lo) return lo; if(v>hi) return hi; return v;
}

/* ================== Timer/ADC init ================== */
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
static void timer1_input_init(void){
    DDRD &= ~(1<<DDD6); // PD6/ICP1 vstup
    TCCR1A=0;
    TCCR1B=(1<<ICNC1)|(1<<ICES1)|g_t1_cs_bits; // noise cancel, rising, presc
    //TCCR1C=0;
    TIMSK |= (1<<TICIE1)|(1<<TOIE1);
    TCNT1=0; t1_ovf=0; last_capture=0;
}
static void timer2_output_init(void){
    DDRD |= (1<<DDD7); // PD7/OC2 výstup
    TCCR2=(1<<WGM21)|(1<<COM20); // CTC + toggle
    OCR2=127;
}
static void adc_init(void){
    ADMUX=(1<<REFS0); // AVcc, ADC0
    ADCSRA=(1<<ADEN)|(1<<ADFR)|(1<<ADIE)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0); // 125kHz
    //SFIOR &= ~((1<<ADTS2)|(1<<ADTS1)|(1<<ADTS0)); // free-running
    ADCSRA|=(1<<ADSC);
}

/* ================== ISR ================== */
ISR(TIMER1_OVF_vect){ t1_ovf++; uart_print_P(PSTR("Y"));}
ISR(TIMER1_CAPT_vect){
    uart_print_P(PSTR("X"));
    uint32_t now=(t1_ovf<<16)|(uint16_t)ICR1;
    uint32_t dt=ticks_diff(now,last_capture); last_capture=now;
    if(dt){
        static uint32_t ema=0; uint8_t sh=g_period_ema_shift & 0x0F;
        if(!ema) ema=dt; else ema += (int32_t)(dt-ema)>>sh;
        period_ticks=ema; period_ready=true;
    }
}
ISR(ADC_vect){    
    uint16_t v=ADC; static uint16_t a=0; uint8_t sh=g_adc_ema_shift & 0x0F;
    a += (int16_t)(v-a)>>sh; adc_filt=a; adc_updated=true;
}

/* ================== Matematika (bez 64bit dělení) ================== */
/* scale32_q10: výpočet  x * (1024/scale)  bez overflow
   res = (x/scale)<<10 + (((x%scale)<<10)+scale/2)/scale
*/
static inline uint32_t scale32_q10(uint32_t x, uint16_t scale){
    if(!scale) return 0;
    uint32_t q = x / scale;
    uint32_t r = x % scale;
    uint32_t hi = (q << 10);
    uint32_t lo = ((r << 10) + (scale>>1)) / scale; // zaokrouhlení
    return hi + lo;
}

/* Tabulka prescalerů Timer2 */
typedef struct { uint16_t p; uint8_t cs; } t2p_t;
static const t2p_t T2P[] = {
    {   1, (1<<CS20) },
    {   8, (1<<CS21) },
    {  32, (1<<CS21)|(1<<CS20) },
    {  64, (1<<CS22) },
    { 128, (1<<CS22)|(1<<CS20) },
    { 256, (1<<CS22)|(1<<CS21) },
    {1024, (1<<CS22)|(1<<CS21)|(1<<CS20)}
};
#define T2P_N (sizeof(T2P)/sizeof(T2P[0]))
#define OCR2_MIN 10u
#define OCR2_MAX 250u

/* Mapování ADC → S (Q10) v runtime rozsahu */
static inline uint16_t map_adc_to_scale_q10(uint16_t adc10){
    uint16_t lo=g_scale_min_q10, hi=g_scale_max_q10;
    if(hi<lo){ uint16_t t=hi; hi=lo; lo=t; }
    uint32_t span=(uint32_t)(hi-lo);
    uint32_t tmp=(span*adc10 + 511u)/1023u;
    uint16_t out=(uint16_t)(lo+tmp);
    if(out<lo) out=lo; if(out>hi) out=hi; return out;
}

/* Přepočet periody → OCR2 & prescaler (bez 64-bit divide) */
static void t2_set_from_period(uint32_t pticks, uint16_t sc_num){
    if(!g_out_enabled){
        uint8_t base=(1<<WGM21)|(1<<COM20); TCCR2=base; return;
    }
    if(!pticks || !sc_num) return;

    uint8_t chosen_cs=0; uint8_t chosen_ocr=127; uint16_t chosen_p2=32; bool ok=false;

    for(uint8_t i=0;i<T2P_N;i++){
        uint32_t p2=T2P[i].p;

        /* tmp1 = ((pticks / (2*p2)) * g_t1_presc)  — nejdřív dělíme, pak násobíme */
        uint32_t tmp = pticks >> 1;  // /2
        if(p2>1) tmp /= (uint32_t)p2; // /p2
        if(g_t1_presc>1) tmp *= (uint32_t)g_t1_presc;

        /* ocr_calc = tmp * (1024/sc_num) - 1  (škálování přes scale32_q10) */
        uint32_t ocr_calc = scale32_q10(tmp, sc_num);
        if(ocr_calc) ocr_calc -= 1;

        if(ocr_calc>=OCR2_MIN && ocr_calc<=OCR2_MAX){
            chosen_cs=T2P[i].cs; chosen_ocr=(uint8_t)ocr_calc;
            chosen_p2=(uint16_t)p2; ok=true; break;
        }
    }

    if(!ok){
        /* fallback k nejbližší dosažitelné hodnotě */
        uint32_t p2=T2P[0].p;
        uint32_t tmp=pticks>>1; if(p2>1) tmp/=p2; if(g_t1_presc>1) tmp*=g_t1_presc;
        int32_t  ocr=(int32_t)scale32_q10(tmp, sc_num) - 1;
        if(ocr<(int32_t)OCR2_MIN){
            chosen_cs=T2P[0].cs; chosen_ocr=OCR2_MIN; chosen_p2=(uint16_t)p2;
        }else{
            uint8_t last=(uint8_t)(T2P_N-1); p2=T2P[last].p;
            tmp=pticks>>1; if(p2>1) tmp/=p2; if(g_t1_presc>1) tmp*=g_t1_presc;
            ocr=(int32_t)scale32_q10(tmp, sc_num) - 1;
            chosen_cs=T2P[last].cs;
            chosen_ocr=(ocr>(int32_t)OCR2_MAX)?OCR2_MAX:(uint8_t)ocr;
            chosen_p2=(uint16_t)p2;
        }
    }
    uint8_t base=(1<<WGM21)|(1<<COM20);
    TCCR2=base|chosen_cs; OCR2=chosen_ocr;
    g_dbg_ocr2=chosen_ocr; g_dbg_p2=chosen_p2;
}

/* ================== Mini parser řádku (LITE) ================== */
static char line[LINE_BUF_SIZE]; static uint8_t llen=0;
static bool read_line(void){
    uint8_t ch; bool got=false;
    while(rx_get(&ch)){
        if(ch=='\r') continue;
        if(ch=='\n'){ line[llen? (llen<LINE_BUF_SIZE?llen:LINE_BUF_SIZE-1):0]=0; llen=0; got=true; break; }
        if(ch==0x08||ch==0x7F){ if(llen) llen--; }
        else if(llen<LINE_BUF_SIZE-1) line[llen++]=(char)ch;
    }
    return got;
}

/* Parsování čísla do Q10: podporuje "1.25" (max 3 dec) a "125%" */
static bool parse_q10(const char* s, uint16_t* out){
    // přeskoč mezery
    while(*s==' '||*s=='\t') s++;
    // integer
    uint32_t ip=0; bool dig=false;
    while(*s>='0' && *s<='9'){ dig=true; ip=ip*10+(*s-'0'); s++; }
    uint32_t fp=0, m=1;
    if(*s=='.' || *s==','){ s++; uint8_t c=0;
        while(c<3 && *s>='0' && *s<='9'){ fp=fp*10+(*s-'0'); m*=10; s++; c++; }
        while(*s>='0' && *s<='9') s++; // přebytek ignoruj
    }
    bool pct=false; if(*s=='%'){ pct=true; s++; }
    while(*s==' '||*s=='\t') s++;
    if(*s!=0 || !dig) return false;
    uint32_t x1000 = ip*1000u + (m? ((fp*1000u + m/2)/m):0u);
    if(pct) x1000 = (x1000+50u)/100u; // dělení 100 s zaokrouhlením
    uint32_t q10 = (x1000*1024u + 500u)/1000u;
    if(q10>0xFFFFu) q10=0xFFFFu; *out=(uint16_t)q10; return true;
}

/* Tisk krátké nápovědy a stavu */
static const char HLP[]  PROGMEM = 
  "?:help G:stat MA:ADC MM:MAN S x R a b O 1/0\r\n";
static void print_status(uint32_t p, uint16_t sn){
    /* fin = F_CPU/(T1_PRESC * p) */
    uint32_t fin = (uint32_t)(F_CPU/(uint32_t)g_t1_presc) / (p?p:1);
    /* fout = F_CPU/(2*p2*(1+OCR2)) */
    uint32_t denom = (uint32_t)2u*(uint32_t)g_dbg_p2*(uint32_t)(1u+g_dbg_ocr2);
    uint32_t fout = denom? (uint32_t)(F_CPU/denom) : 0;

    uart_print_P(PSTR("fi=")); uart_print_u32(fin);
    uart_print_P(PSTR(" S=")); uart_print_fixed_q10(sn);
    uart_print_P(PSTR(" fo=")); uart_print_u32(fout);
    uart_print_P(PSTR(" O=")); uart_print_u16(g_dbg_ocr2);
    uart_print_P(PSTR(" P2=")); uart_print_u16(g_dbg_p2);
    uart_print_P(PSTR(" T1=")); uart_print_u16(g_t1_presc);
    uart_print_P(PSTR("\r\n"));
}

/* ================== MAIN ================== */
int main(void){
    timer1_input_init();
    timer2_output_init();
    adc_init();
    uart_init();
    sei();

    uart_print_P(PSTR("\r\nAVR Fsync LITE @16MHz\r\n"));
    uart_print_P(HLP);

    while(1){
        if(read_line()){
            const char* s=line;
            // přeskoč mezery
            while(*s==' '||*s=='\t') s++;

            if(s[0]=='?' && s[1]==0){
                uart_print_P(HLP);
            }
            else if(s[0]=='G' && (s[1]==0||s[1]==' ')){
                uint32_t p=period_ticks;
                uint16_t sn=(g_mode==MODE_ADC)? map_adc_to_scale_q10(adc_filt) : g_scale_manual_q10;
                print_status(p,sn);
            }
            else if(s[0]=='M' && s[1]=='A' && s[2]==0){
                g_mode=MODE_ADC; uart_print_P(PSTR("OK MA\r\n"));
            }
            else if(s[0]=='M' && s[1]=='M' && s[2]==0){
                g_mode=MODE_MANUAL; uart_print_P(PSTR("OK MM\r\n"));
            }
            else if(s[0]=='S' && (s[1]==0 || s[1]==' ')){
                uint16_t q; if(!parse_q10(s+1,&q)){ uart_print_P(PSTR("ERR\r\n")); }
                else { g_scale_manual_q10=q; g_mode=MODE_MANUAL; uart_print_P(PSTR("OK S\r\n")); }
            }
            else if(s[0]=='R' && (s[1]==0 || s[1]==' ')){
                // R <min> <max>
                // najdi druhý token
                const char* p=s+1; uint16_t qmin,qmax;
                if(!parse_q10(p,&qmin)){ uart_print_P(PSTR("ERR\r\n")); }
                else{
                    // posuň na další token
                    while(*p && *p!=' ') p++; while(*p==' ') p++;
                    if(!parse_q10(p,&qmax)){ uart_print_P(PSTR("ERR\r\n")); }
                    else { g_scale_min_q10=qmin; g_scale_max_q10=qmax; uart_print_P(PSTR("OK R\r\n")); }
                }
            }
            else if(s[0]=='O' && (s[1]==0 || s[1]==' ')){
                // O 1|0
                const char* p=s+1; while(*p==' ') p++;
                if(*p=='1' && !p[1]){ g_out_enabled=true; uart_print_P(PSTR("OK O1\r\n")); }
                else if(*p=='0' && !p[1]){ g_out_enabled=false; uart_print_P(PSTR("OK O0\r\n")); }
                else uart_print_P(PSTR("ERR\r\n"));
            }
            else{
                uart_print_P(PSTR("?\r\n"));
            }
        }

        /* přepočet při změně periody/trimru/režimu */
        static scale_mode_t last_mode = 0xFF;
        bool do_upd=false; uint32_t p=period_ticks;
        if(period_ready){ period_ready=false; do_upd=true; }
        if(adc_updated && g_mode==MODE_ADC){ adc_updated=false; do_upd=true; }
        if(last_mode!=g_mode){ last_mode=g_mode; do_upd=true; }

        if(do_upd && p){
            uint16_t sn = (g_mode==MODE_ADC)? map_adc_to_scale_q10(adc_filt) : g_scale_manual_q10;
            t2_set_from_period(p, sn);
            /* krátký auto‑status každou změnu – lze odkomentovat:
               print_status(p,sn);
            */
        }
    }
}