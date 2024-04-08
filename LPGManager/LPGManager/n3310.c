 
#include <util/delay.h>
#include "n3310.h"

#define ADD_SMALL_DELAY 1

char char_start;
int x_souradnice,y_souradnice;
//uint8_t kontrast = 30;	// without LED Light
uint8_t kontrast = 61;		// with LED Light
char delayCounter;

/************** Inicializace SPI sbernice *****************/
void spi_init(void)
{
   SPCR = 0b01011111;  
   // Interrupt disable, SPI enable, MSB is transmitted first, MASTER enabled, Clock polariy, Clock phase, fosc/4
}


/************** Inicializace displeje *****************/
void LCD_init ( void )
{
	_delay_ms(100);
	CLEAR_SCE_PIN;    //Enable LCD
    CLEAR_RST_PIN;	//reset LCD
    _delay_ms(100);
    SET_RST_PIN;
	SET_SCE_PIN;	//disable LCD
	_delay_ms(100);

    LCD_writeCommand( 0x21 );  // LCD Extended Commands.
    
	//LCD_writeCommand( 0b10011111 );  			// Set LCD Vop (Contrast).
	LCD_writeCommand( 0b10000000 + kontrast );  // Set LCD Vop (Contrast).
    LCD_writeCommand( 0x04 );  					// Set Temp coefficent.
    LCD_writeCommand( 0x13 );  					// LCD bias mode 1:48.
    LCD_writeCommand( 0x20 );  					// LCD Standard Commands, Horizontal addressing mode.
    LCD_writeCommand( 0x0c );  					// LCD in normal mode.

	_delay_ms(200);
    LCD_clear();
}


/************** Zapsat prikaz *****************/
void LCD_writeCommand ( char command )
{
    CLEAR_SCE_PIN;	  //enable LCD

	CLEAR_DC_PIN;	  //set LCD in command mode
		
    //  Send data to display controller.
    SPDR = command;

    //  Wait until Tx register empty.
    while ( !(SPSR & 0x80) );

    SET_SCE_PIN;   	 //disable LCD
}


/************** Zapsat data *****************/
void LCD_writeData ( char Data )
{
    CLEAR_SCE_PIN;	  //enable LCD
	SET_DC_PIN;	  //set LCD in Data mode
	
    //  Send data to display controller.
    SPDR = Data;

    //  Wait until Tx register empty.
    while ( !(SPSR & 0x80) );
	
    SET_SCE_PIN;   	 //disable LCD
}

/************** Smazani obrazovky *****************/
void LCD_clear ( void )
{
    int i,j;
	
	LCD_gotoXY (0,0);  	//start with (0,0) position

    for(i=0; i<8; i++)
	  for(j=0; j<90; j++)
	     LCD_writeData( 0x00 );
   
    LCD_gotoXY (0,0);	//bring the XY position back to (0,0)
      
}



/************** Pohyb kurzoru na X,Y souradnice *****************/
void LCD_gotoXY ( char x, char y )
{
    LCD_writeCommand (0x80 | x);   //column
	LCD_writeCommand (0x40 | y);   //row
}


/************** Zapsani jednoho znaku *****************/
void LCD_writeChar (unsigned char ch)
{
   char j;
   for(j=0; j<5; j++)
     LCD_writeData(pgm_read_byte(&( smallFont [(ch-32)*5 + j] )));
	 
   LCD_writeData( 0x00 );						// mezera za znakem	
} 


/************** Zapsani stringu malym pismem *****************/
void LCD_writeString_F ( const char *string)
{
    while ( *string )
        LCD_writeChar( *string++ );
}


/************** Nakresleni ramecku *****************/
void LCD_drawBorder (void )
{
  char i, j;  
    
  for(i=0; i<6; i++)
  {
    LCD_gotoXY (0,i);
	
	for(j=0; j<84; j++)
	{
	  if(j == 0 || j == 83)
		LCD_writeData (0xff);		// first and last column solid fill to make line
	  else if(i == 0)
	    LCD_writeData (0x01);		// row 0 is having only 5 bits (not 8)
	  else if(i == 5)
	    LCD_writeData (0x01);		// row 6 is having only 3 bits (not 8)
	  else
	    LCD_writeData (0x00);
	}
  }
}	

void LCD_setPos(int radek, int sloupec)
{
	char_start=sloupec;
	y_souradnice=radek;
}


/************** Invertovani - bile pismo a cerne pozadi *****************/
void LCD_invertovat (void)
{
 LCD_writeCommand(0b00001101);
}


void LCD_smaz_velke (void )
{
   char i, j;  
    
  for(i=0; i<3; i++) 
  {
    LCD_gotoXY (0,i);
	
	for(j=0; j<84; j++)
	{
	
		LCD_writeData ( 0);		// first and last column solid fill to make line
	
	}
  }
}



