/*
 * Health Monitor P.c
 *
 * Created: 1/31/2018 8:47:51 AM
 * Author : Amilya DD
 * Description : Portable Health Monitor System
 */ 

/*---- Libraries ------*/
#include <avr/io.h>			// similar to stdio.h 
#include <util/delay.h>		// required for delay_ms() 
#include <stdint.h>
#include "i2c_master.h"		// Github.com
#include "i2c_master.c"		// Github.com

/*---- Variables ------*/
#define F_CPU 1000000UL		//must match clock selection from AVR Studio
#define ADC_PIN      0		//ADC0 for pulse sensor #1 finger #1
#define ADC_PIN1     1		//ADC1 for temperature 
#define ADC_PIN2     2		//ADC2 for pulse sensor #2 ear 
#define ADC_PIN3     3		//ADC3 for pulse sensor #3 finger #2
#define	LED_PIN	    PB0     //Sets LED output to PB0
#define	SYNC_P		PD7     //Sets sync pulse output to PD7 for each byte
#define THOLD	    550		//2.5V

#define PREAMBLE	0x40    // ASCII @ 0x40
#define KEYWORD		0x37	// ASCII 7 0x37 for portable TX
#define KEYWORD2    0x35	// ASCII 5 0x35 for home TX
#define FEEDBACK	0x4B	// ASCII K 0x4B
#define BPMADDRESS	0x31	// ASCII 1 0x31
#define tempADDRESS 0x32	// ASCII 2 0x32
#define BP1ADDRESS  0x33	// ASCII 3 0x33
#define BP2ADDRESS  0x34	// ASCII 4 0x34

/*---- Function prototype section ------*/
uint16_t ADC_read(uint8_t adcx);
void ADC_init(void);
void Intro (int, int);
void Dec_Hex (int, int[]);
void Lookup (char[], int);
void USART (void);
void TX (char);
int RX(void);
void Packet(char, char);
void LCD_Clear (void);
void LCD_1line (void);
void LCD_2line (void);
void Transmit_Check(int, char);

	
char BPMD[] = {"Heart Rate (BPM)>"};
char TempD[] = {"Body Temp  (C)  >"};
char Pulse_Repo[] = {"RPS"};
char BloodPD1[] = {"SBP Right (mmHg)>"};
char BloodPD2[] = {"SBP Left  (mmHg)>"};
char Title[] = {"   Welcome to NAD!"};
char Title1[] = {"    by Amilya DD."};

//**************************************************
int main(void) 
{
	/* Enable the ADC and LCD functions */
	ADC_init();
	i2c_init();
	USART ();
	
	/* Set the Ports as an output or input. */
	DDRB  |= _BV(LED_PIN);			//Output
	DDRD  |= _BV(SYNC_P);		   //Output
	
	/*Variables*/
	int pulse = 0;
	float temp = 0;
    int Pcnt = 0;					//Period time count
	int Tcnt = 0 ;					//Total count
	float avgcnt = 0;				//avg pulse time
	int BPM= 0;						//BPM calculation
	int LCD[3];
	int end =0; 
	int OK_cnt=0;
	int BP1= 0;
	int BP2= 0;
	int packet_cnt=0; 
	 
	//helloworld    
	/* LCD Title Display */
	i2c_start(0x50);				// Comm with LCD address
	_delay_ms(400);
	LCD_Clear ();
	end= 0x21;
	Lookup (Title, end );
	_delay_ms(40);
	LCD_2line ();
	end= 0x2E;
	Lookup (Title1, end );
	_delay_ms(4000);
	i2c_stop();

	
	/* START MAIN */
	for(;;)
	{	
		/* Heart Sensor *******************************************************
		Tcnt=0;									//Clear all variables each loop
		avgcnt = 0;
		do
		{
			pulse = ADC_read(ADC_PIN2);			//Check pulse
			if (pulse >  THOLD)					//If pulse is over threshold continue
			{	PORTB |= _BV(LED_PIN);			//LED ON
					
				do
				{  pulse = ADC_read(ADC_PIN2);	//This loop measured the period for each pulse
				   Pcnt++;
				} while (pulse >  THOLD);
				
				Tcnt++;
				avgcnt= avgcnt + Pcnt;			//Add period time to variable and clear
				Pcnt= 0;
			}
			else
			PORTB &= ~_BV(LED_PIN);				//LED OFF
				
		} while (Tcnt < 11);					//Count period 10 times
		
		BPM= (60 * (1/(avgcnt/1400)));			//BPM Calculations
			
		/* Convert Decimal to hex for LCD */
		BPM++;
		if (BPM == 180)
		BPM=0;
		LCD[3]=0;
		Dec_Hex(BPM, LCD);
		
		/* LCD BPM Display */
		i2c_start(0x50);						// Comm with LCD 0Faddress
		_delay_ms(40);
		LCD_Clear();
		end= 0x3E;
		Lookup (BPMD, end);
		//if ((180<BPM) | (BPM<50))
		//{
		//	end= 0x53;
		//	Lookup (Pulse_Repo, end);
		//	_delay_ms(40);
		//	i2c_stop();
		//} else
		//{
			i2c_write(LCD[2]);
			_delay_ms(10);
			i2c_write(LCD[1]);					// ASCII equiv. of BPM 10
			_delay_ms(10);
			i2c_write(LCD[0]);					// ASCII equiv. of BPM 1
			_delay_ms(1000);
			i2c_stop();
		//}
	
		/* Transmit Frame */
		//while (packet_cnt==3)
		//{
			_delay_ms(1);
			Packet(BPM, BPMADDRESS);
			//Transmit_Check (BPM, BPMADDRESS);
			_delay_ms(40);
		//	packet_cnt++; 
		//}
		//	packet_cnt=0; 
									
		/* Temperature Sensor *************************************************/
		//''temp = ADC_read(ADC_PIN1);			//Check temperature
		temp = (temp -102)/2;
		
		temp++; 
		if (temp == 45)
		temp=0;
	
		
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
		_delay_ms(10);
		i2c_write(LCD[1]);					// ASCII equiv. of BPM 10
		_delay_ms(10);
		i2c_write(LCD[0]);					// ASCII equiv. of BPM 1
		_delay_ms(1000);
		i2c_stop();

		/* Transmit Frame */
		//while (packet_cnt==3)
		//{
			_delay_ms(1);
			Packet(temp, tempADDRESS);
			//Transmit_Check (BPM, BPMADDRESS);
			_delay_ms(40);
			//packet_cnt++;
	//	}
	//	packet_cnt=0;
		
		/* Blood Pressure 1 *******************************************************
		Tcnt=0;									//Clear all variables each loop
		avgcnt = 0;
		do
		{
			pulse = ADC_read(ADC_PIN);			//Check pulse
			if (pulse >  THOLD)					//If pulse is over threshold continue
			{	
				do
				{  pulse = ADC_read(ADC_PIN2);	//This loop measured the period for each pulse
					Pcnt++;
				} while (pulse >  THOLD);
			
				Tcnt++;
				avgcnt= avgcnt + Pcnt;			//Add period time to variable and clear
				Pcnt= 0;
			}
		} while (Tcnt < 11);					//Count period 10 times
		
		BP1= 0.325*(avgcnt/10)+122.35;			//BPM Calculations
			
		/* Convert Decimal to hex for LCD */
		BP1= 132; 
		LCD[3]=0;
		Dec_Hex(BP1, LCD);
		
		/* LCD temp Display */
		i2c_start(0x50);					// Comm with LCD address
		_delay_ms(40);
		LCD_Clear ();
		end= 0x3E;
		Lookup (BloodPD1, end);
		i2c_write(LCD[2]);
		_delay_ms(10);
		i2c_write(LCD[1]);					// ASCII equiv. of BPM 10
		_delay_ms(10);
		i2c_write(LCD[0]);					// ASCII equiv. of BPM 1
		_delay_ms(1000);
		i2c_stop();
		
		/* Transmit Frame */
		//while (packet_cnt==3)
		//{
			_delay_ms(1);
			Packet(BP1, BP1ADDRESS);
			//Transmit_Check (BPM, BPMADDRESS);
			_delay_ms(40);
		//	packet_cnt++;
	//	}
		//packet_cnt=0;
		
		/* Blood Pressure 2 *******************************************************
		Tcnt=0;									//Clear all variables each loop
		avgcnt = 0;
		do
		{
			pulse = ADC_read(ADC_PIN);			//Check pulse
			if (pulse >  THOLD)					//If pulse is over threshold continue
			{
				do
				{  pulse = ADC_read(ADC_PIN3);	//This loop measured the period for each pulse
					Pcnt++;
				} while (pulse >  THOLD);
				
				Tcnt++;
				avgcnt= avgcnt + Pcnt;			//Add period time to variable and clear
				Pcnt= 0;
			}
		} while (Tcnt < 11);					//Count period 10 times
		
		//BP2= 0.325*(avgcnt/10)+122.35;			//BPM Calculations
		
		/* Convert Decimal to hex for LCD */
		BP2= 130;
		LCD[3]=0;
		Dec_Hex(BP2, LCD);
		
		/* LCD temp Display */
		i2c_start(0x50);					// Comm with LCD address
		_delay_ms(40);
		LCD_2line ();
		end= 0x3E;
		Lookup (BloodPD2, end);
		i2c_write(LCD[2]);
		_delay_ms(10);
		i2c_write(LCD[1]);					// ASCII equiv. of BPM 10
		_delay_ms(10);
		i2c_write(LCD[0]);					// ASCII equiv. of BPM 1
		_delay_ms(1000);
		i2c_stop();
		
		/* Transmit Frame */
	//	while (packet_cnt==3)
	//	{
			_delay_ms(1);
			Packet(BP2, BP2ADDRESS);
			//Transmit_Check (BPM, BPMADDRESS);
			_delay_ms(40);
		//	packet_cnt++;
	//	}
	//	packet_cnt=0;
		
	}	
	return 0;
}
//**********************************************
//                FUNCTIONS 
//********************************************** Provided by Github 
void ADC_init(void)
{
	// Enable ADCMUX
	ADMUX = (1<<REFS0); 
	// ADC Enable and prescaler of 128
	ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0); 
	return;
}

uint16_t ADC_read(uint8_t ch) 
{
	//Select Channel 
	ADMUX	&=	0xf8;
	ADMUX	|=	ch;
	// This starts the conversion
	ADCSRA |= _BV(ADSC);
	//Wait till Conversion
	while ( (ADCSRA & _BV(ADSC)) );
	return ADCW;
}
//********************************************** Decimal to Hexadecimal Conversion
void Dec_Hex(int DEC, int HEX[])
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
//********************************************** Simple lookup table to display test on LCD
void Lookup ( char look[], int stop )
{                                                                                                                                                                                                                                                                                      
	int ltr=0;						//lookup table letter
	int x=0;						//lookup table loop counter
	
	while (ltr != stop)				//lookup table loop title
	{
		ltr= look[x++];
		i2c_write(ltr);
		_delay_ms(10);
	}
	return;
} 
//**********************************************  USART initialize, TX and RX 
void USART (void)
{
	//Baud rate of 2400  (1/440us)
	UBRR0H = 0;
	UBRR0L = 51;
	UCSR0A |= (1<< U2X0);
	//UBRR0L = (unsigned char) BAUD;
	//Enable the receiver and transmitter
	//UCSR0B = (1 << RXEN0);
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
	PORTD |= _BV(SYNC_P);
	_delay_us(1);
	PORTD &= ~_BV(SYNC_P);	
	
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
void Packet (char data, char address)
{ 
	char checksum=0; 
	
	TX(PREAMBLE);
	TX(PREAMBLE);
	TX(PREAMBLE);
	TX(KEYWORD);
	TX(address); 
	TX(data);
	checksum= address+data;
	TX(checksum);
	
	return;
}

void Transmit_Check (int data, char address)
{
	int OK_cnt=0;
	int fback=0;
	int keyword=0;
	
	UCSR0B = (1 << RXEN0);
	while ((RX() != KEYWORD2) & (OK_cnt<10))
	{
		OK_cnt++;
	}
	fback=RX(); 
	UCSR0B = (0 << RXEN0);		
	if (fback=0x4B)	
	{ 
		_delay_ms(1);
		Packet(data, address);
		_delay_ms(40);
	}
	return ;
}