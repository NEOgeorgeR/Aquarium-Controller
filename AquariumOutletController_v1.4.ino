/*
  AQUARIUM OUTLET CONTROLLER

  Sketch by George Roark, with great respect for the contributions of the hardware designers and authors of the included libraries!
  Additional thanks to the following Arduino forum members: cattledog, BulldogLowell, el_supremo and septillion!

  - Arduino Uno R3 or equivalent: $11 for an Uno, $3.50 for a Nano (Amazon)
  - DS3231 RTC module $4 (Amazon)
  - 16x2 LCD with I2C backpack $10 (Amazon)
  - 4 channel 5v relay module (optically isolated relays rated for 10A @110 V outputs) $7 (Amazon)
  - DS18b20 waterproof temperature sensor $2.50 (Amazon)
  - Momentary push button switch $2.50
  -HC-05 or HC-06 Bluetooth Module: $8 (Amazon)

  Rough hardware cost: $39

  For more details on how to build this controller: http://www.instructables.com/id/Aquarium-Outlet-Controller-With-Time-Temp-Events/
*/


#include <Time.h>                                                    // http://playground.arduino.cc/code/time
#include <TimeAlarms.h>                                              // http://playground.arduino.cc/code/time
#include <Wire.h>                                                    // https://www.arduino.cc/en/Reference/Wire

#include <SoftwareSerial.h>                                          // Bluetooth will use mySerial to avoid conflicts with USB serial
SoftwareSerial mySerial(3, 4);                                       // RX, TX

#include <DallasTemperature.h>                                       // https://github.com/milesburton/Arduino-Temperature-Control-Library

#include <EEPROM.h>

#include <hd44780.h>                                                 // https://github.com/duinoWitchery/hd44780
#include <hd44780ioClass/hd44780_I2Cexp.h>                           // Include i/o class header

hd44780_I2Cexp lcd;                                                  // Declare lcd object: auto locate & config display for hd44780 chip

#include <DS3232RTC.h>                                               //https://github.com/JChristensen/DS3232RTC

/* The One Wire Bus is used for the DS18b20 temperature probe.
  Set up a oneWire instance to communicate with any OneWire device.
  Tell Dallas Temperature Library to use oneWire Library*/
#define ONE_WIRE_BUS 2                                               // Connect one wire sensors to Pin 2
OneWire ourWire(ONE_WIRE_BUS);
DallasTemperature sensors(&ourWire);

AlarmID_t R1_ID1 = 1;                                                // Used to capture TimeAlarms ID tags so alarms can be disabled
AlarmID_t R1_ID2 = 2;
AlarmID_t R1_ID3 = 3;
AlarmID_t R1_ID4 = 4;
AlarmID_t R3cycle_ID = 5;

int relay1 = 8;                                                     // Connect relay1 to pin 8
int relay2 = 9;                                                     // Connect relay2 to pin 9
int relay3 = 10;                                                    // Connect relay3 to pin 10
int relay4 = 11;                                                    // Connect relay4 to pin 11

int temp1;

String inputString = "";                                            // String objects used with the serial read function
String value = "";
boolean stringComplete = false;

byte ind1 = 0;                                                      // Used to parse the user input string
byte ind2 = 0;
byte ind3 = 0;
byte ind4 = 0;
byte ind5 = 0;
byte ind6 = 0;
byte ind7 = 0;
byte ind8 = 0;
byte ind9 = 0;
byte ind10 = 0;
byte ind11 = 0;
byte ind12 = 0;

int user_values [18];                                              // This array holds the user specified alarm and temperature values
int Array_location = 0;

int EEPROM_addr = 0;

char currentTime[9];

const byte buttonPin = 5;                                          // Connect the push button to pin 5
byte oldButtonState = HIGH;                                        // Assume switch open because of pull-up resistor
const unsigned long debounceTime = 10;                             // Debounce time in milliseconds
unsigned long buttonPressTime;                                     // when the switch last changed state
boolean buttonPressed = 0;                                         // a flag variable




void setup()
{
  lcd.begin(16, 2);                                                // Initialize LCD with number of columns and rows

  Serial.begin(9600);
  mySerial.begin(9600);

  setSyncProvider(RTC.get);                                        // Get the time from the RTC
  if (timeStatus() != timeSet)
    mySerial.println(F("Unable to sync with the RTC"));
  else
    mySerial.println(F("RTC has set the system time"));

  Wire.begin();

  sensors.begin();                                                //Start up the DallasTemperature library

  inputString.reserve(50);
  value.reserve(50);

  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

  /*Assign relay output pins and establish initial state (LOW turns a relay ON) */

  /* relay1
         I use relay1 to turn the skimmer and refugium OFF during peak viewing times.
         Since the devices are ON most of the time, the corresponding outlet is wired via the NC (normally closed)
         terminals of the relay. As a result, turning the relay ON actually turns off the devices.
         This configuration saves power (the relay is not ON most of the time). */

  pinMode(relay1, OUTPUT);
  digitalWrite(relay1, HIGH);                                    // Turn relay1 OFF

  /* relay2: In this sketch relay2 is used for a temperature event so it doesn't use the alarm feature. */

  pinMode(relay2, OUTPUT);
  digitalWrite(relay2, HIGH);                                   // Turn relay2 OFF

  /*relay3
      Relay3 controls the outlet for the wavemaker. Since I have a nano tank, keeping the wavemaker on
      all of the time stresses the soft corals, even at low speed. So I setup an ON/OFF cycle to give the
      critters a respit. */

  pinMode(relay3, OUTPUT);
  digitalWrite(relay3, HIGH);                                   // Turn off relay3 OFF

  /* relay4: In this sketch relay4 is triggered by a button press (to turn off pumps for feeding time)
     so it doesn't use the alarm feature. Again, this is wired using the NC relay terminal.*/

  pinMode(relay4, OUTPUT);
  digitalWrite(relay4, HIGH);                                   // Turn relay4 OFF

  pinMode (buttonPin, INPUT_PULLUP);                            // Set up the button pin with a pullup resistor


  lcd.print(F("Ready!   "));                                       // Setup is complete
  mySerial.println(F("Ready!"));


  int i = 0;

  for (i = 0; i <= 18; i++) {                                   // Restore the user alarm and temperature values from EEPROM
    user_values[Array_location] = EEPROM.read(EEPROM_addr);
    Serial.print(F("Array_location = "));
    Serial.print(Array_location);
    Serial.print(F(": User_values = "));
    Serial.println(user_values[Array_location]);
    EEPROM_addr++;
    Array_location++;
  }

  EEPROM_addr = 0;
  Array_location = 0;

  // Restore alarms per user settings in EEPROM

  if (user_values[17] == 88) {                                   // If Relay 1 values are stored in EEPROM, restore the alarms

    R1_ID1 = Alarm.alarmRepeat(user_values[0], user_values[1], user_values[2], Relay1Repeats);
    mySerial.println(F("Relay1-ON1 Alarm set"));

    R1_ID2 = Alarm.alarmRepeat(user_values[3], user_values[4], user_values[5], Relay1Repeats);
    mySerial.println(F("Relay1-OFF1 Alarm set"));

    R1_ID3 = Alarm.alarmRepeat(user_values[6], user_values[7], user_values[8], Relay1Repeats);
    mySerial.println(F("Relay1-ON2 Alarm set"));

    R1_ID4 = Alarm.alarmRepeat(user_values[9], user_values[10], user_values[11], Relay1Repeats);
    mySerial.println(F("Relay1-OFF2 Alarm set"));
  }


  if (user_values[18] == 88) {                                  // If Relay 3 values are stored in EEPROM, restore the alarm
    R3cycle_ID = Alarm.timerRepeat(user_values[14], user_values[15], user_values[16], Relay3Repeats);
  }
}




void  loop()
{
  digitalClockDisplay();                                      // Print the current time at the beginning of the LCD second row
  Alarm.delay(250);

  sensors.requestTemperatures();                              // Send the command to get temperatures

  temp1 = sensors.getTempFByIndex(0);
  if (temp1 >= user_values[12])                              // Turn relay2 ON if the Set Temperature is reached
  {
    digitalWrite(relay2, LOW);
    lcd.home();
    lcd.print(F("Temp. Event! "));                           // Display temperature event on first line of the LCD
    lcd.print(temp1);
    lcd.setCursor(11, 1);
    lcd.print(temp1);                                        // Display current temperature to the right of the time
    lcd.print(" F");
    mySerial.print(F("Temperature Event! "));
    mySerial.print(temp1);
    mySerial.println(F(" F"));

  }
  if (temp1 < user_values[13])                               // Turn relay2 OFF when this Set Temperature is reached
  {
    /* The temperature gap between ON and OFF is intentional (hysteresis) */
    digitalWrite(relay2, HIGH);
    lcd.home();
    lcd.print("Event Monitor   ");
    lcd.setCursor(11, 1);
    lcd.print(temp1);                                        // Display current temperature to the right of the time
    lcd.print(" F");
  }
  else
  {
    lcd.setCursor(11, 1);                                    // Temperature is between the set points
    lcd.print(temp1);                                        // Display current temperature to the right of the time
    lcd.print(" F");
    mySerial.print(F("Temperature Event! "));
    mySerial.print(temp1);
    mySerial.println(F(" F"));
  }

  /* Button reading with non-delay() debounce - thank you Nick Gammon */
  byte buttonState = digitalRead (buttonPin);
  if (buttonState != oldButtonState) {
    if (millis () - buttonPressTime >= debounceTime) {
      buttonPressTime = millis ();
      oldButtonState =  buttonState;
      if (buttonState == LOW) {
//        lcd.home();
//        lcd.print("Button closed   ");                      // Display that a button event has occured
        buttonPressed = 1;
        FeedingTime();                                      // Call the FeedingTime function
      }
      else {
        buttonPressed = 0;
      }
    }                                                       // end if debounce time up
  }




  while (mySerial.available() > 0 && stringComplete == false) {
    char inChar = (char)mySerial.read();                                // Get the new byte
    inputString += inChar;                                              // Add byte to the inputString

    // If the incoming character is a newline or a carriage return, set a flag for the main loop

    if (inChar == '\n' || inChar == '\r') {
      mySerial.print(F("Input string = "));
      mySerial.println(inputString);

      stringComplete = true;

      boolean stringOK = false;

      if (inputString.startsWith("Relay1")) {                           // Is this a list of settings for Relay1?
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

  snprintf(currentTime, sizeof(currentTime), "%02d:%02d:%02d", hour(), minute(), second());
  lcd.setCursor(0, 1);
  lcd.print(currentTime);
}



void processRelay1(void)
{
  inputString = inputString.substring(6);                                  // Remove "Relay1" from inputString
  int pos = inputString.indexOf('=');                                      // Set pos to the location of "=" in the string
  if (pos > -1) {
    value = inputString.substring(pos + 1, inputString.length() - 1);      // Extract alarm values up to \n exluded

    Array_location = 0;
    EEPROM_addr = 0;

    ind1 = value.indexOf(',');                                             // Location of first comma
    user_values[Array_location] = atoi(value.substring(0, ind1).c_str());  // User selected alarm hours
    EEPROM.update(EEPROM_addr, user_values[Array_location]);               // Save value in EEPROM
    Array_location++;
    EEPROM_addr++;

    ind2 = value.indexOf(',', ind1 + 1);                                   // Location of the second comma
    user_values[Array_location] = atoi(value.substring(ind1 + 1, ind2 + 1).c_str());         // User selected alarm minutes
    EEPROM.update(EEPROM_addr, user_values[Array_location]);
    Array_location++;
    EEPROM_addr++;

    ind3 = value.indexOf(',', ind2 + 1);
    user_values[Array_location] = atoi(value.substring(ind2 + 1, ind3 + 1).c_str());         // User selected alarm seconds
    EEPROM.update(EEPROM_addr, user_values[Array_location]);
    Array_location++;
    EEPROM_addr++;

    ind4 = value.indexOf(',', ind3 + 1);
    user_values[Array_location] = atoi(value.substring(ind3 + 1, ind4 + 1).c_str());
    EEPROM.update(EEPROM_addr, user_values[Array_location]);
    Array_location++;
    EEPROM_addr++;

    ind5 = value.indexOf(',', ind4 + 1);
    user_values[Array_location] = atoi(value.substring(ind4 + 1, ind5 + 1).c_str());
    EEPROM.update(EEPROM_addr, user_values[Array_location]);
    Array_location++;
    EEPROM_addr++;

    ind6 = value.indexOf(',', ind5 + 1);
    user_values[Array_location] = atoi(value.substring(ind5 + 1, ind6 + 1).c_str());
    EEPROM.update(EEPROM_addr, user_values[Array_location]);
    Array_location++;
    EEPROM_addr++;

    ind7 = value.indexOf(',', ind6 + 1);
    user_values[Array_location] = atoi(value.substring(ind6 + 1, ind7 + 1).c_str());
    EEPROM.update(EEPROM_addr, user_values[Array_location]);
    Array_location++;
    EEPROM_addr++;

    ind8 = value.indexOf(',', ind7 + 1);
    user_values[Array_location] = atoi(value.substring(ind7 + 1, ind8 + 1).c_str());
    EEPROM.update(EEPROM_addr, user_values[Array_location]);
    Array_location++;
    EEPROM_addr++;

    ind9 = value.indexOf(',', ind8 + 1);
    user_values[Array_location] = atoi(value.substring(ind8 + 1, ind9 + 1).c_str());
    EEPROM.update(EEPROM_addr, user_values[Array_location]);
    Array_location++;
    EEPROM_addr++;

    ind10 = value.indexOf(',', ind9 + 1);
    user_values[Array_location] = atoi(value.substring(ind9 + 1, ind10 + 1).c_str());
    EEPROM.update(EEPROM_addr, user_values[Array_location]);
    Array_location++;
    EEPROM_addr++;

    ind11 = value.indexOf(',', ind10 + 1);
    user_values[Array_location] = atoi(value.substring(ind10 + 1, ind11 + 1).c_str());
    EEPROM.update(EEPROM_addr, user_values[Array_location]);
    Array_location++;
    EEPROM_addr++;

    ind12 = value.indexOf(',', ind11 + 1);
    user_values[Array_location] = atoi(value.substring(ind11 + 1, ind12 + 1).c_str());
    EEPROM.update(EEPROM_addr, user_values[Array_location]);
    //    Array_location++;
    //    EEPROM_addr++;

    EEPROM_addr = 17;
    user_values[17] = 88;                                                           // Set flag to indicate user values are stored
    EEPROM.update(EEPROM_addr, user_values[17]);


    Alarm.disable(R1_ID1);
    Alarm.free(R1_ID1);

    Alarm.disable(R1_ID2);
    Alarm.free(R1_ID2);

    Alarm.disable(R1_ID3);
    Alarm.free(R1_ID3);

    Alarm.disable(R1_ID4);
    Alarm.free(R1_ID4);


    R1_ID1 = Alarm.alarmRepeat(user_values[0], user_values[1], user_values[2], Relay1Repeats);            // Toggle Relay1 at user requested time
    Serial.println(F("Relay1-1 Alarm set"));

    R1_ID2 = Alarm.alarmRepeat(user_values[3], user_values[4], user_values[5], Relay1Repeats);            // Toggle Relay1 at user requested time
    Serial.println(F("Relay1-2 Alarm set"));

    R1_ID3 = Alarm.alarmRepeat(user_values[6], user_values[7], user_values[8], Relay1Repeats);            // Toggle Relay1 at user requested time
    Serial.println(F("Relay1-3 Alarm set"));

    R1_ID4 = Alarm.alarmRepeat(user_values[9], user_values[10], user_values[11], Relay1Repeats);          // Toggle Relay1 at user requested time
    Serial.println(F("Relay1-4 Alarm set"));

    inputString = "";
    stringComplete = false;


  }
}


void processRelay2(void)
{
  inputString = inputString.substring(6);                               // Remove "Relay2" from inputString
  int pos = inputString.indexOf('=');                                   // Set pos to the location of "=" in the string
  if (pos > -1) {
    value = inputString.substring(pos + 1, inputString.length() - 1);   // Extract alarm values up to \n exluded

    EEPROM_addr = 12;

    ind1 = value.indexOf(',');                                          // Location of first comma
    user_values[12] = atoi(value.substring(0, ind1).c_str());           // User selected high temperature
    EEPROM.update(EEPROM_addr, user_values[12]);                        // Save value in EEPROM
    Serial.print(F("tempHigh = "));
    Serial.println(user_values[12]);
    EEPROM_addr++;

    ind2 = value.indexOf(',', ind1 + 1);                                   // Location of the second comma
    user_values[13] = atoi(value.substring(ind1 + 1, ind2 + 1).c_str());   // User selected low temperature
    EEPROM.update(EEPROM_addr, user_values[13]);                           // Save value in EEPROM
    Serial.print(F("tempLow = "));
    Serial.println(user_values[13]);

    inputString = "";
    stringComplete = false;
    EEPROM_addr = 0;


  }
}


void processRelay3(void)
{
  inputString = inputString.substring(6);                             // Remove "Relay3" from inputString
  int pos = inputString.indexOf('=');                                 // Set pos to the location of "=" in the string
  if (pos > -1) {
    value = inputString.substring(pos + 1, inputString.length() - 1); // Extract alarm values up to \n exluded

    EEPROM_addr = 14;

    ind1 = value.indexOf(',');                                        // Location of first comma
    user_values[14] = atoi(value.substring(0, ind1).c_str());         // User selected cycle period
    EEPROM.update(EEPROM_addr, user_values[14]);                      // Save value in EEPROM
    EEPROM_addr++;

    ind2 = value.indexOf(',', ind1 + 1);                              // Location of the second comma
    user_values[15] = atoi(value.substring(ind1 + 1, ind2 + 1).c_str());        // User selected alarm minutes
    EEPROM.update(EEPROM_addr, user_values[15]);                      // Save value in EEPROM
    EEPROM_addr++;

    ind3 = value.indexOf(',', ind2 + 1);
    user_values[16] = atoi(value.substring(ind2 + 1, ind3 + 1).c_str());        // User selected alarm seconds
    EEPROM.update(EEPROM_addr, user_values[16]);                     // Save value in EEPROM

    EEPROM_addr = 18;
    user_values[18] = 88;                                            // Set flag to indicate user values are stored
    EEPROM.update(EEPROM_addr, user_values[18]);

    Alarm.disable(R3cycle_ID);
    Alarm.free(R3cycle_ID);


    R3cycle_ID = Alarm.timerRepeat(user_values[14], user_values[15], user_values[16], Relay3Repeats);
    Serial.println(F("New Relay3 Alarm set"));

    inputString = "";
    stringComplete = false;
    EEPROM_addr = 0;

  }
}




void Relay1Repeats()
{
  lcd.home();
  lcd.print("Skimmer Toggle  ");
  mySerial.print(F("Relay1 Alarm at "));
  mySerial.println(currentTime);

  digitalWrite(relay1, !digitalRead(relay1));                      // Toggle state of the skimmer power

  inputString = "";
  stringComplete = false;
}


void Relay3Repeats()
{
  lcd.home();
  lcd.print("Wavemaker Toggle");
  mySerial.print(F("Relay3 Alarm at "));
  mySerial.println(currentTime);
  digitalWrite(relay3, !digitalRead(relay3));                     // Toggle state of the wavemaker power

  inputString = "";
  stringComplete = false;
}



void FeedingTime()
{

  Alarm.timerOnce(0, 10, 0, OnceOnly);                          // Calls OnceOnly once after 10 minutes
  lcd.home();
  lcd.print("Feeding Time!   ");
  Serial.println(F("Feeding Time!"));
  mySerial.print(F("Button Push at "));
  mySerial.println(currentTime);

  /* Outlet is connected to the NC relay terminals, so turning the relay ON turns off outlet power
     I feed a small frozen mysis parcel, which floats at the top of the water.
     The wavemaker gently circulates the mysis so all fish have a chance to eat. */
  digitalWrite(relay4, LOW);
  digitalWrite(relay3, LOW);                                   // Turn the wavemaker ON for feeding

}



void OnceOnly()
{
  digitalWrite(relay4, HIGH);                                  // Turn the pumps back ON after feeding
}                                                              // Allow relay3 to stay ON until next cycle









