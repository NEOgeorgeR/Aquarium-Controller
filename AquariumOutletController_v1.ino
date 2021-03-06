
/*
AQUARIUM OUTLET CONTROLLER

  Sketch by George Roark, with great respect for the brilliant work of the hardware designers and authors of the included libraries!

  - Arduino Uno R3 or equivalent (for development, plan is to use a Nano for implementation)
  - DS3231 RTC module
  - 16x2 LCD with I2C backpack
  - 4 channel 5v relay module (optically isolated relays rated for 10A @110 V outputs)
  - DS18b20 waterproof temperature sensors
*/


#include <Time.h> // http://playground.arduino.cc/code/time
#include <TimeAlarms.h> // http://playground.arduino.cc/code/time
#include <Wire.h>  // https://www.arduino.cc/en/Reference/Wire
#include <DallasTemperature.h> // https://github.com/milesburton/Arduino-Temperature-Control-Library

#include <hd44780.h> // https://github.com/duinoWitchery/hd44780
#include <hd44780ioClass/hd44780_I2Cexp.h> // include i/o class header

hd44780_I2Cexp lcd; // declare lcd object: auto locate & config display for hd44780 chip

#include <DS3232RTC.h> //https://github.com/JChristensen/DS3232RTC

#define ONE_WIRE_BUS 2 /*-(Connect to Pin 2 )-*/
/* Set up a oneWire instance to communicate with any OneWire device*/
OneWire ourWire(ONE_WIRE_BUS);
/* Tell Dallas Temperature Library to use oneWire Library */
DallasTemperature sensors(&ourWire);

// Define the pins that control the relays
int relay1 = 8; 
int relay2 = 9;
int relay3 = 10;
int relay4 = 11;

int temp1;


void setup()
{
  lcd.begin(16, 2); // Initialize LCD with number of columns and rows
  
  Serial.begin(9600);

    setSyncProvider(RTC.get);   // Get the time from the RTC
    if(timeStatus() != timeSet) 
        Serial.println("Unable to sync with the RTC");
    else
        Serial.println("RTC has set the system time");
  
  Wire.begin();

  sensors.begin(); //Start up the DallasTemperature library

  // Assign relay output pins and establish initial state (LOW turns relay ON)
  pinMode(relay1, OUTPUT);
  digitalWrite(relay1, HIGH); // relay1 OFF
  pinMode(relay2, OUTPUT);
  digitalWrite(relay2, HIGH); // relay2 OFF
  pinMode(relay3, OUTPUT);
  digitalWrite(relay3, HIGH);  // relay3 OFF
  pinMode(relay4, OUTPUT);
  digitalWrite(relay4, HIGH);  // relay4 OFF

  // Create the time alarms (set temperature events in void loop())

  /*
   * TIMING EVENTS
   * 
   * Here's some example syntax:
   * Alarm.alarmRepeat(8,30,0, MorningAlarm2);             // Calls the MorningAlarm2 function at 8:30am every day
   * Alarm.alarmRepeat(17,45,0,EveningAlarm2);             // Calls the EveningAlarm2 function at5:45pm every day
   * Alarm.alarmRepeat(dowSaturday,8,30,30,WeeklyAlarm2);  // Calls WeeklyAlarm2 at 8:30:30 every Saturday
   * Alarm.timerRepeat(15, Repeats2);                      // Calls Repeats2 every 15 seconds
   * Alarm.timerOnce(10, OnceOnly2);                       // Calls OnceOnly2 once after 10 seconds
   * 
   * See the bottom of the sketch for the time event functions
   */

  /* relay1
      I'm using relay1 to turn the skimmer and refugium OFF during peak viewing times.
      Since the devices are ON most of the time, the corresponding outlet is wired via the NC (normally closed)
      terminals of the relay. As a result, turning the relay ON actually turns off the devices.
      This configuration saves power (the relay is not ON most of the time).
  */
  Alarm.alarmRepeat(6,0,0, Relay1Repeats);     // 6:00 am every day, turn relay1 OFF
  Alarm.alarmRepeat(8,0,0, Relay1Repeats);     // 8:00 am every day, turn relay1 ON
  Alarm.alarmRepeat(16,30,0, Relay1Repeats);   // 4:30 pm every day, turn relay1 OFF 
  Alarm.alarmRepeat(18,30,0, Relay1Repeats);   // 6:30 pm every day, turn relay1 ON

  
  // relay2: In this sketch relay2 is used for a temperature event

 
  /* relay3 
   *  Relay3 controls the outlet for the wavemaker. Since I have a nano tank, keeping the wavemaker on
   *  all of the time stresses the soft corals, even on low speed. So every 30 minutes they get a 30 minute break!
  */
  Alarm.timerRepeat(0,30,0, Relay3Repeats);     // Toggle relay3 every 30 minutes

 
  // relay4 (insert time events below (maybe a water sensor, another thermal sensor, another time event...)


  // Print a message to the LCD
  lcd.print("Ready!   ");
  Alarm.delay(1000); // Use Alarm.delay in place of delay in support of TimeAlarms
}

void  loop(){

  digitalClockDisplay();  
  Alarm.delay(100); 

  sensors.requestTemperatures(); // Send the command to get temperatures

  //    ...SET UP TEMPERATURE EVENTS HERE...
  
  temp1 = sensors.getTempFByIndex(0); 
  if (temp1 >= 81)
  {
    // Turn relay2 ON if the temperature is 81 degrees F or higher (external cooling fan)
    digitalWrite(relay2, LOW); 
    lcd.home();
    lcd.print("Temp. Event! "); // Display temperature event on first line of the LCD
    lcd.print(temp1);
    lcd.setCursor(11,1);
    lcd.print(temp1); // Display current temperature to the right of the time
    lcd.print(" F");
    
  }
  if (temp1 < 80)
  {
    // The temperature gap between ON and OFF is intentional (hysteresis)
    digitalWrite(relay2, HIGH);
    lcd.home();
    lcd.print("Event Monitor   ");
    lcd.setCursor(11,1);
    lcd.print(temp1); // Display current temperature to the right of the time
    lcd.print(" F");
  }
  else
  {
    lcd.setCursor(11,1); // Temperature is between the set points
    lcd.print(temp1); // Display current temperature to the right of the time
    lcd.print(" F");
  }

}

void digitalClockDisplay(void)
{
    // Display time in the time in second row of the LCD
    lcd.setCursor(0,1);
    lcd.print(hour());
    printDigits(minute());
    printDigits(second());
    //Serial.print(' ');
    //Serial.print(day());
    //Serial.print(' ');
    //Serial.print(month());
    //Serial.print(' ');
    //Serial.print(year()); 
    //Serial.println(); 
}

void printDigits(int digits)
{
    // utility function for digital clock display: prints preceding colon and leading 0
    lcd.print(':');
    if(digits < 10)
        lcd.print('0');
    lcd.print(digits);
}

// Functions to be called when an alarm triggers:

void Relay1Repeats(){

  lcd.home();
  lcd.print("Relay1 Toggle   ");  
  digitalWrite(relay1, !digitalRead(relay1)); //toggle state  
  Alarm.delay(1000); // Display event text for 1 second     
}


void Relay3Repeats(){

  lcd.home();
  lcd.print("Relay3 Toggle   ");  
  digitalWrite(relay3, !digitalRead(relay3)); //toggle state 
  Alarm.delay(1000); // Display event text for 1 second      
}

/*
void MorningAlarm1(){
  //Serial.println("Alarm: - turn lights off"); 
  lcd.home();
  lcd.print("MorningAlarm1   ");
}

void EveningAlarm1(){
  //Serial.println("Alarm: - turn lights on"); 
  lcd.home();
  lcd.print("EveningAlarm1   ");          
}

void WeeklyAlarm1(){
  //Serial.println("Alarm: - its Monday Morning");
  lcd.home();
  lcd.print("WeeklyAlarm1    ");        
}

void ExplicitAlarm1(){
  //Serial.println("Alarm: - this triggers only at the given date and time");
  lcd.home();
  lcd.print("ExplicitAlarm1  " );       
}

void OnceOnly1(){
  //Serial.println("This timer only triggers once");
  lcd.home();
  lcd.print("OnceOnly1       ");  
}
*/


