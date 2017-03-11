
/*
  AQUARIUM OUTLET CONTROLLER

  Sketch by George Roark, with great respect for the contributions of the hardware designers and authors of the included libraries!
  Additional thanks to the following Arduino forum members: cattledog, BulldogLowell, and el_supremo!

  - Arduino Uno R3 or equivalent: $11 for an Uno, $3.50 for a Nano (Amazon)
  - DS3231 RTC module $4 (Amazon)
  - 16x2 LCD with I2C backpack $10 (Amazon)
  - 4 channel 5v relay module (optically isolated relays rated for 10A @110 V outputs) $7 (Amazon)
  - DS18b20 waterproof temperature sensor $2.50 (Amazon)
  - Momentary push button switch $2.50
  -HC-05 or HC-06 Bluetooth Module: $8 (Amazon)

  Rough hardware cost: $39
*/


#include <Time.h> // http://playground.arduino.cc/code/time
#include <TimeAlarms.h> // http://playground.arduino.cc/code/time
#include <Wire.h>  // https://www.arduino.cc/en/Reference/Wire
#include <DallasTemperature.h> // https://github.com/milesburton/Arduino-Temperature-Control-Library

#include <hd44780.h> // https://github.com/duinoWitchery/hd44780
#include <hd44780ioClass/hd44780_I2Cexp.h> // include i/o class header

hd44780_I2Cexp lcd; // declare lcd object: auto locate & config display for hd44780 chip

#include <DS3232RTC.h> //https://github.com/JChristensen/DS3232RTC

/* The One Wire Bus is used for the DS18b20 temperature probe.
  Set up a oneWire instance to communicate with any OneWire device.
  Tell Dallas Temperature Library to use oneWire Library*/
#define ONE_WIRE_BUS 2                                               // Connect one wire sensors to Pin 2
OneWire ourWire(ONE_WIRE_BUS);
DallasTemperature sensors(&ourWire);

AlarmID_t R1_ID1 = 1;                                               // Used to capture TimeAlarms ID tags
AlarmID_t R1_ID2 = 2;
AlarmID_t R1_ID3 = 3;
AlarmID_t R1_ID4 = 4;
AlarmID_t R3cycle_ID = 5;

int relay1 = 8;                                                     // Connect relay1 to pin 8
int relay2 = 9;
int relay3 = 10;
int relay4 = 11;

int tempHigh = 81;                                                  // Holds the high trigger temperature for relay3
int tempLow = 80;
int temp1;

// Strings objects used with the serial read function

String inputString = "";
//String command = "";
String value = "";
boolean stringComplete = false;

int ind1 = 0;
int ind2 = 0;
int ind3 = 0;
int ind4 = 0;
int ind5 = 0;
int ind6 = 0;
int ind7 = 0;
int ind8 = 0;
int ind9 = 0;
int ind10 = 0;
int ind11 = 0;
int ind12 = 0;

int hh1 = 0;
int mm1 = 0;
int ss1 = 0;
int hh2 = 0;
int mm2 = 0;
int ss2 = 0;
int hh3 = 0;
int mm3 = 0;
int ss3 = 0;
int hh4 = 0;
int mm4 = 0;
int ss4 = 0;

int R3hh = 0;
int R3mm = 30;
int R3ss = 0;


/* Push button reading variables */

const byte buttonPin = 5;                                           // Connect the push button to pin 5
byte oldButtonState = HIGH;                                         // Assume switch open because of pull-up resistor
const unsigned long debounceTime = 10;                              // Debounce time in milliseconds
unsigned long buttonPressTime;                                      // when the switch last changed state
boolean buttonPressed = 0;                                          // a flag variable




void setup()
{
  lcd.begin(16, 2);                                                // Initialize LCD with number of columns and rows

  Serial.begin(9600);

  setSyncProvider(RTC.get);                                        // Get the time from the RTC
  if (timeStatus() != timeSet)
    Serial.println("Unable to sync with the RTC");
  else
    Serial.println("RTC has set the system time");

  Wire.begin();

  sensors.begin();                                                //Start up the DallasTemperature library


  inputString.reserve(50);
  //  command.reserve(50);
  value.reserve(50);


  /*Assign relay output pins and establish initial state (LOW turns a relay ON) */

  /* relay1
         I'm using relay1 to turn the skimmer and refugium OFF during peak viewing times.
         Since the devices are ON most of the time, the corresponding outlet is wired via the NC (normally closed)
         terminals of the relay. As a result, turning the relay ON actually turns off the devices.
         This configuration saves power (the relay is not ON most of the time). */

  pinMode(relay1, OUTPUT);
  digitalWrite(relay1, HIGH);         // relay1 OFF

  /* relay2: In this sketch relay2 is used for a temperature event so it doesn't use the alarm feature. */

  pinMode(relay2, OUTPUT);
  digitalWrite(relay2, HIGH);         // relay2 OFF

  /*relay3
      Relay3 controls the outlet for the wavemaker. Since I have a nano tank, keeping the wavemaker on
      all of the time stresses the soft corals, even at low speed. So I setup an ON/OFF cycle. */

  pinMode(relay3, OUTPUT);
  digitalWrite(relay3, HIGH);         // relay3 OFF

  /* relay4: In this sketch relay4 is triggered by a button press (to turn off pumps for feeding time)
     so it doesn't use the alarm feature.*/

  pinMode(relay4, OUTPUT);
  digitalWrite(relay4, HIGH);         // relay4 OFF

  pinMode (buttonPin, INPUT_PULLUP);  // Set up the button pin with a pullup resistor


  lcd.print("Ready!   ");             // Setup is complete
  Alarm.delay(1000);                  // Use Alarm.delay in place of delay in support of TimeAlarms
}




void  loop()
{

  digitalClockDisplay();                      // Print the current time at the beginning of the LCD second row
  Alarm.delay(250);

  sensors.requestTemperatures();              // Send the command to get temperatures

  temp1 = sensors.getTempFByIndex(0);
  if (temp1 >= tempHigh)
  {
    /* Turn relay2 ON if the temperature is 81 degrees F or higher (external cooling fan) */
    digitalWrite(relay2, LOW);
    lcd.home();
    lcd.print("Temp. Event! ");              // Display temperature event on first line of the LCD
    lcd.print(temp1);
    lcd.setCursor(11, 1);
    lcd.print(temp1);                        // Display current temperature to the right of the time
    lcd.print(" F");

  }
  if (temp1 < tempLow)
  {
    /* The temperature gap between ON and OFF is intentional (hysteresis) */
    digitalWrite(relay2, HIGH);
    lcd.home();
    lcd.print("Event Monitor   ");
    lcd.setCursor(11, 1);
    lcd.print(temp1);                        // Display current temperature to the right of the time
    lcd.print(" F");
  }
  else
  {
    lcd.setCursor(11, 1);                   // Temperature is between the set points
    lcd.print(temp1);                       // Display current temperature to the right of the time
    lcd.print(" F");
  }

  /* Button reading with non-delay() debounce - thank you Nick Gammon */
  byte buttonState = digitalRead (buttonPin);
  if (buttonState != oldButtonState) {
    if (millis () - buttonPressTime >= debounceTime) {
      buttonPressTime = millis ();
      oldButtonState =  buttonState;
      if (buttonState == LOW) {
        lcd.home();
        lcd.print("Button closed   ");      // Display that a button event has occured
        buttonPressed = 1;
        FeedingTime();                      // Call the feedingTime function
      }
      else {
        buttonPressed = 0;
      }
    }                                       // end if debounce time up
  }




  while (Serial.available() > 0 && stringComplete == false) {
    char inChar = (char)Serial.read();                        // Get the new byte
    inputString += inChar;                                    // Add byte to the inputString

    //If the incoming character is a newline or a carriage return, set a flag for the main loop

    if (inChar == '\n' || inChar == '\r') {
      Serial.print(F("Input string = "));
      Serial.println(inputString);

      stringComplete = true;

      boolean stringOK = false;

      //      Serial.println(F("inputString has been read!"));


      if (inputString.startsWith("Relay1")) {                            // Is this a list of settings for Relay1?
        //        Serial.println(F("Relay1 text detected"));
        processRelay1();
      }
      else if (inputString.startsWith("Relay2")) {                     // Is this a list of settings for Relay2?
        //        Serial.println(F("Relay2 text detected"));
        processRelay2();
      }
      else if (inputString.startsWith("Relay3")) {                     // Is this a list of settings for Relay3?
        //        Serial.println(F("Relay3 text detected"));
        processRelay3();
      }
    }
  }
}  // loop



void digitalClockDisplay(void)
{
  /* Display time in the time in second row of the LCD  */

  char currentTime[9];
  snprintf(currentTime, sizeof(currentTime), "%02d:%02d:%02d", hour(), minute(), second());
  lcd.setCursor(0, 1);
  lcd.print(currentTime);
}



void processRelay1(void)
{
  inputString = inputString.substring(6);                             // Remove "Relay1" from inputString
  int pos = inputString.indexOf('=');                                 // Set pos to the location of "=" in the string
  if (pos > -1) {
    value = inputString.substring(pos + 1, inputString.length() - 1); // Extract alarm values up to \n exluded


    //    Serial.print(F("Process the Relay1 value="));
    //    Serial.println(value);


    ind1 = value.indexOf(',');                                        // Location of first comma
    //    Serial.print(F("hh1 = "));
    hh1 = atoi(value.substring(0, ind1).c_str());                     // User selected alarm hours
    //    Serial.println(hh1);

    ind2 = value.indexOf(',', ind1 + 1);                             // Location of the second comma
    mm1 = atoi(value.substring(ind1 + 1, ind2 + 1).c_str());         // User selected alarm minutes
    //    Serial.print(F("mm1 = "));
    //    Serial.println(mm1);

    ind3 = value.indexOf(',', ind2 + 1);
    ss1 = atoi(value.substring(ind2 + 1, ind3 + 1).c_str());         // User selected alarm seconds
    //    Serial.print(F("ss1 = "));
    //    Serial.println(ss1);

    ind4 = value.indexOf(',', ind3 + 1);
    hh2 = atoi(value.substring(ind3 + 1, ind4 + 1).c_str());
    //    Serial.print(F("hh2 = "));
    //    Serial.println(hh2);

    ind5 = value.indexOf(',', ind4 + 1);
    mm2 = atoi(value.substring(ind4 + 1, ind5 + 1).c_str());
    //    Serial.print(F("mm2 = "));
    //    Serial.println(mm2);

    ind6 = value.indexOf(',', ind5 + 1);
    ss2 = atoi(value.substring(ind5 + 1, ind6 + 1).c_str());
    //    Serial.print(F("ss2 = "));
    //    Serial.println(ss2);

    ind7 = value.indexOf(',', ind6 + 1);
    hh3 = atoi(value.substring(ind6 + 1, ind7 + 1).c_str());
    //    Serial.print(F("hh3 = "));
    //    Serial.println(hh3);

    ind8 = value.indexOf(',', ind7 + 1);
    mm3 = atoi(value.substring(ind7 + 1, ind8 + 1).c_str());
    //    Serial.print(F("mm3 = "));
    //    Serial.println(mm3);

    ind9 = value.indexOf(',', ind8 + 1);
    ss3 = atoi(value.substring(ind8 + 1, ind9 + 1).c_str());
    //    Serial.print(F("ss3 = "));
    //    Serial.println(ss3);

    ind10 = value.indexOf(',', ind9 + 1);
    hh4 = atoi(value.substring(ind9 + 1, ind10 + 1).c_str());
    //    Serial.print(F("hh4 = "));
    //    Serial.println(hh4);

    ind11 = value.indexOf(',', ind10 + 1);
    mm4 = atoi(value.substring(ind10 + 1, ind11 + 1).c_str());
    //    Serial.print(F("mm4 = "));
    //    Serial.println(mm4);

    ind12 = value.indexOf(',', ind11 + 1);
    ss4 = atoi(value.substring(ind11 + 1, ind12 + 1).c_str());
    //    Serial.print(F("ss4 = "));
    //    Serial.println(ss4);


    Alarm.disable(R1_ID1);
    //    Serial.print(F("BT R1_ID1 = "));
    //    Serial.println(R1_ID1);
    Alarm.free(R1_ID1);

    Alarm.disable(R1_ID2);
    //    Serial.print(F("BT R1_ID2 = "));
    //    Serial.println(R1_ID2);
    Alarm.free(R1_ID2);

    Alarm.disable(R1_ID3);
    //    Serial.print(F("BT R1_ID3 = "));
    //    Serial.println(R1_ID3);
    Alarm.free(R1_ID3);

    Alarm.disable(R1_ID4);
    //    Serial.print(F("BT R1_ID4 = "));
    //    Serial.println(R1_ID4);
    Alarm.free(R1_ID4);


    R1_ID1 = Alarm.alarmRepeat(hh1, mm1, ss1, Relay1Repeats);            // Toggle Relay1 at user requested time
    Serial.println(F("New Relay1-1 Alarm set"));
    //    Serial.print(F("New R1_ID1 = "));
    //    Serial.println(R1_ID1);

    R1_ID2 = Alarm.alarmRepeat(hh2, mm2, ss2, Relay1Repeats);            // Toggle Relay1 at user requested time
    Serial.println(F("New Relay1-2 Alarm set"));
    //    Serial.print(F("New R1_ID2 = "));
    //    Serial.println(R1_ID2);

    R1_ID3 = Alarm.alarmRepeat(hh3, mm3, ss3, Relay1Repeats);            // Toggle Relay1 at user requested time
    Serial.println(F("New Relay1-3 Alarm set"));
    //    Serial.print(F("New R1_ID3 = "));
    //    Serial.println(R1_ID3);

    R1_ID4 = Alarm.alarmRepeat(hh4, mm4, ss4, Relay1Repeats);            // Toggle Relay1 at user requested time
    Serial.println(F("New Relay1-4 Alarm set"));
    //    Serial.print(F("New R1_ID4 = "));
    //    Serial.println(R1_ID4);

    inputString = "";
    stringComplete = false;

  }
}


void processRelay2(void)
{
  inputString = inputString.substring(6);                          // Remove "Relay2" from inputString
  int pos = inputString.indexOf('=');                              // Set pos to the location of "=" in the string
  if (pos > -1) {
    value = inputString.substring(pos + 1, inputString.length() - 1); // Extract alarm values up to \n exluded


    //    Serial.print(F("Process the Relay2 value="));
    //    Serial.println(value);


    ind1 = value.indexOf(',');                                     // Location of first comma
    tempHigh = atoi(value.substring(0, ind1).c_str());             // User selected high temperature
    Serial.print(F("tempHigh = "));
    Serial.println(tempHigh);

    ind2 = value.indexOf(',', ind1 + 1);                           // Location of the second comma
    tempLow = atoi(value.substring(ind1 + 1, ind2 + 1).c_str());   // User selected low temperature
    Serial.print(F("tempLow = "));
    Serial.println(tempLow);

    inputString = "";
    stringComplete = false;
  }
}

void processRelay3(void)
{
  inputString = inputString.substring(6);                          // Remove "Relay3" from inputString
  int pos = inputString.indexOf('=');                              // Set pos to the location of "=" in the string
  if (pos > -1) {
    value = inputString.substring(pos + 1, inputString.length() - 1); // Extract alarm values up to \n exluded


    //    Serial.print(F("Process the Relay3 value="));
    //    Serial.println(value);


    ind1 = value.indexOf(',');                                     // Location of first comma
    R3hh = atoi(value.substring(0, ind1).c_str());                // User selected cycle period
    //    Serial.print(F("R3hh = "));
    //    Serial.println(R3hh);

    ind2 = value.indexOf(',', ind1 + 1);                           // Location of the second comma
    R3mm = atoi(value.substring(ind1 + 1, ind2 + 1).c_str());        // User selected alarm minutes
    //    Serial.print(F("R3mm = "));
    //    Serial.println(R3mm);

    ind3 = value.indexOf(',', ind2 + 1);
    R3ss = atoi(value.substring(ind2 + 1, ind3 + 1).c_str());        // User selected alarm seconds
    //    Serial.print(F("R3ss = "));
    //    Serial.println(R3ss);

    Alarm.disable(R3cycle_ID);
    //    Serial.print(F("BT R3cycle_ID = "));
    //    Serial.println(R3cycle_ID);
    Alarm.free(R3cycle_ID);


    R3cycle_ID = Alarm.timerRepeat(R3hh, R3mm, R3ss, Relay3Repeats);
    Serial.println(F("New Relay3 Alarm set"));
    //    Serial.print(F("New R3cycle_ID = "));
    //    Serial.println(R3cycle_ID);

    inputString = "";
    stringComplete = false;
  }
}





/* Functions to be called when an alarm triggers: */

void Relay1Repeats()
{
  //  Serial.print(F("Relay1Repeats R1_ID1 = "));
  //  Serial.println(R1_ID1);

  //  Serial.print(F("Relay1Repeats R1_ID2 = "));
  //  Serial.println(R1_ID2);

  //  Serial.println(inputString);                                       // Print the received input string to Bluetooth
  lcd.home();
  lcd.print("Skimmer Toggle  ");

  digitalWrite(relay1, !digitalRead(relay1));                             // Toggle state of the skimmer power

  inputString = "";
  stringComplete = false;
}

void Relay3Repeats()
{

  //  Serial.print(F("Relay3Repeats R3cycle_ID = "));
  //  Serial.println(R3cycle_ID);

  lcd.home();
  lcd.print("Wavemaker Toggle");
  digitalWrite(relay3, !digitalRead(relay3)); //toggle state of the wavemaker power

  inputString = "";
  stringComplete = false;
}


void OnceOnly()
{
  digitalWrite(relay4, HIGH); // Turn the pumps back ON after feeding
}


void FeedingTime()
{

  Alarm.timerOnce(0, 10, 0, OnceOnly);        // Calls OnceOnly once after 10 minutes
  lcd.home();
  lcd.print("Feeding Time!   ");
  Alarm.delay(1000);                          // Display event text for 1 second

  // Outlet is connected to the NC relay terminals, so turning the relay ON turns off outlet power
  digitalWrite(relay4, LOW);

  // I feed a small frozen mysis parcel, which floats at the top of the water.
  // The wavemaker gently circulates the mysis so all fish have a chance to eat.
  digitalWrite(relay3, LOW); // Turn the wavemaker ON for feeding

}






