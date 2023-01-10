////timePeriods
//WDTO_15MS
//WDTO_30MS
//WDTO_60MS
//WDTO_120MS
//WDTO_250MS
//WDTO_500MS
//WDTO_1S
//WDTO_2S
//WDTO_4S
//WDTO_8S


//#include <avr/wdt.h>
//wdt_enable(WDTO_8S);
//WDTCSR |= (1 << WDIE);/*To enable the “interrupt only” mode we have 
//						to change a bit in the Watchdog Timer Control Register (WDTCSR).
//						This will fire off  an interrupt when the WDT expires*/
//
//////To “catch” the interrupt, add the following routine to your code and do something within it.
//ISR (WDT_vect)
//{
///   // Add your own funky code here  (but do keep it short and sweet).
//}
//
///////////////////////////////////////////////////////////
//wdt_enable(timePeriod); //“interrupt and reset” mode
//WDTCSR |= (1 << WDIE);	// Watchdog Interrupt Enable
//WDTCSR |= (1 << WDE);	// Watchdog  System Reset Enable

//
#include <avr/wdt.h>

ISR (WDT_vect)
{
   // Add your own funky code here  (but do keep it short and sweet).
   Serial.println("ISR routine");
}

void setup() 
{
  //wdt_enable(WDTO_2S);
  //WDTCSR |= (1 << WDIE);
  cli();                              // disable interrupts

  MCUSR = 0;                          // reset status register flags

                                      // Put timer in interrupt-only mode:                                        
  WDTCSR |= 0b00011000;               // Set WDCE (5th from left) and WDE (4th from left) to enter config mode,
                                      // using bitwise OR assignment (leaves other bits unchanged).
  WDTCSR =  0b01000000 | 0b100001;    // set WDIE (interrupt enable...7th from left, on left side of bar)
                                      // clr WDE (reset enable...4th from left)
                                      // and set delay interval (right side of bar) to 8 seconds,
                                      // using bitwise OR operator.

  sei();     
  Serial.begin(9600);
  Serial.println("Reset!");
}
 
void loop() 
{
	
}
