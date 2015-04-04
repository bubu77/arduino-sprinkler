#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <avr/io.h>


#define LED_PIN 13


volatile int f_wdt=1;
/***************************************************
 *  Name:        ISR(WDT_vect)
 *
 *  Returns:     Nothing.
 *
 *  Parameters:  None.
 *
 *  Description: Watchdog Interrupt Service. This
 *               is executed when watchdog timed out.
 *
 ***************************************************/
ISR(WDT_vect)
{
	if(f_wdt == 0)
  	{
    	f_wdt=1;
  	}
  	else
  	{
    	Serial.println("WDT Overrun!!!");
  	}
}


/***************************************************
 *  Name:        enterSleep
 *
 *  Returns:     Nothing.
 *
 *  Parameters:  None.
 *
 *  Description: Enters the arduino into sleep mode.
 *
 ***************************************************/
void enterSleep(void)
{
  	set_sleep_mode(SLEEP_MODE_PWR_SAVE);   /* EDIT: could also use SLEEP_MODE_PWR_DOWN for lowest power consumption. */
  	sleep_enable();
  
  	/* Now enter sleep mode. */
  	sleep_mode();
  
  	/* The program will continue from here after the WDT timeout*/
  	sleep_disable(); /* First thing to do is disable sleep. */
  
  	/* Re-enable the peripherals. */
  	power_all_enable();
}

void setupWDT(){
	/*** Setup the WDT ***/
  
  	/* Clear the reset flag. */
  	MCUSR &= ~(1<<WDRF);
  
  	/* In order to change WDE or the prescaler, we need to
   	 * set WDCE (This will allow updates for 4 clock cycles).
   	 */
  	WDTCSR |= (1<<WDCE) | (1<<WDE);

  	/* set new watchdog timeout prescaler value */
  	WDTCSR = 1<<WDP0 | 1<<WDP3; /* 8.0 seconds */
  
  	/* Enable the WD interrupt (note no reset). */
  	WDTCSR |= _BV(WDIE);
}

void ping(){

	for(int i=0;i<5;i++){
		digitalWrite(LED_PIN, HIGH);
    	delay(300);
    	digitalWrite(LED_PIN, LOW);
		delay(200);
	}

}

void setup()
{
	Serial.begin(9600);	
   	pinMode(LED_PIN,OUTPUT);      // set pin 13 as an output so we can use LED to monitor

	Serial.println("setup start");
	delay(100);

	setupWDT();

	Serial.println("setup done");
	delay(100);
}

void loop()
{
	Serial.println("loop start");
	
	if(f_wdt == 1)
  	{
    	/* Toggle the LED */
   		ping();
 
    	/* Don't forget to clear the flag. */
    	f_wdt = 0;
    
    	/* Re-enter sleep mode. */
    	enterSleep();
  	}
  	else
  	{
    	/* Do nothing. */
  	}
	
	Serial.println("loop end");
}