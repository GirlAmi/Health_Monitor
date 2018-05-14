/*
 * Home Station.c
 *
 * Created: 2018-05-11 11:31:12 PM
 * Author : Amilya DD
 * Description : Home Health Monitor System
 */ 

/*---- Libraries ------*/
#include <avr/io.h>			// similar to stdio.h
#include <util/delay.h>		// required for delay_ms()
#include <stdint.h>
#include "i2c_master.h"		// Github.com
#include "i2c_master.c"		// Github.com

/*---- Variables ------*/
#define F_CPU 1000000UL		//must match clock selection from AVR Studio
#define PREAMBLE	0x40    // ASCII @ 0x40
#define KEYWORD		0x37	// ASCII 7 0x37 for portable TX
#define KEYWORD2    0x35	// ASCII 5 0x35 for home TX
#define KEYWORD3    0x21	// ASCII ! 0x21 for alarm
#define FEEDBACK	0x4B	// ASCII K 0x4B
#define BPMADDRESS	0x31	// ASCII 1 0x31
#define tempADDRESS 0x54	// ASCII T 0x54
#define BP1ADDRESS  0x42	// ASCII B 0x42
#define BP2ADDRESS  0x62	// ASCII b 0x62
#define testADDRESS  0x30	// ASCII 0 0x30

/*---- Function prototype section ------*/
void Dec_Hex (char, char[]);
void Lookup (char[], int);
void USART (void);
void TX (char);
int RX(void);
void Packet(char, char);
void LCD_Clear (void);
void LCD_1line (void);
void LCD_2line (void);
void LCD_3line (void);
void LCD_4line (void);
char Check_Packet(void);
char Checksum_Check(char);
void Packet_feedback (void);
void Packet_Alarm (char, char );
void Intro(void);

char BPMD[] =		{"Heart Rate(BPM) >"};
char TempD[] =		{"Body Temp (C)   >"};
char Pulse_Repo[] = {"NA"};
char BloodPD1[] =	{"SBP Right (mmHg)>"};
char BloodPD2[] =	{"SBP Left  (mmHg)>"};

int main(void)
{
    /* Enable the ADC and LCD functions */
    i2c_init();
    USART ();
    
    /*Variables*/
	char adr= 0;					//Save address from frames
	char temp = 0;					//Temperature measurement and calculation
	char BPM= 0;					//BPM Calculation
	char LCD[3];					//LCD msb to lsb bits
	int end =0;						//stop value for LCD text
	int BP1= 0;						//Blood pressure calculation for right arm
	int BP2= 0;						//Blood pressure calculation for left arm
	int BP=0;						//Difference in blood pressure
	int test_cnt=0;					//Test counter for test alarm
	
	/* LCD Title Display */
	Intro(); 
	
	/* START MAIN */
	for(;;)
	{
		
		/* The following code continuously checks to receive a specific keyword.
		   If the keyword is received, an address is saved and directed to the 
		   appropriate reading. The data is then analyzed and if they are found to 
		   be within a range, a signal is sent out to the alarm station. A test
		   alarm signal is sent out every 10 counts. 
		*/
		
		UCSR0B = (1 << RXEN0);			//Enable RX
		adr= Check_Packet();			//Check key word and save address
	
		/* Heart Sensor *******************************************************/
		if (adr== BPMADDRESS)
		{
			BPM= Checksum_Check(adr);   //Save data
			
			/* Convert Decimal to hex for LCD */
			LCD[3]=0;
			Dec_Hex(BPM, LCD);
			
			/* LCD temp Display */
			i2c_start(0x50);				// Comm with LCD address
			_delay_ms(40);
			LCD_1line(); 
			end= 0x3E;
			Lookup (BPMD, end);
			i2c_write(LCD[2]);
			i2c_write(LCD[1]); 					
			i2c_write(LCD[0]);
			i2c_stop();
			
			if ((110<BPM) | (BPM<60))
			{
				_delay_ms(1);
				Packet_Alarm(BPM, BPMADDRESS);
				_delay_ms(40);
			}
			
		/* Temperature *******************************************************/
		} else if (adr== tempADDRESS)
		{
			temp= Checksum_Check(adr); 
			
			/* Convert Decimal to hex for LCD */
			LCD[3]=0;
			Dec_Hex(temp, LCD);
			
			/* LCD temp Display */
			i2c_start(0x50);					// Comm with LCD address
			_delay_ms(40);
			LCD_2line ();
			end= 0x3E;
			Lookup (TempD, end);
			i2c_write(LCD[2]);
			i2c_write(LCD[1]);					// ASCII equiv. of BPM 10
			i2c_write(LCD[0]);					// ASCII equiv. of BPM 1
			i2c_stop();
			
			if ((38<BPM) | (BPM<36))
			{
				_delay_ms(1);
				Packet_Alarm(temp, tempADDRESS);
				_delay_ms(40);
			}
		/* Blood Presure 1 *******************************************************/
		} else if (adr== BP1ADDRESS)
		{
			BP1= Checksum_Check(adr); 
			
			/* Convert Decimal to hex for LCD */
			LCD[3]=0;
			Dec_Hex(BP1, LCD);
			
			/* LCD temp Display */
			i2c_start(0x50);					// Comm with LCD address
			_delay_ms(40);
			LCD_3line();
			end= 0x3E;
			Lookup (BloodPD1, end);
			i2c_write(LCD[2]); 
			i2c_write(LCD[1]); 
			i2c_write(LCD[0]);
			i2c_stop();
			
		/* Blood Presure 2 *******************************************************/
		}	else if (adr== BP2ADDRESS)
		{
			BP2= Checksum_Check(adr); 
			
			/* Convert Decimal to hex for LCD */
			LCD[3]=0;
			Dec_Hex(BP2, LCD);
			
			/* LCD temp Display */
			i2c_start(0x50);					// Comm with LCD address
			_delay_ms(40);
			LCD_4line ();
			end= 0x3E;
			Lookup (BloodPD2, end);
			i2c_write(LCD[2]); 
			i2c_write(LCD[1]);
			i2c_write(LCD[0]); 
			i2c_stop();
			
			BP= BP1-BP2;	// ex 103-113 = -10 therefor no
			if (BP>=10)
			{
				_delay_ms(1);
				Packet_Alarm(BP, BP1ADDRESS);
				_delay_ms(40);
			}
			
			BP= BP2-BP1;	// ex 113-103 = 10 therefor yes
			if (BP>=10)
			{
				_delay_ms(1);
				Packet_Alarm(BP, BP2ADDRESS);
				_delay_ms(40);
			}
		}
		/* Test *******************************************************/
			test_cnt++;
			if (test_cnt==10)
			{
				_delay_ms(1);
				Packet_Alarm(0, testADDRESS);
				_delay_ms(40);
				test_cnt=0;
			} 
	}
	return 0; 
}

//**********************************************
//                FUNCTIONS
//********************************************** 
//********************************************** Decimal to Hexadecimal Conversion
void Dec_Hex(char DEC, char HEX[])
{
	HEX[2]= DEC/100;						
	HEX[1]= (DEC - (HEX[2]*100))/10;	
	HEX[0]= DEC - ((HEX[2]*100) + (HEX[1]*10));
	HEX[2]+= 48;
	HEX[1]+= 48;
	HEX[0]+= 48;
	
	return;
}

//********************************************** LCD Clear and set line
void LCD_Clear (void)
{
	i2c_write(0xFE);					// 0xFE lets it know command not data
	i2c_write(0x51);					// Clears Screen
			
	return;
}
void LCD_1line (void)
{
	i2c_write(0xFE);					//0xFE lets it know command not data
	i2c_write(0x45);					//Set cursor
	i2c_write(0x00);					//Second line
	
	return;
}

void LCD_2line (void)
{
	i2c_write(0xFE);					//0xFE lets it know command not data
	i2c_write(0x45);					//Set cursor
	i2c_write(0x40);					//Second line
	
	return;
}
void LCD_3line (void)
{
	i2c_write(0xFE);					//0xFE lets it know command not data
	i2c_write(0x45);					//Set cursor
	i2c_write(0x14);					//Second line
	
	return;
}
void LCD_4line (void)
{
	i2c_write(0xFE);					//0xFE lets it know command not data
	i2c_write(0x45);					//Set cursor
	i2c_write(0x54);					//Second line
	
	return;
}
void Intro(void)
{
	char Title1[]= {"Welcome to NAD!"};
	char Title2[]= {"Your Personalized"};
	char Title3[]= {"Health Monitor"};
	char Title4[]= {"by Amilya DD."};
		
	int end=0;
			
	i2c_start(0x50);					// Comm with LCD address
	_delay_ms(40);
	LCD_Clear(); 
	end= 0x21;
	Lookup (Title1, end);
	_delay_ms(40);
	LCD_2line();
	end= 0x64;
	Lookup (Title2, end);
	_delay_ms(40);
	LCD_3line();
	end= 0x72;
	Lookup (Title3, end);
	_delay_ms(40);
	LCD_4line();
	end= 0x2E;
	Lookup (Title4, end);
	_delay_ms(4000);
	LCD_Clear(); 
	i2c_stop();
	
	return;
}
//********************************************** Simple lookup table to display test on LCD
void Lookup ( char look[], int stop )
{                                                                                                                                                                                                                                                                                      
	int ltr=0;						//lookup table letter
	int x=0;						//lookup table loop counter
	
	while (ltr != stop)				//lookup table loop title
	{
		ltr= look[x++];
		i2c_write(ltr);
	}
	return;
} 
//**********************************************  USART initialize, TX and RX
/* In order to transmit and receiver frames to the portable and alarm 
station,the USART on the Atmega328p needs to be initialized
to set the appropriate BAUD rate and frame start and
stop bits when sending data. The initialization code
can be found on the Atmega328p data sheet.
*/ 
void USART (void)
{
	//Baud rate of 2400  (1/440us)
	UBRR0H = 0;
	UBRR0L = 51;
	UCSR0A |= (1<< U2X0);
	//Set 2 stop bits and data bit length is 8-bit
	UCSR0C = (1 << USBS0) | (3 << UCSZ00);
     
 return;	
}
int RX (void)
{
	while  ((UCSR0A & (1 << RXC0))==0);
	return (UDR0);
}
void TX (char TX_Data)
{	
	//Enable Transmitter
	UCSR0B = (1 << TXEN0);
	//Wait until the Transmitter is ready
	while (! (UCSR0A & (1 << UDRE0)) );
	//Send Data
	UDR0 = TX_Data;
	//Disable Transmitter
	UCSR0B = (0 << TXEN0);
	return;
}

//********************************************** Frame Loop 
/* In order to send out the data received from 
   the sensors to the home station, the data must 
   be inserted inside a frame that consist of 3 preambles, 
   a keyword, an address, the data and a checksum that 
   simple adds up the address and data. The following 
   codes demonstrate the process of sending out data. 
*/
void Packet_Alarm (char data, char address)
{ 
	char checksum=0; 
	
	TX(PREAMBLE);
	TX(PREAMBLE);
	TX(PREAMBLE);
	TX(KEYWORD3);
	TX(address); 
	TX(data);
	checksum= address+data;
	TX(checksum);
	
	return;
}
//********************************************** Frame Loop 
void Packet_feedback (void)
{
	TX(PREAMBLE);
	TX(PREAMBLE);
	TX(PREAMBLE);
	TX(KEYWORD2);
	TX(FEEDBACK);
	
	return;
}
//********************************************** Check Packet and Checksum
/* The following code continuously checks for a keyword. 
   If keyword is found, it saves the next byte to address. 
*/
char Check_Packet(void)
{
	char address=0; 
	
	while (RX()!= KEYWORD); 
	address =RX(); 
	
	return address;	
}
/* The following code saves the data and checksum. 
   It then checks to see if checksum if correct. 
   If not, send out frame to notify portable station. 
*/
char Checksum_Check(char adr)
{
	char data=0;
	char checksum=0; 
	char checksum_C=0; 
	
	data = RX(); 
	checksum = RX(); 
	
	UCSR0B = (0 << RXEN0);
	
	checksum_C= adr+data; 
	
	if (checksum_C == checksum)
	return data;
	//{
	//_delay_ms(1);
	//Packet_feedback();
	//_delay_ms(40);
	//} 
		
}