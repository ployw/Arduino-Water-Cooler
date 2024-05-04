// CPE Final Project
// Group Name

#define RDA 0x80
#define TBE 0x20  
#include <RTClib.h>
#include <LiquidCrystal.h>
#include <Stepper.h>
#include <Wire.h>
#include <dht.h>

//keep track of states
enum State
{
  Disabled,
  Idle,
  Error,
  Running
};

//Realtime Clock
RTC_DS1307 rtc;

//LCD
const int RS = 11, EN = 12, D4 = 2, D5 = 3, D6 = 4, D7 = 5;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

//UART
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;

//leds
volatile unsigned char *port_l = (unsigned char *)0x10B;
volatile unsigned char *ddr_l = (unsigned char *)0x10A;
volatile unsigned char *pin_l = (unsigned char *)0x109;

//buttons
volatile unsigned char *port_d = (unsigned char *)0x2B;
volatile unsigned char *ddr_d = (unsigned char *)0x2A;
volatile unsigned char *pin_d = (unsigned char *)0x29;
volatile unsigned char* port_e = (unsigned char*) 0x2E; 
volatile unsigned char* ddr_e  = (unsigned char*) 0x2D; 
volatile unsigned char* pin_e  = (unsigned char*) 0x2C;

//TIMER
volatile unsigned char *myTCCR1A = (unsigned char *)0x80;
volatile unsigned char *myTCCR1B = (unsigned char *)0x81;
volatile unsigned char *myTCCR1C = (unsigned char *)0x82;
volatile unsigned char *myTIMSK1 = (unsigned char *)0x6F;
volatile unsigned int *myTCNT1 = (unsigned int *)0x84;
volatile unsigned char *myTIFR1 = (unsigned char *)0x36;

//current state
State currentState = Disabled;
bool stateChanged = 0;

//ISR/INTERRUPT
volatile unsigned char *mySREG = (unsigned char *)0x5F;
volatile unsigned char *myEICRA = (unsigned char *)0x69;
volatile unsigned char *myEIMSK = (unsigned char *)0x3D;

//ADC
volatile unsigned char *my_ADMUX = (unsigned char *)0x7C;
volatile unsigned char *my_ADCSRB = (unsigned char *)0x7B;
volatile unsigned char *my_ADCSRA = (unsigned char *)0x7A;
volatile unsigned int *my_ADC_DATA = (unsigned int *)0x78;

//STEPPER
Stepper stepper(32, 22, 24, 23, 25);

//HUM/TEMP
dht DHT;


void setup()
{
  currentState = Disabled;

  Wire.begin();
  rtc.begin();

  lcd.begin(16, 2); // set up number of columns and rows
  
  //initialize the serial port on USART0:
  U0init(9600);
  adc_init();

  //LEDs to output
  *ddr_l |= 0b00001111; //pins 49 (yellow), 48 (blue), 47 (green), 46 (red)
  *port_l &= 0b11110001;
  *port_l |= 0b00000001;

  //set interruptPins to input
  *port_d |= 0b00001100;
  *port_e |= 0b00000100;
  *ddr_e &= 0b11111011; //pin 2 (reset)
  *ddr_d &= 0b11110011; //pins 19 (start), 18 (stop)
  attachInterrupt(digitalPinToInterrupt(19), startButtonISR, RISING);
  attachInterrupt(digitalPinToInterrupt(2), resetButtonISR, RISING);
  attachInterrupt(digitalPinToInterrupt(18), stopButtonISR, RISING);

  //interrupt
  *mySREG |= 0b10000000;
}

void loop()
{
  if(stateChanged)
  {
    printTime();
  }

  if(currentState != Disabled)
  {
    
  }
  
  switch(currentState)
  {
    case Disabled:
      stateChanged = 0;
      //Serial.println("DISABLED");
      //turn on yellow lED
      *port_l &= 0b11110001;
      *port_l |= 0b00000001;

      //no monitoring of water/temp
      //monitor start button using ISR

    break;
    case Idle:
      stateChanged = 0;
      //green led
      *port_l &= 0b11110100;
      *port_l |= 0b00000100;

      //record transition times
      //water level is monitored, change to ERROR state if low


    break;
    case Error:
      stateChanged = 0;
      //red led
      *port_l &= 0b11111000;
      *port_l |= 0b00001000;

      //motor is off

      //reset button triggers change to idle stage is water is above the threshold
      displayError();

    break;
    case Running:
      stateChanged = 0;
      //Serial.println("RUNNING");

      //blue led
      *port_l &= 0b11110010;
      *port_l |= 0b00000010;
      //motor is on
      //transition to idle if temp drops below threshold
      //transition to error if water is too low

    break;
  }

}

void startButtonISR()
{
  stateChanged = 1;
  currentState = Running;
}

void resetButtonISR()
{
  stateChanged = 1;
  currentState = Idle;
  //if water is above threshold, change to IDLE state
}

void stopButtonISR()
{
  if(currentState != Disabled)
  {
    stateChanged = 1;
    currentState = Disabled;
  }
}

// function to initialize USART0 to "int" Baud, 8 data bits,no parity, and one stop bit. Assume FCPU = 16MHz
void U0init(unsigned long U0baud)
{
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 // Same as (FCPU / (16 * U0baud)) - 1;
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}

// Read USART0 RDA status bit and return non-zero true if set
unsigned char U0kbhit()
{
  return (*myUCSR0A & RDA) != 0;
}

// Read input character from USART0 input buffer
unsigned char U0getchar()
{
  return *myUDR0;
}

// Wait for USART0 (myUCSR0A) TBE to be set then write character to transmit buffer
void U0putchar(unsigned char U0pdata)
{
  while(!(*myUCSR0A & TBE)){};
  *myUDR0 = U0pdata;
}

void displayError()
{
  lcd.setCursor(0, 0);
  lcd.print("ERROR");
}

void printTime()
{
  DateTime now = rtc.now();
  int year = now.year();
  int month = now.month();
  int day = now.day();
  int hour = now.hour();
  int minute = now.minute();
  int second = now.second();
  char time[22] = {
    month / 10 + '0',
    month % 10 + '0',
    '/',
    day / 10 + '0',
    day % 10 + '0',
    '/',
    (year / 1000) + '0',
    (year % 1000 / 100) + '0',
    (year % 100 / 10) + '0',
    (year % 10) + '0',
    ' ',
    'a',
    't',
    ' ',
    hour / 10 + '0',
    hour % 10 + '0',
    ':',
    minute / 10 + '0',
    minute % 10 + '0',
    ':',
    second / 10 + '0',
    second % 10 + '0',
  };

  for (int i = 0; i < 22; i++)
  {
    U0putchar(time[i]);
  }

  U0putchar('\n');
}

void my_delay(unsigned int freq) //takes frequency to make a delay
{
  // calc period
  double period = 1.0 / double(freq);
  // 50% duty cycle
  double half_period = period / 2.0f;
  // clock period def
  double clk_period = 0.0000000625;
  // calc ticks
  unsigned int ticks = half_period / clk_period;
  // stop the timer
  *myTCCR1B &= 0xF8;
  // set the counts
  *myTCNT1 = (unsigned int)(65536 - ticks);
  // start the timer
  *myTCCR1B |= 0b00000001;
  // wait for overflow
  while ((*myTIFR1 & 0x01) == 0)
    ; // 0b 0000 0000
  // stop the timer
  *myTCCR1B &= 0xF8; // 0b 0000 0000
  // reset TOV
  *myTIFR1 |= 0x01;
}

void adc_init() //initialize adc
{
  // setup the A register
  *my_ADCSRA |= 0b10000000; // set bit   7 to 1 to enable the ADC
  *my_ADCSRA &= 0b11011111; // clear bit 6 to 0 to disable the ADC trigger mode
  *my_ADCSRA &= 0b11110111; // clear bit 5 to 0 to disable the ADC interrupt
  *my_ADCSRA &= 0b11111000; // clear bit 0-2 to 0 to set prescaler selection to slow reading
  // setup the B register
  *my_ADCSRB &= 0b11110111; // clear bit 3 to 0 to reset the channel and gain bits
  *my_ADCSRB &= 0b11111000; // clear bit 2-0 to 0 to set free running mode
  // setup the MUX Register
  *my_ADMUX &= 0b01111111; // clear bit 7 to 0 for AVCC analog reference
  *my_ADMUX |= 0b01000000; // set bit   6 to 1 for AVCC analog reference
  *my_ADMUX &= 0b11011111; // clear bit 5 to 0 for right adjust result
  *my_ADMUX &= 0b11100000; // clear bit 4-0 to 0 to reset the channel and gain bits
}

unsigned int adc_read(unsigned char adc_channel_num) //adc read
{
  // clear the channel selection bits (MUX 4:0)
  *my_ADMUX &= 0b11100000;
  // clear the channel selection bits (MUX 5)
  *my_ADCSRB &= 0b11110111;
  // set the channel number
  if (adc_channel_num > 7)
  {
    // set the channel selection bits, but remove the most significant bit (bit 3)
    adc_channel_num -= 8;
    // set MUX bit 5
    *my_ADCSRB |= 0b00001000;
  }
  // set the channel selection bits
  *my_ADMUX += adc_channel_num;
  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0x40;
  // wait for the conversion to complete
  while ((*my_ADCSRA & 0x40) != 0)
    ;
  // return the result in the ADC data register
  return *my_ADC_DATA;
}
