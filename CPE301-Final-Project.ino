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

  //reseet
  volatile unsigned char* port_e = (unsigned char*) 0x2E; 
  volatile unsigned char* ddr_e  = (unsigned char*) 0x2D; 
  volatile unsigned char* pin_e  = (unsigned char*) 0x2C; 

  //stop
  volatile unsigned char* port_d = (unsigned char*) 0x2B; 
  volatile unsigned char* ddr_d  = (unsigned char*) 0x2A; 
  volatile unsigned char* pin_d  = (unsigned char*) 0x29; 

  //current state
  State currentState = Disabled;

void setup()
{
  lcd.begin(16, 2); // set up number of columns and rows
  
  // initialize the serial port on USART0:
  U0init(9600);

  //set PK2 to INPUT
  *ddr_k &= 0xEF;
}

void loop()
{

  if(currentState != Disabled)
  {
    
  }
  
  switch(currentState)
  {
    case Disabled:

    break;
    case Idle:

    break;
    case Error:
    displayError();

    break;
    case Running:

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
  lcd.setCursor(0, 1);
  lcd.print("ERROR");
}

void displayTempHumidity
{
  
}
