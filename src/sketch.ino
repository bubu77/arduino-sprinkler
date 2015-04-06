#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <avr/io.h>


#include <LiquidCrystal_I2C.h>
#include <LiquidCrystal.h>
#include <Wire.h> 
#include <LCD.h>

//STATUS LED, BUILT-IN
#define LED_PIN 		13

//how often we shoudl wake up? N ==> N*8s
#define SLEEP_SCALER 		1//8 for prod env

//moisture sensor pins
#define MOIST_PIN_POWER 	9
#define MOIST_PIN_SENS 		0

/****************************************************************
I2C Addresses
0x27	--> I2C_LCD module
****************************************************************/
#define I2C_LCD_ADDR 0x27



//I2C_LCD comm variable
LiquidCrystal_I2C lcd(I2C_LCD_ADDR, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address

//sleep variables
byte sleepCounter = 0;
byte counter=1;
bool canSleep = true;

//sensors reading variables
int moistSens=0;

//pump on Flag
bool pumpOn = false;

/***************************************************
 *  Name:        ISR(WDT_vect)
 *  Returns:     Nothing.
 *  Parameters:  None.
 *  Description: Watchdog Interrupt Service. This
 *               is executed when watchdog timed out.
 ***************************************************/
ISR(WDT_vect)
{
	
}


/***************************************************
 *  Name:        enterSleep
 *  Returns:     Nothing.
 *  Parameters:  None.
 *  Description: Enters the arduino into sleep mode.
 ***************************************************/
void enterSleep(void)
{
  	set_sleep_mode(SLEEP_MODE_PWR_DOWN);   
  	sleep_enable();
  
  	/* Now enter sleep mode. */
  	sleep_mode();
  
  	/* The program will continue from here after the WDT timeout*/
  	sleep_disable(); /* First thing to do is disable sleep. */
  
  	/* Re-enable the peripherals. */
  	power_all_enable();
}



/***************************************************
 *  Name:        setupWDT
 *  Returns:     Nothing.
 *  Parameters:  None.
 *  Description: Setup registers for sleep activities
 ***************************************************/
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



/***************************************************
 *  Name:        ping
 *  Returns:     Nothing.
 *  Parameters:  None.
 *  Description: Hearth beat blink
 ***************************************************/
void ping(){

	digitalWrite(LED_PIN, HIGH);
    	delay(100);
	digitalWrite(LED_PIN, LOW);
	delay(200);
	digitalWrite(LED_PIN, HIGH);
    	delay(100);
	digitalWrite(LED_PIN, LOW);
	delay(300);


}

/***************************************************
 *  Name:        setupLCD
 *  Returns:     Nothing.
 *  Parameters:  None.
 *  Description: setup lcd screen
 ***************************************************/
void setupLCD(){
	lcd.begin(16,2);   // initialize the lcd for 16 chars 2 lines, turn on backlight
	lcd.noBacklight();

}

/***************************************************
 *  Name:        setupSensors
 *  Returns:     Nothing.
 *  Parameters:  None.
 *  Description: setup sensors
 ***************************************************/
void setupSensors(){
	//moisture sensor
	pinMode(MOIST_PIN_POWER, OUTPUT);
        pinMode(MOIST_PIN_SENS, INPUT);
}

/***************************************************
 *  Name:        setupSystems
 *  Returns:     Nothing.
 *  Parameters:  None.
 *  Description: Setup different sub systems
 ***************************************************/
void setupSystems(){
	//Serial
	Serial.begin(9600);	

	//WDT
	setupWDT();

	//LCD
	setupLCD();

	//SENSORS
	setupSensors();
}


/***************************************************
 *  Name:        log
 *  Returns:     Nothing.
 *  Parameters:  msg to log.
 *  Description: logs to serial if available
 ***************************************************/
void log(char* msg){
	if(Serial){
		Serial.println(msg);
		delay(100);
	}
	//TODO?
/*
	else{Serial.begin(9600);}
*/
}

void setup()
{

	//LED Monitor
   	pinMode(LED_PIN,OUTPUT);
	
	//waits few secs to be programmed without issues
	digitalWrite(LED_PIN, HIGH);
	delay(5000);
	digitalWrite(LED_PIN,LOW);

	setupSystems();

	ping();
}

/***************************************************
 *  Name:        showStatus
 *  Returns:     Nothing.
 *  Parameters:  none.
 *  Description: print current status to lcd monitor
 ***************************************************/
void readSensors(){
	//MOIST
	//perform multiple reads and use avg
	int tests = 5;
	moistSens = 0;
        digitalWrite(MOIST_PIN_POWER, HIGH);
	delay(1000);
	for(int i=0;i<tests;i++){
		int curr = analogRead(MOIST_PIN_SENS);
		moistSens+= curr;
//		Serial.println(curr);
//		Serial.println(moistSens);
		delay(200);		
	}

//	Serial.println("---");
	moistSens=moistSens/tests;	        
//	Serial.println(moistSens);
//	Serial.println("---");

	digitalWrite(MOIST_PIN_POWER, LOW);

}
/***************************************************
 *  Name:        showStatus
 *  Returns:     Nothing.
 *  Parameters:  none.
 *  Description: print current status to lcd monitor
 ***************************************************/
void showStatus(){
		//moist R1[X0000]
		lcd.setCursor(0,0); 
  		lcd.print("H");
		lcd.setCursor(1,0);
		lcd.print(moistSens);


		//pump R2[P0]
		lcd.setCursor(0,1);
		lcd.print("P");
		lcd.setCursor(0,1);
//		lcd.print("-");
		if(pumpOn){
			lcd.blink();
		}else{
			lcd.noBlink();
		}

}


/***************************************************
 *  Name:        applyLogic
 *  Returns:     Nothing.
 *  Parameters:  none.
 *  Description: evaluate current sensor data and prepares
		 variables for actuators
 ***************************************************/
void applyLogic(){
	if(moistSens < 100){
		pumpOn = true;
	}else{
		pumpOn = false;
	}
}

void loop()
{
	canSleep = false;

	if(sleepCounter-- <1)	{
		//this code is the loop working part
		ping();

		readSensors();
		applyLogic();
		lcd.display();
		showStatus();


		//turn off display
//		lcd.noDisplay();
		//reset sleep counter
		sleepCounter=SLEEP_SCALER;
	}

	if(canSleep){ 
	    	enterSleep();
	}

}
