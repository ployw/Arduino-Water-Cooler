// CPE Final Project
// Group Name

//keep track of states
enum State
{
  Disabled,
  Idle,
  Error,
  Running
}

//Realtime Clock
#include <RTClib.h>
RTC_DS3231 rtc;

//LCD
#include <LiquidCrystal.h>
const int RS = 11, EN = 12, D4 = 2, D5 = 3, D6 = 4, D7 = 5;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

//UART
#define RDA 0x80
#define TBE 0x20  
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;

//buttons
//start
volatile unsigned char* port_k = (unsigned char*) 0x108; 
volatile unsigned char* ddr_k  = (unsigned char*) 0x107; 
volatile unsigned char* pin_k  = (unsigned char*) 0x106; 

//reset
volatile unsigned char* port_e = (unsigned char*) 0x2E; 
volatile unsigned char* ddr_e  = (unsigned char*) 0x2D; 
volatile unsigned char* pin_e  = (unsigned char*) 0x2C; 

//stop
volatile unsigned char* port_d = (unsigned char*) 0x2B; 
volatile unsigned char* ddr_d  = (unsigned char*) 0x2A; 
volatile unsigned char* pin_d  = (unsigned char*) 0x29; 

//TIMER
volatile unsigned char *myTCCR1A = (unsigned char *)0x80;
volatile unsigned char *myTCCR1B = (unsigned char *)0x81;
volatile unsigned char *myTCCR1C = (unsigned char *)0x82;
volatile unsigned char *myTIMSK1 = (unsigned char *)0x6F;
volatile unsigned int *myTCNT1 = (unsigned int *)0x84;
volatile unsigned char *myTIFR1 = (unsigned char *)0x36;

//current state
State currentState = Disabled;

//ISR
const byte interruptPin = 2;
volatile byte state = HIGH;

void setup()
{
  currentState = Disabled;

  attachInterrupt(digitalPinToInterrupt(interruptPin), startRunning, RISING);
  lcd.begin(16, 2); // set up number of columns and rows
  
  //initialize the serial port on USART0:
  U0init(9600);

  //set interruptPin to input
}

void loop()
{

  if(currentState != Disabled)
  {
    
  }
  
  switch(currentState)
  {
    case Disabled:
      //yellow led is on
      //no monitoring of water/temp
      //monitor start button using ISR

    break;
    case Idle:
      //record transition times
      //water level is monitored, change to ERROR state if low
      //green led is on

    break;
    case Error:
      //motor is off
      //red led
      //reset button triggers change to idle stage is water is above the threshold
      displayError();

    break;
    case Running:
      //motor is on
      //transition to idle if temp drops below threshold
      //transition to error if water is too low

    break;
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
  DateTime current = rtc.now();
  int year = current.year();
  int month = current.month();
  int day = current.day();
  int hour = current.hour();
  int min = current.minute();
  int sec = current.second();

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
    min / 10 + '0',
    min % 10 + '0',
    ':',
    sec / 10 + '0',
    sec % 10 + '0',
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

void startRunning()
{
  currentState = Running;
}
