/*
 * Health Monitor H.c
 *
 * Created: 4/20/2018 8:34:28 AM
 * Author : Amilya DD
 * Description : Alarm Health Monitor System
 */

/*---- Variables ------*/
#ifndef F_CPU
#define F_CPU 1000000UL // 1 MHz clock speed
#endif
#define D4 eS_PORTC0
#define D5 eS_PORTC1
#define D6 eS_PORTC2
#define D7 eS_PORTC3
#define RS eS_PORTB1
#define EN eS_PORTB2

#define PREAMBLE	0x40    // ASCII @ 0x40
#define KEYWORD3	0x21	// ASCII ! 0x21 from home TX
#define BPMADDRESS	0x31	// ASCII 1 0x31
#define tempADDRESS 0x32	// ASCII 2 0x32
#define BP2ADDRESS  0x62	// ASCII b 0x62
#define testADDRESS 0x30	// ASCII 0 0x30


/*---- Libraries ------*/
#include <avr/io.h>
#include <util/delay.h>
#include "lcd.h" 

/*---- Function prototype section ------*/
void Dec_Hex (int, int[]);
void USART (void);
char RX(void);
char Check_Key(void);
char Check_Data(char);
void Warning_LCD(void);
void Intro_LCD(void);

int main(void)
{
	 /* Set the Ports as an output. */
	DDRB = 0xFF;
	DDRC = 0xFF;
	
	/*Variables*/
	int BPM= 0;		//BPM data
	int BPD= 0;		//Blood pressure data
	int temp=0;		//temp data
	int LCD[3];		//LCD msb to lsb bits
	char adr=0;		//Save address from frames
	int i=0;		//counter for strolling
	
	//* Enable the LCD subroutines */
	Lcd4_Init();
	Lcd4_Clear();
	USART ();
	
	/* LCD Title Display */
	Intro_LCD();
	
	/* START MAIN */
	for(;;)
	{
		/*  This code essentially only receives alarm signals 
			and displays them on a parallel LCD screen. Its
			the same concept as the home station code. 
		*/
		
		adr=Check_Key(); 
		
		/* Heart Sensor *******************************************************/
		if (adr== BPMADDRESS)
		{
			BPM= Check_Data(adr);
			LCD[3]=0;
			Dec_Hex(BPM, LCD);
			
			Warning_LCD();
			Lcd4_Clear();
			Lcd4_Set_Cursor(2,0);
			Lcd4_Write_String("Heart Rate  >");
			Lcd4_Write_Char(LCD[2]);
			Lcd4_Write_Char(LCD[1]);
			Lcd4_Write_Char(LCD[0]);
			_delay_ms(2000);
			Lcd4_Clear();
		
		/* Temperature *******************************************************/
		} else if (adr==tempADDRESS)
		{
			temp= Check_Data(adr);
			LCD[3]=0;
			Dec_Hex(temp, LCD);
			
			Warning_LCD();
			Lcd4_Clear();
			Lcd4_Set_Cursor(2,0);
			Lcd4_Write_String("Body Temp  > ");
			Lcd4_Write_Char(LCD[2]);
			Lcd4_Write_Char(LCD[1]);
			Lcd4_Write_Char(LCD[0]);
			_delay_ms(2000);
			Lcd4_Clear();
			
		/* Blood Pressure  *******************************************************/
		} else if (adr==BP2ADDRESS)
		{
			BPD= Check_Data(adr);
			LCD[3]=0;
			Dec_Hex(BPD, LCD);
		
			Warning_LCD();
			Lcd4_Clear();
			Lcd4_Set_Cursor(2,0);
			Lcd4_Write_String("SBP Diff  > ");
			Lcd4_Write_Char(LCD[2]);
			Lcd4_Write_Char(LCD[1]);
			Lcd4_Write_Char(LCD[0]);
			_delay_ms(2000);
			Lcd4_Clear();
			
		/* Test  *******************************************************/
		} else if (adr==testADDRESS)
		{
			BPD= Check_Data(adr);
			LCD[3]=0;
			Dec_Hex(BPD, LCD);
		
			Warning_LCD();
			Lcd4_Clear();
			Lcd4_Set_Cursor(2,0);
			Lcd4_Write_String("!!!Test Alarm!!!");
			for(i=0;i<15;i++)
			{
				_delay_ms(250);
				Lcd4_Shift_Left();
			}
			for(i=0;i<15;i++)
			{
				_delay_ms(250);
				Lcd4_Shift_Right();
			}
			_delay_ms(2000);
			Lcd4_Clear();
		}
	}
	return 0; 
}

//**********************************************
//                FUNCTIONS
//**********************************************   Decimal to Hexadecimal Conversionvoid Dec_Hex(int DEC, int HEX[])
{	HEX[2]= DEC/100;							HEX[1]= (DEC - (HEX[2]*100))/10;	HEX[0]= DEC - ((HEX[2]*100) + (HEX[1]*10));	HEX[2]+= 48;	HEX[1]+= 48;	HEX[0]+= 48;
	
	return;
}

//**********************************************  Intro LCD
void Intro_LCD(void)
{
	Lcd4_Set_Cursor(1,1);
	Lcd4_Write_String("Welcome to NAD!");
	_delay_ms(100); 
	Lcd4_Set_Cursor(2,1);
	Lcd4_Write_String(" by Amilya DD");
	_delay_ms(4000); 
	Lcd4_Clear();
	
	return;
}
//**********************************************  "Warning" Display LCD
void Warning_LCD(void)
{
	int i;
	
	Lcd4_Set_Cursor(1,0);
	Lcd4_Write_String("Warning! Warning!");
	_delay_ms(200);
	Lcd4_Clear();
	Lcd4_Write_String("Warning! Warning!");
	Lcd4_Clear();
	_delay_ms(200);
	Lcd4_Write_String("Warning! Warning!");
	
	return;
}
//**********************************************  USART initialize, TX and RX 
void USART (void)
{
	//Baud rate of 2400  (1/440us)
	UBRR0H = 0;
	UBRR0L = 51;
	UCSR0A |= (1<< U2X0);
	//Enable the receiver 
	UCSR0B = (1 << RXEN0);
	//Set 2 stop bits and data bit length is 8-bit
	UCSR0C = (1 << USBS0) | (3 << UCSZ00);
     
 return;	
}
char RX (void)
{
	while  ((UCSR0A & (1 << RXC0))==0);
	return (UDR0);
}
//********************************************** 
char Check_Key (void)
{
	char address=0;
	
	while (RX() != KEYWORD3);	//cjeck for keywork
	address= RX();				//save address
	
	return address;
}
char Check_Data (char add)
{
	char data=0;
	char checksum=0; 
	char checksum_Check=0; 
	
	data= RX();				//save data
	checksum = RX();		//save checksum
	
	checksum_Check = add+data;
	
	if (checksum_Check == checksum)		//do not display if checksum is wrong
	return (data); 
}
