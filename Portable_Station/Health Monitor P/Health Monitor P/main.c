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
#include "i2c_master.h"		// I2C Protocol Library from Github.com
#include "i2c_master.c"		// I2C Protocol Library from Github.com

/*---- Variables ------*/
#define F_CPU 1000000UL		//must match clock selection from AVR Studio
#define ADC_PIN      0		//ADC0 for pulse sensor #1 situated at right earlobe
#define ADC_PIN1     1		//ADC1 for temperature 
#define ADC_PIN2     2		//ADC2 for pulse sensor #2 right index finger
#define ADC_PIN3     3		//ADC3 for pulse sensor #3 left index finger
#define	LED_PIN	    PB0     //Sets LED output to PB0
#define	SYNC_P		PD7     //Sets sync pulse output to PD7 for each byte
#define THOLD	    550		//2.68V

#define PREAMBLE	0x40    // ASCII @ 0x40
#define KEYWORD		0x37	// ASCII 7 0x37 for portable TX
#define KEYWORD2    0x35	// ASCII 5 0x35 for home TX
#define FEEDBACK	0x4B	// ASCII K 0x4B
#define BPMADDRESS	0x31	// ASCII 1 0x31
#define tempADDRESS 0x54	// ASCII T 0x54
#define BP1ADDRESS  0x42	// ASCII B 0x42
#define BP2ADDRESS  0x62	// ASCII b 0x62

/*---- Function prototype section ------*/
uint16_t ADC_read(uint8_t adcx);
void ADC_init(void);
void Intro (int, int);
void Dec_Hex (int, int[]);
void Lookup (char[], int);
void USART (void);
void TX (char);
int RX(void);
void Frame(char, char);
void LCD_Clear (void);
void LCD_1line (void);
void LCD_2line (void);
void Transmit_Check(int, char);

char Title1[] =		{"   Welcome to NAD!"};
char Title2[] =		{"    by Amilya DD."};
char BPMD[] =		{"Heart Rate (BPM)>"};
char TempD[] =		{"Body Temp  (C)  >"};
char Pulse_Repo[] = {"RPS"};
char BloodPD1[] =	{"SBP Right (mmHg)>"};
char BloodPD2[] =	{"SBP Left  (mmHg)>"};


//**************************************************
int main(void) 
{
	/* Enable the ADC, LCD and USART functions */
	ADC_init();
	i2c_init();
	USART ();
	
	/* Set the Ports as an output or input. */
	DDRB  |= _BV(LED_PIN);			//Output
	DDRD  |= _BV(SYNC_P);			//Output
	
	/*Variables*/
	int pulse = 0;					//ADC measurement
	float temp = 0;					//Temperature measurement and calculation
    int Pcnt = 0;					//Period time count for pulse sensor
	int Tcnt = 0 ;					//Total count for pulse sensor
	float avgcnt = 0;				//AVG period time for pulse sensor
	int BPM= 0;						//BPM Calculation
	int LCD[3];						//LCD msb to lsb bits
	int end =0;						//stop value for LCD text
	int BP1= 0;						//Blood pressure calculation for right arm
	int BP2= 0;						//Blood pressure calculation for left arm
	
	/* LCD Title Display */
	i2c_start(0x50);				// Comm with LCD address
	_delay_ms(400);
	LCD_Clear ();					//Clear display to write on first line
	end= 0x21;						//Display Title 1
	Lookup (Title1, end );
	_delay_ms(40);
	LCD_2line ();					//Write on second line
	end= 0x2E;						//Display Title 2
	Lookup (Title2, end );
	_delay_ms(4000);
	i2c_stop();

	
	/* START MAIN */
	for(;;)
	{	
		/* Heart Sensor *******************************************************
		   Section 1  is used to calculate the heart rate in BPM. Once initialized, 
		   the code checks the ADC input until the pulse sensor reaches its set 
		   threshold which is essentially the systolic wave. Knowing the pulse sensor 
		   will emit a voltage above or below midpoint (2.5V) depending on the changes 
		   in light intensity, the threshold is set to 550bits (2.6V). When this threshold 
		   is met, an LED goes high demonstrating the code has read the pulse. 
		   To calculate BPM, it measures the period in between each pulse. Essentially 
		   its the heart rates frequency multiplied by 60. In order to get an accurate 
		   reading due to possible faulty measurements, the code will get an average 
		   time reading for each period. The following code takes the average period time
		   for 10 pulse measurements. A simple decimal to hexadecimal conversion was needed 
		   to display the BPM on the LCD. In order to send ASCII characters to the LCD, a 
		   lookup table with a loop needed to be created.
		*/
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
				}  while (pulse >  THOLD);
				Tcnt++;
				avgcnt= avgcnt + Pcnt;			//Add period time to variable and clear
				Pcnt= 0;
			}
			else
			PORTB &= ~_BV(LED_PIN);				//LED OFF	
		} while (Tcnt < 11);					//Count period 10 times
		
		BPM= (60 * (1/(avgcnt/1400)));			//BPM Calculations
			
		/* Convert Decimal to hex for LCD */
		LCD[3]=0;
		Dec_Hex(BPM, LCD);
		
		/* LCD BPM Display */
		i2c_start(0x50);						// Comm with LCD 0Faddress
		_delay_ms(40);
		LCD_Clear();
		end= 0x3E;
		Lookup (BPMD, end);
		if ((180<BPM) | (BPM<50))				//If BPM shows impossible readings
		{										//tell user to reposition sensor
			end= 0x53;
			Lookup (Pulse_Repo, end);
			_delay_ms(40);
			i2c_stop();
		} else
		{
			i2c_write(LCD[2]);					// MSB number of data
			_delay_ms(10);
			i2c_write(LCD[1]);					
			_delay_ms(10);
			i2c_write(LCD[0]);					// LSB number of data
			_delay_ms(1000);
			i2c_stop();
		}
	
		/* Transmit Frame */
		_delay_ms(1);
		Frame(BPM, BPMADDRESS);
		//Transmit_Check (BPM, BPMADDRESS);
		_delay_ms(100);
									
		/* Temperature Sensor *************************************************
		   Section 2 is used to measure the temperature with the MCP9700 chip 
		   which will output 0.5V at 0 degrees C, 0.75V at 25 C, and 10mV per degree C. 
		   Knowing the body temperature will not exceed 0 degrees C, the starting point 
		   needs to be altered when reading the ADC. When converting the voltage to bits, 
		   0.5V is equal to 102 bits, therefore whatever the temperature is read at ADC 
		   is automatically 102 bits less and divided by 2 to represents the Celsius equivalent. 
		   The same code was used to display the data on the LCD. 
		*/
		temp = ADC_read(ADC_PIN1);			//Check temperature
		temp = (temp -102)/2;

		/* Convert Decimal to hex for LCD */
		LCD[3]=0;
		Dec_Hex(temp, LCD);
			
		/* LCD temp Display */
		i2c_start(0x50);					// Comm with LCD address
		_delay_ms(40);
		LCD_2line ();
		end= 0x3E;
		Lookup (TempD, end);
		i2c_write(LCD[2]);					// MSB number of data
		_delay_ms(10);
		i2c_write(LCD[1]);
		_delay_ms(10);
		i2c_write(LCD[0]);					// LSB number of data
		_delay_ms(1000);
		i2c_stop();

		/* Transmit Frame */
		_delay_ms(1);
		Frame(temp, tempADDRESS);
		//Transmit_Check (temp, tempADDRESS);
		_delay_ms(40);
		
		/* Blood Pressure 1 *******************************************************
		   Section 3 is used to measure the blood pressure on the right arm.  
		   Using the same base code as section 1, the pulse transit time is measured
		   by reading the time delay between sensor #1 and #2. Using the linear equation 
		   from figure 30, the blood pressure is estimated and sent to the display.
		*/
		Tcnt=0;									//Clear all variables each loop
		avgcnt = 0;
		do
		{
			pulse = ADC_read(ADC_PIN2);			//Check pulse
			if (pulse >  THOLD)					//If pulse is over threshold continue
			{	
				do
				{  pulse = ADC_read(ADC_PIN);	//This loop measured the time between sensor
					Pcnt++;
				} while (pulse >  THOLD);
			
				Tcnt++;
				avgcnt= avgcnt + Pcnt;			//Add period time to variable and clear
				Pcnt= 0;
			}
		} while (Tcnt < 11);					//Count period 10 times
		
		BP1= -0.2666*(avgcnt/10)+160;			//BP1 Calculations
			
		/* Convert Decimal to hex for LCD */ 
		LCD[3]=0;
		Dec_Hex(BP1, LCD);
		
		/* LCD temp Display */
		i2c_start(0x50);					// Comm with LCD address
		_delay_ms(40);
		LCD_Clear ();
		end= 0x3E;
		Lookup (BloodPD1, end);
		i2c_write(LCD[2]);					// MSB number of data
		_delay_ms(10);
		i2c_write(LCD[1]);
		_delay_ms(10);
		i2c_write(LCD[0]);					// LSB number of data
		_delay_ms(1000);
		i2c_stop();
		
		/* Transmit Frame */
		_delay_ms(1);
		Frame(BP1, BP1ADDRESS);
		//Transmit_Check (BP1, BP1ADDRESS);
		_delay_ms(40);
		
		/* Blood Pressure 2 *******************************************************
		   Section 4 is used to measure the blood presure in the left arm. 
		   Same explanation as section 3.  
		*/
		Tcnt=0;									//Clear all variables each loop
		avgcnt = 0;
		do
		{
			pulse = ADC_read(ADC_PIN2);			//Check pulse
			if (pulse >  THOLD)					//If pulse is over threshold continue
			{
				do
				{  pulse = ADC_read(ADC_PIN3);	//This loop measured the time between sensor
					Pcnt++;
				} while (pulse >  THOLD);
				
				Tcnt++;
				avgcnt= avgcnt + Pcnt;			//Add period time to variable and clear
				Pcnt= 0;
			}
		} while (Tcnt < 11);					//Count period 10 times
		
		BP2= -0.2666*(avgcnt/10)+140;			//BPM Calculations
		
		/* Convert Decimal to hex for LCD */
		LCD[3]=0;
		Dec_Hex(BP2, LCD);
		
		/* LCD temp Display */
		i2c_start(0x50);					// Comm with LCD address
		_delay_ms(40);
		LCD_2line ();
		end= 0x3E;
		Lookup (BloodPD2, end);
		i2c_write(LCD[2]);					// MSB number of data
		_delay_ms(10);
		i2c_write(LCD[1]);
		_delay_ms(10);
		i2c_write(LCD[0]);					// LSB number of data
		_delay_ms(1000);
		i2c_stop();
		
		/* Transmit Frame */
		_delay_ms(1);
		Frame(BP2, BP2ADDRESS);
		//Transmit_Check (BP2, BP2ADDRESS);
		_delay_ms(40);	
	}	
	return 0;
}
//**********************************************
//                FUNCTIONS 
//********************************************** Provided by maxembedded.com
/* In order to activate the internal ADC and MUX 
   situated inside the Atmega328P, it needs to be 
   initialized and told which channel to read. 
   This internal ADC and MUX is used to convert 
   the analog sensors to digital. The following code 
   demonstrates the initialization of the ADC and the MUX.  
*/
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
//********************************************** Simple lookup table to display text on LCD
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
/* In order to transmit frames to the home station, 
the USART on the Atmega328p needs to be initialized 
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

/* The TX function sends out a pulse when ever a 
frame has been transmitter and also waits for 
the buffer to be ready.
*/
void TX (char TX_Data)
{
	PORTD |= _BV(SYNC_P);			//Send out a pulse to syn with frame bytes
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

//********************************************** Frame 
/* In order to send out the data received from 
   the sensors to the home station, the data must 
   be inserted inside a frame that consist of 3 preambles, 
   a keyword, an address, the data and a checksum that 
   simple adds up the address and data. The following 
   codes demonstrate the process of sending out data. 
*/
void Frame (char data, char address)
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
/* The following code waits to receive a signal 
   from the home station if the checksum was calculated 
   to be incorrect. If so, the frame with be sent again. 
   If not, the code carries on. 
*/
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
		Frame(data, address);
		_delay_ms(40);
	}
	return ;
}