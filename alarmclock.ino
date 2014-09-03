/* Arduino Alarm Clock
   Author: Alex Strandberg
   
   The circuit consists of a 7-Segment LED Matrix, a 14-Segment LED Matrix, 
   a Chronodot, a MCP9808 Temperature Sensor Breakout, a VS1053 Breakout, 
   and Adafruit's EZ-Link Bluetooth Module
   
   Uses Adafruit's LEDBackpack, MCP9808, and VS1053 libraries
   https://github.com/adafruit
   
   Also uses Stephanie Maks' Chronodot library
   https://github.com/Stephanie-Maks/Arduino-Chronodot
   
   The EEPROM library is used to store preferences in memory
   
*/

// include SPI, MP3 and SD libraries
#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <SD.h>

#include "EEPROM.h"

#include <Wire.h>
#include "Chronodot.h"

Chronodot RTC;

#include "Adafruit_MCP9808.h"

// Create the MCP9808 temperature sensor object
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();

// These are the pins used for the breakout example
#define BREAKOUT_RESET  9      // VS1053 reset pin (output)
#define BREAKOUT_CS     10     // VS1053 chip select pin (output)
#define BREAKOUT_DCS    8      // VS1053 Data/command select pin (output)
// These are the pins used for the music maker shield
#define SHIELD_CS     7      // VS1053 chip select pin (output)
#define SHIELD_DCS    6      // VS1053 Data/command select pin (output)

// These are common pins between breakout and shield
#define CARDCS 4     // Card chip select pin
// DREQ should be an Int pin, see http://arduino.cc/en/Reference/attachInterrupt
#define DREQ 3       // VS1053 Data request, ideally an Interrupt pin

#define LEFT_BUTTON_PIN  46
#define RIGHT_BUTTON_PIN 42
#define TOP_BUTTON_PIN   38

Adafruit_VS1053_FilePlayer musicPlayer = 
  // create breakout-example object!
  Adafruit_VS1053_FilePlayer(BREAKOUT_RESET, BREAKOUT_CS, BREAKOUT_DCS, DREQ, CARDCS);
  
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

Adafruit_7segment matrix1 = Adafruit_7segment();
Adafruit_AlphaNum4 matrix2 = Adafruit_AlphaNum4();

// Variables based on preferences stored in EEPROM
boolean is24HourFormat = false;
boolean clockColon = false;
int displayType = 0; // 0 for Temperature, 1 for Date, 2 for None
int dateDisplayPart = 0; // 0-3 for Day of Week, 4-7 for Month/Day, 8-11 for Year
boolean inFahrenheit = true;
int brightness = 15;

boolean inProgrammingMode = false;
String serialString = "";
boolean stringComplete = false;
boolean speakInfo = false;
String whatToSpeak = "Clock";


// Alarm Clock Times, a value of 255 indicates that there is no alarm set
int sundayAlarmHour = 255;
int sundayAlarmMinute = 255;
int mondayAlarmHour = 255;
int mondayAlarmMinute = 255;
int tuesdayAlarmHour = 255;
int tuesdayAlarmMinute = 255;
int wednesdayAlarmHour = 255;
int wednesdayAlarmMinute = 255;
int thursdayAlarmHour = 255;
int thursdayAlarmMinute = 255;
int fridayAlarmHour = 255;
int fridayAlarmMinute = 225;
int saturdayAlarmHour = 225;
int saturdayAlarmMinute = 225;

// Store if each alarm is on or not
boolean sundayAlarmOn = false;
boolean mondayAlarmOn = false;
boolean tuesdayAlarmOn = false;
boolean wednesdayAlarmOn = false;
boolean thursdayAlarmOn = false;
boolean fridayAlarmOn = false;
boolean saturdayAlarmOn = false;

// EEPROM Storage Address List
const int addrSundayAlarmHour = 0;
const int addrSundayAlarmMinute = 1;
const int addrMondayAlarmHour = 2;
const int addrMondayAlarmMinute = 3;
const int addrTuesdayAlarmHour = 4;
const int addrTuesdayAlarmMinute = 5;
const int addrWednesdayAlarmHour = 6;
const int addrWednesdayAlarmMinute = 7;
const int addrThursdayAlarmHour = 8;
const int addrThursdayAlarmMinute = 9;
const int addrFridayAlarmHour = 10;
const int addrFridayAlarmMinute = 11;
const int addrSaturdayAlarmHour = 12;
const int addrSaturdayAlarmMinute = 13;
const int addrDisplayType = 14;
const int addrTempUnits = 15;
const int addr24HourFormat = 16;
const int addrSundayAlarmOn = 17;
const int addrMondayAlarmOn = 18;
const int addrTuesdayAlarmOn = 19;
const int addrWednesdayAlarmOn = 20;
const int addrThursdayAlarmOn = 21;
const int addrFridayAlarmOn = 22;
const int addrSaturdayAlarmOn = 23;
const int addrSundaySong = 24;
const int addrMondaySong = 25;
const int addrTuesdaySong = 26;
const int addrWednesdaySong = 27;
const int addrThursdaySong = 28;
const int addrFridaySong = 29;
const int addrSaturdaySong = 30;
const int addrBrightness = 31;

int sundaySong = 255;
int mondaySong = 255;
int tuesdaySong = 255;
int wednesdaySong = 255;
int thursdaySong = 255;
int fridaySong = 255;
int saturdaySong = 255;

void setup() {
  Serial.begin(9600);
  
  matrix1.begin(0x70);
  matrix2.begin(0x71);
  
  Serial.println("Alarm Clock is starting up...");
  Serial.println("Initializing Chronodot.");

  Wire.begin();
  RTC.begin();

  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  
  // Uncomment to manually reset time to current computer time
  //RTC.adjust(DateTime(__DATE__, __TIME__));
  
  if (!tempsensor.begin()) {
    Serial.println("Couldn't find MCP9808!");
    while (1);
  }

  if (! musicPlayer.begin()) { // initialise the music player
     Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
     while (1);
  }
  Serial.println(F("VS1053 found"));
  
  SD.begin(CARDCS);    // initialise the SD card
  
  // Set volume for left, right channels. lower numbers == louder volume!
  musicPlayer.setVolume(20,20);

  // If DREQ is on an interrupt pin (on uno, #2 or #3) we can do background
  // audio playing
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int
  
  serialString.reserve(20);
  
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(LEFT_BUTTON_PIN + 1, OUTPUT);
  pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON_PIN + 1, OUTPUT);
  pinMode(TOP_BUTTON_PIN, INPUT_PULLUP);
  pinMode(TOP_BUTTON_PIN + 1, OUTPUT);
  
  digitalWrite(LEFT_BUTTON_PIN + 1, LOW);
  digitalWrite(RIGHT_BUTTON_PIN + 1, LOW);
  digitalWrite(TOP_BUTTON_PIN + 1, LOW);
  
  Serial.println("Loading settings...");
  
  if (EEPROM.read(addrDisplayType) == B1) displayType = 1;
  else if (EEPROM.read(addrDisplayType) == B10) displayType = 2;
  else displayType = 0;
  
  if (EEPROM.read(addrTempUnits) == B1) inFahrenheit = false;
  else inFahrenheit = true;
  
  if (EEPROM.read(addr24HourFormat) == B1) is24HourFormat = true;
  else is24HourFormat = false;
  
  sundayAlarmHour = EEPROM.read(addrSundayAlarmHour);
  sundayAlarmMinute = EEPROM.read(addrSundayAlarmMinute);
  mondayAlarmHour = EEPROM.read(addrMondayAlarmHour);
  mondayAlarmMinute = EEPROM.read(addrMondayAlarmMinute);
  tuesdayAlarmHour = EEPROM.read(addrTuesdayAlarmHour);
  tuesdayAlarmMinute = EEPROM.read(addrTuesdayAlarmMinute);
  wednesdayAlarmHour = EEPROM.read(addrWednesdayAlarmHour);
  wednesdayAlarmMinute = EEPROM.read(addrWednesdayAlarmMinute);
  thursdayAlarmHour = EEPROM.read(addrThursdayAlarmHour);
  thursdayAlarmMinute = EEPROM.read(addrThursdayAlarmMinute);
  fridayAlarmHour = EEPROM.read(addrFridayAlarmHour);
  fridayAlarmMinute = EEPROM.read(addrFridayAlarmMinute);
  saturdayAlarmHour = EEPROM.read(addrSaturdayAlarmHour);
  saturdayAlarmMinute = EEPROM.read(addrSaturdayAlarmMinute);
  
  if (EEPROM.read(addrSundayAlarmOn) == B1) sundayAlarmOn = true;
  else sundayAlarmOn = false;
  if (EEPROM.read(addrMondayAlarmOn) == B1) mondayAlarmOn = true;
  else mondayAlarmOn = false;
  if (EEPROM.read(addrTuesdayAlarmOn) == B1) tuesdayAlarmOn = true;
  else tuesdayAlarmOn = false;
  if (EEPROM.read(addrWednesdayAlarmOn) == B1) wednesdayAlarmOn = true;
  else wednesdayAlarmOn = false;
  if (EEPROM.read(addrThursdayAlarmOn) == B1) thursdayAlarmOn = true;
  else thursdayAlarmOn = false;
  if (EEPROM.read(addrFridayAlarmOn) == B1) fridayAlarmOn = true;
  else fridayAlarmOn = false;
  if (EEPROM.read(addrSaturdayAlarmOn) == B1) saturdayAlarmOn = true;
  else saturdayAlarmOn = false;
  
  sundaySong = EEPROM.read(addrSundaySong);
  mondaySong = EEPROM.read(addrMondaySong);
  tuesdaySong = EEPROM.read(addrTuesdaySong);
  wednesdaySong = EEPROM.read(addrWednesdaySong);
  thursdaySong = EEPROM.read(addrThursdaySong);
  fridaySong = EEPROM.read(addrFridaySong);
  saturdaySong = EEPROM.read(addrSaturdaySong);
  
  brightness = EEPROM.read(addrBrightness);
  if (brightness > 15 || brightness < 0) brightness = 15;
  matrix1.setBrightness(brightness);
  matrix2.setBrightness(brightness);
  
  Serial.println("Done loading, type P followed by a newline character to enter programming mode.");
  
}

void loop() {
  
  if (inProgrammingMode) {
    while (Serial.available()) {
      char inChar = (char)Serial.read();
      if (inChar == '\n') {
        stringComplete = true;
      }
      else serialString += inChar;
    }
    
    if (stringComplete) { // Handle all serial input to adjust settings
      Serial.println(serialString);
      
      if (serialString.indexOf("EXIT") != -1) {
        Serial.println("Exiting Programming Mode");
        inProgrammingMode = false;
      } else if (serialString.indexOf("SET DISPLAY TEMP") != -1) {
        Serial.println("Setting Display to Temperature");
        displayType = 0;
        EEPROM.write(addrDisplayType, 0);
      } else if (serialString.indexOf("SET DISPLAY DATE") != -1) {
        Serial.println("Setting Display to Date");
        displayType = 1;
        EEPROM.write(addrDisplayType, 1);
      } else if (serialString.indexOf("SET DISPLAY NONE") != -1) {
        Serial.println("Setting Display to None");
        displayType = 2;
        EEPROM.write(addrDisplayType, 2);
      } else if (serialString.indexOf("SET TEMP FAHRENHEIT") != -1) {
        Serial.println("Setting Temperature Units to Fahrenheit");
        inFahrenheit = true;
        EEPROM.write(addrTempUnits, 0);
      } else if (serialString.indexOf("SET TEMP CELSIUS") != -1) {
        Serial.println("Setting Temperature Units to Celsius");
        inFahrenheit = false;
        EEPROM.write(addrTempUnits, 1);
      } else if (serialString.indexOf("SET CLOCK 24-HOUR") != -1) {
        Serial.println("Setting Clock to 24-Hour Format");
        is24HourFormat = true;
        EEPROM.write(addr24HourFormat, 1);
      } else if (serialString.indexOf("SET CLOCK 12-HOUR") != -1) {
        Serial.println("Setting Clock to 12-Hour Format");
        is24HourFormat = false;
        EEPROM.write(addr24HourFormat, 0);
      } else if (serialString.indexOf("SET SUNDAY ALARM ") != -1) {
        sundayAlarmHour = serialString.substring(serialString.lastIndexOf(' ') + 1, serialString.indexOf(':')).toInt();
        sundayAlarmMinute = serialString.substring(serialString.indexOf(':') + 1, serialString.indexOf(':') + 3).toInt();
        Serial.print("Setting Alarm Clock for Sunday at ");
        printTime(sundayAlarmHour, sundayAlarmMinute, true);
        EEPROM.write(addrSundayAlarmHour, sundayAlarmHour);
        EEPROM.write(addrSundayAlarmMinute, sundayAlarmMinute);
      } else if (serialString.indexOf("TURN SUNDAY ALARM ON") != -1) {
        sundayAlarmOn = true;
        EEPROM.write(addrSundayAlarmOn, 1);
        Serial.println("Turning Sunday Alarm On");
      } else if (serialString.indexOf("TURN SUNDAY ALARM OFF") != -1) {
        sundayAlarmOn = false;
        EEPROM.write(addrSundayAlarmOn, 0);
        Serial.println("Turning Sunday Alarm Off");
      } else if (serialString.indexOf("SET MONDAY ALARM ") != -1) {
        mondayAlarmHour = serialString.substring(serialString.lastIndexOf(' ') + 1, serialString.indexOf(':')).toInt();
        mondayAlarmMinute = serialString.substring(serialString.indexOf(':') + 1, serialString.indexOf(':') + 3).toInt();
        Serial.print("Setting Alarm Clock for Monday at ");
        printTime(mondayAlarmHour, mondayAlarmMinute, true);
        EEPROM.write(addrMondayAlarmHour, mondayAlarmHour);
        EEPROM.write(addrMondayAlarmMinute, mondayAlarmMinute);
      } else if (serialString.indexOf("TURN MONDAY ALARM ON") != -1) {
        mondayAlarmOn = true;
        EEPROM.write(addrMondayAlarmOn, 1);
        Serial.println("Turning Monday Alarm On");
      } else if (serialString.indexOf("TURN MONDAY ALARM OFF") != -1) {
        mondayAlarmOn = false;
        EEPROM.write(addrMondayAlarmOn, 0);
        Serial.println("Turning Monday Alarm Off");
      } else if (serialString.indexOf("SET TUESDAY ALARM ") != -1) {
        tuesdayAlarmHour = serialString.substring(serialString.lastIndexOf(' ') + 1, serialString.indexOf(':')).toInt();
        tuesdayAlarmMinute = serialString.substring(serialString.indexOf(':') + 1, serialString.indexOf(':') + 3).toInt();
        Serial.print("Setting Alarm Clock for Tuesday at ");
        printTime(tuesdayAlarmHour, tuesdayAlarmMinute, true);
        EEPROM.write(addrTuesdayAlarmHour, tuesdayAlarmHour);
        EEPROM.write(addrTuesdayAlarmMinute, tuesdayAlarmMinute);
      } else if (serialString.indexOf("TURN TUESDAY ALARM ON") != -1) {
        tuesdayAlarmOn = true;
        EEPROM.write(addrTuesdayAlarmOn, 1);
        Serial.println("Turning Tuesday Alarm On");
      } else if (serialString.indexOf("TURN TUESDAY ALARM OFF") != -1) {
        tuesdayAlarmOn = false;
        EEPROM.write(addrTuesdayAlarmOn, 0);
        Serial.println("Turning TUESDAY Alarm Off");
      } else if (serialString.indexOf("SET WEDNESDAY ALARM ") != -1) {
        wednesdayAlarmHour = serialString.substring(serialString.lastIndexOf(' ') + 1, serialString.indexOf(':')).toInt();
        wednesdayAlarmMinute = serialString.substring(serialString.indexOf(':') + 1, serialString.indexOf(':') + 3).toInt();
        Serial.print("Setting Alarm Clock for Wednesday at ");
        printTime(wednesdayAlarmHour, wednesdayAlarmMinute, true);
        EEPROM.write(addrWednesdayAlarmHour, wednesdayAlarmHour);
        EEPROM.write(addrWednesdayAlarmMinute, wednesdayAlarmMinute);
      } else if (serialString.indexOf("TURN WEDNESDAY ALARM ON") != -1) {
        wednesdayAlarmOn = true;
        EEPROM.write(addrWednesdayAlarmOn, 1);
        Serial.println("Turning Wednesday Alarm On");
      } else if (serialString.indexOf("TURN WEDNESDAY ALARM OFF") != -1) {
        wednesdayAlarmOn = false;
        EEPROM.write(addrWednesdayAlarmOn, 0);
        Serial.println("Turning Wednesday Alarm Off");
      } else if (serialString.indexOf("SET THURSDAY ALARM ") != -1) {
        thursdayAlarmHour = serialString.substring(serialString.lastIndexOf(' ') + 1, serialString.indexOf(':')).toInt();
        thursdayAlarmMinute = serialString.substring(serialString.indexOf(':') + 1, serialString.indexOf(':') + 3).toInt();
        Serial.print("Setting Alarm Clock for Thursday at ");
        printTime(thursdayAlarmHour, thursdayAlarmMinute, true);
        EEPROM.write(addrThursdayAlarmHour, thursdayAlarmHour);
        EEPROM.write(addrThursdayAlarmMinute, thursdayAlarmMinute);
      } else if (serialString.indexOf("TURN THURSDAY ALARM ON") != -1) {
        thursdayAlarmOn = true;
        EEPROM.write(addrThursdayAlarmOn, 1);
        Serial.println("Turning Thursday Alarm On");
      } else if (serialString.indexOf("TURN THURSDAY ALARM OFF") != -1) {
        thursdayAlarmOn = false;
        EEPROM.write(addrThursdayAlarmOn, 0);
        Serial.println("Turning Thursday Alarm Off");
      } else if (serialString.indexOf("SET FRIDAY ALARM ") != -1) {
        fridayAlarmHour = serialString.substring(serialString.lastIndexOf(' ') + 1, serialString.indexOf(':')).toInt();
        fridayAlarmMinute = serialString.substring(serialString.indexOf(':') + 1, serialString.indexOf(':') + 3).toInt();
        Serial.print("Setting Alarm Clock for Friday at ");
        printTime(fridayAlarmHour, fridayAlarmMinute, true);
        EEPROM.write(addrFridayAlarmHour, fridayAlarmHour);
        EEPROM.write(addrFridayAlarmMinute, fridayAlarmMinute);
      } else if (serialString.indexOf("TURN FRIDAY ALARM ON") != -1) {
        fridayAlarmOn = true;
        EEPROM.write(addrFridayAlarmOn, 1);
        Serial.println("Turning Friday Alarm On");
      } else if (serialString.indexOf("TURN FRIDAY ALARM OFF") != -1) {
        fridayAlarmOn = false;
        EEPROM.write(addrFridayAlarmOn, 0);
        Serial.println("Turning Friday Alarm Off");
      } else if (serialString.indexOf("SET SATURDAY ALARM ") != -1) {
        saturdayAlarmHour = serialString.substring(serialString.lastIndexOf(' ') + 1, serialString.indexOf(':')).toInt();
        saturdayAlarmMinute = serialString.substring(serialString.indexOf(':') + 1, serialString.indexOf(':') + 3).toInt();
        Serial.print("Setting Alarm Clock for Saturday at ");
        printTime(saturdayAlarmHour, saturdayAlarmMinute, true);
        EEPROM.write(addrSaturdayAlarmHour, saturdayAlarmHour);
        EEPROM.write(addrSaturdayAlarmMinute, saturdayAlarmMinute);
      } else if (serialString.indexOf("TURN SATURDAY ALARM ON") != -1) {
        saturdayAlarmOn = true;
        EEPROM.write(addrSaturdayAlarmOn, 1);
        Serial.println("Turning Saturday Alarm On");
      } else if (serialString.indexOf("TURN SATURDAY ALARM OFF") != -1) {
        saturdayAlarmOn = false;
        EEPROM.write(addrSaturdayAlarmOn, 0);
        Serial.println("Turning Saturday Alarm Off");
      } else if (serialString.indexOf("SET SUNDAY SONG ") != -1) {
        sundaySong = serialString.substring(serialString.lastIndexOf(' ') + 1).toInt();
        EEPROM.write(addrSundaySong, sundaySong);
        Serial.print("Setting Sunday Alarm's Song to ");
        printTrackName(sundaySong, true);
      } else if (serialString.indexOf("SET MONDAY SONG ") != -1) {
        mondaySong = serialString.substring(serialString.lastIndexOf(' ') + 1).toInt();
        EEPROM.write(addrMondaySong, mondaySong);
        Serial.print("Setting Monday Alarm's Song to ");
        printTrackName(mondaySong, true);
      } else if (serialString.indexOf("SET TUESDAY SONG ") != -1) {
        tuesdaySong = serialString.substring(serialString.lastIndexOf(' ') + 1).toInt();
        EEPROM.write(addrTuesdaySong, tuesdaySong);
        Serial.print("Setting Tuesday Alarm's Song to ");
        printTrackName(tuesdaySong, true);
      } else if (serialString.indexOf("SET WEDNESDAY SONG ") != -1) {
        wednesdaySong = serialString.substring(serialString.lastIndexOf(' ') + 1).toInt();
        EEPROM.write(addrWednesdaySong, wednesdaySong);
        Serial.print("Setting Wednesday Alarm's Song to ");
        printTrackName(wednesdaySong, true);
      } else if (serialString.indexOf("SET THURSDAY SONG ") != -1) {
        thursdaySong = serialString.substring(serialString.lastIndexOf(' ') + 1).toInt();
        EEPROM.write(addrThursdaySong, thursdaySong);
        Serial.print("Setting Thursday Alarm's Song to ");
        printTrackName(thursdaySong, true);
      } else if (serialString.indexOf("SET FRIDAY SONG ") != -1) {
        fridaySong = serialString.substring(serialString.lastIndexOf(' ') + 1).toInt();
        EEPROM.write(addrFridaySong, fridaySong);
        Serial.print("Setting Friday Alarm's Song to ");
        printTrackName(fridaySong, true);
      } else if (serialString.indexOf("SET SATURDAY SONG ") != -1) {
        saturdaySong = serialString.substring(serialString.lastIndexOf(' ') + 1).toInt();
        EEPROM.write(addrSaturdaySong, saturdaySong);
        Serial.print("Setting Saturday Alarm's Song to ");
        printTrackName(saturdaySong, true);
      } else if (serialString.indexOf("PLAY SONG ") != -1) {
        String filename = serialString.substring(serialString.indexOf('G') + 2, serialString.indexOf('mp3') + 4);
        Serial.println("Playing " + filename);
        Serial.println("Type STOP SONG to stop playback");
        char charBuf[15];
        filename.toCharArray(charBuf, 15);
        musicPlayer.startPlayingFile(charBuf);
      } else if (serialString.indexOf("PLAY SUNDAY SONG") != -1) {
        String filename = "track";
        if (sundaySong < 10) filename += '0';
        if (sundaySong < 100) filename += '0';
        filename += sundaySong;
        filename += ".mp3";
        Serial.println("Playing " + filename);
        Serial.println("Type STOP SONG to stop playback");
        char charBuf[15];
        filename.toCharArray(charBuf, 15);
        musicPlayer.startPlayingFile(charBuf);
      } else if (serialString.indexOf("PLAY MONDAY SONG") != -1) {
        String filename = "track";
        if (mondaySong < 10) filename += '0';
        if (mondaySong < 100) filename += '0';
        filename += mondaySong;
        filename += ".mp3";
        Serial.println("Playing " + filename);
        Serial.println("Type STOP SONG to stop playback");
        char charBuf[15];
        filename.toCharArray(charBuf, 15);
        musicPlayer.startPlayingFile(charBuf);
      } else if (serialString.indexOf("PLAY TUESDAY SONG") != -1) {
        String filename = "track";
        if (tuesdaySong < 10) filename += '0';
        if (tuesdaySong < 100) filename += '0';
        filename += tuesdaySong;
        filename += ".mp3";
        Serial.println("Playing " + filename);
        Serial.println("Type STOP SONG to stop playback");
        char charBuf[15];
        filename.toCharArray(charBuf, 15);
        musicPlayer.startPlayingFile(charBuf);
      } else if (serialString.indexOf("PLAY WEDNESDAY SONG") != -1) {
        String filename = "track";
        if (wednesdaySong < 10) filename += '0';
        if (wednesdaySong < 100) filename += '0';
        filename += wednesdaySong;
        filename += ".mp3";
        Serial.println("Playing " + filename);
        Serial.println("Type STOP SONG to stop playback");
        char charBuf[15];
        filename.toCharArray(charBuf, 15);
        musicPlayer.startPlayingFile(charBuf);
      } else if (serialString.indexOf("PLAY THURSDAY SONG") != -1) {
        String filename = "track";
        if (thursdaySong < 10) filename += '0';
        if (thursdaySong < 100) filename += '0';
        filename += thursdaySong;
        filename += ".mp3";
        Serial.println("Playing " + filename);
        Serial.println("Type STOP SONG to stop playback");
        char charBuf[15];
        filename.toCharArray(charBuf, 15);
        musicPlayer.startPlayingFile(charBuf);
      } else if (serialString.indexOf("PLAY FRIDAY SONG") != -1) {
        String filename = "track";
        if (fridaySong < 10) filename += '0';
        if (fridaySong < 100) filename += '0';
        filename += fridaySong;
        filename += ".mp3";
        Serial.println("Playing " + filename);
        Serial.println("Type STOP SONG to stop playback");
        char charBuf[15];
        filename.toCharArray(charBuf, 15);
        musicPlayer.startPlayingFile(charBuf);
      } else if (serialString.indexOf("PLAY SATURDAY SONG") != -1) {
        String filename = "track";
        if (saturdaySong < 10) filename += '0';
        if (saturdaySong < 100) filename += '0';
        filename += saturdaySong;
        filename += ".mp3";
        Serial.println("Playing " + filename);
        Serial.println("Type STOP SONG to stop playback");
        char charBuf[15];
        filename.toCharArray(charBuf, 15);
        musicPlayer.startPlayingFile(charBuf);
      } else if (serialString.indexOf("STOP") != -1) {
        musicPlayer.stopPlaying();
        Serial.println("Stopping Playback");
      } else if (serialString.indexOf("SET BRIGHTNESS ") != -1) {
        brightness = serialString.substring(serialString.lastIndexOf(' ') + 1).toInt();
        if (brightness > 15 || brightness < 0) brightness = 15;
        EEPROM.write(addrBrightness, brightness);
        matrix1.setBrightness(brightness);
        matrix2.setBrightness(brightness);
        Serial.print("Setting Brightness to ");
        Serial.println(brightness);
      } else if (serialString.indexOf("HELP") != -1) {
        Serial.println("Here is a list of accepted commands: (The | symbol separates valid arguments for each command)");
        Serial.println("SET DISPLAY TEMP|DATE|NONE");
        Serial.println("SET TEMP FAHRENHEIT|CELSIUS");
        Serial.println("SET CLOCK 12-HOUR|24-HOUR");
        Serial.println("SET SUNDAY|MONDAY|TUESDAY|WEDNESDAY|THURSDAY|FRIDAY|SATURDAY ALARM HH:MM");
        Serial.println("TURN SUNDAY|MONDAY|TUESDAY|WEDNESDAY|THURSDAY|FRIDAY|SATURDAY ALARM ON|OFF");
        Serial.println("SET SUNDAY|MONDAY|TUESDAY|WEDNESDAY|THURSDAY|FRIDAY|SATURDAY SONG 0|255");
        Serial.println("PLAY SUNDAY|MONDAY|TUESDAY|WEDNESDAY|THURSDAY|FRIDAY|SATURDAY SONG");
        Serial.println("PLAY SONG track000.mp3|track255.mp3");
        Serial.println("STOP SONG");
        Serial.println("SET BRIGHTNESS 0|15");
        Serial.println("PRINT TRACK LIST");
        Serial.println("PRINT CURRENT DATA");
        Serial.println("SET TIME YEAR!MONTH@DAY#HOUR$MIN%SEC");
        Serial.println("HELP");
        Serial.println("EXIT");
      } else if (serialString.indexOf("VIEW SETTINGS") != -1) {
        Serial.print("Display: ");
        switch (displayType) {
          case 0:
            Serial.println("Temp");
            break;
          case 1:
            Serial.println("Date");
            break;
          case 2:
            Serial.println("None");
          default:
            break;
        }
        if (inFahrenheit) Serial.println("Fahrenheit");
        else Serial.println("Celsius");
        if (is24HourFormat) Serial.println("24-Hour Format");
        else Serial.println("12-Hour Format");
        Serial.print("Brightness: ");
        Serial.println(brightness);
        Serial.print("Sunday Alarm: ");
        if (sundayAlarmOn) Serial.print("ON ");
        else Serial.print("OFF ");
        printTime(sundayAlarmHour, sundayAlarmMinute, false);
        Serial.print(" ");
        printTrackName(sundaySong, true);
        Serial.print("Monday Alarm: ");
        if (mondayAlarmOn) Serial.print("ON ");
        else Serial.print("OFF ");
        printTime(mondayAlarmHour, mondayAlarmMinute, false);
        Serial.print(" ");
        printTrackName(mondaySong, true);
        Serial.print("Tuesday Alarm: ");
        if (tuesdayAlarmOn) Serial.print("ON ");
        else Serial.print("OFF ");
        printTime(tuesdayAlarmHour, tuesdayAlarmMinute, false);
        Serial.print(" ");
        printTrackName(tuesdaySong, true);
        Serial.print("Wednesday Alarm: ");
        if (wednesdayAlarmOn) Serial.print("ON ");
        else Serial.print("OFF ");
        printTime(wednesdayAlarmHour, wednesdayAlarmMinute, false);
        Serial.print(" ");
        printTrackName(wednesdaySong, true);
        Serial.print("Thursday Alarm: ");
        if (thursdayAlarmOn) Serial.print("ON ");
        else Serial.print("OFF ");
        printTime(thursdayAlarmHour, thursdayAlarmMinute, false);
        Serial.print(" ");
        printTrackName(thursdaySong, true);
        Serial.print("Friday Alarm: ");
        if (fridayAlarmOn) Serial.print("ON ");
        else Serial.print("OFF ");
        printTime(fridayAlarmHour, fridayAlarmMinute, false);
        Serial.print(" ");
        printTrackName(fridaySong, true);
        Serial.print("Saturday Alarm: ");
        if (saturdayAlarmOn) Serial.print("ON ");
        else Serial.print("OFF ");
        printTime(saturdayAlarmHour, saturdayAlarmMinute, false);
        Serial.print(" ");
        printTrackName(saturdaySong, true);
      } else if (serialString.indexOf("PRINT TRACK LIST") != -1) {
        Serial.println("Printing list of tracks:");
        File root = SD.open("/");
        printTrackList(root);
      } else if (serialString.indexOf("PRINT CURRENT DATA") != -1) {
        printCurrentData(RTC.now());
      } else if (serialString.indexOf("SET TIME ") != -1) {
        int newYear = serialString.substring(serialString.indexOf("E ") + 2, serialString.indexOf("!")).toInt();
        int newMonth = serialString.substring(serialString.indexOf("!") + 1, serialString.indexOf("@")).toInt();
        int newDay = serialString.substring(serialString.indexOf("@") + 1, serialString.indexOf("#")).toInt();
        int newHour = serialString.substring(serialString.indexOf("#") + 1, serialString.indexOf("$")).toInt();
        int newMinute = serialString.substring(serialString.indexOf("$") + 1, serialString.indexOf("%")).toInt();
        int newSecond = serialString.substring(serialString.indexOf("%") + 1).toInt();
        RTC.adjust(DateTime(newYear, newMonth, newDay, newHour, newMinute, newSecond, 0, 0));
        Serial.println("The time has been adjusted:");
        printCurrentData(RTC.now());
      } else {
        Serial.println("Command not recognized. Type HELP for a list of commands.");
      }
      
      if (inProgrammingMode) Serial.print("-> ");
      
      stringComplete = false;
      serialString = "";
    }
  } else {
  
    DateTime now = RTC.now();
    
    if (digitalRead(LEFT_BUTTON_PIN) == LOW) {
      //Serial.println("Left Button Pressed");
      displayType++;
      if (displayType > 2) displayType = 0;
      EEPROM.write(addrDisplayType, displayType);
      Serial.print("Setting Display to ");
      switch (displayType) {
        case 0:
          Serial.println("Temp");
          break;
        case 1:
          Serial.println("Date");
          dateDisplayPart = 0;
          break;
        case 2:
          Serial.println("None");
          break;
        default:
          break;
      }
    }
    
    else if (digitalRead(RIGHT_BUTTON_PIN) == LOW) {
      //Serial.println("Right Button Pressed");
      speakInfo = true;
      matrix1.drawColon(true);
      matrix1.writeDisplay();
      matrix1.blinkRate(2);
      delay(1000);
      matrix1.blinkRate(0);
      whatToSpeak = "Clock";
      if (digitalRead(RIGHT_BUTTON_PIN) == LOW) {
        if (displayType == 0) whatToSpeak = "Temp";
        else if (displayType == 1) whatToSpeak = "Date";
      }
      if (displayType == 1) {
        dateDisplayPart = 0;
      }
    }
    
    else if (digitalRead(TOP_BUTTON_PIN) == LOW) {
      boolean shouldExit = false;
      String sundayString;
      String mondayString;
      String tuesdayString;
      String wednesdayString;
      String thursdayString;
      String fridayString;
      String saturdayString;
      if (sundayAlarmOn) sundayString = "SUNDAY ALARM ON ";
      else sundayString = "SUNDAY ALARM OFF ";
      if (mondayAlarmOn) mondayString = "MONDAY ALARM ON ";
      else mondayString = "MONDAY ALARM OFF ";
      if (tuesdayAlarmOn) tuesdayString = "TUESDAY ALARM ON ";
      else tuesdayString = "TUESDAY ALARM OFF ";
      if (wednesdayAlarmOn) wednesdayString = "WEDNESDAY ALARM ON ";
      else wednesdayString = "WEDNESDAY ALARM OFF ";
      if (thursdayAlarmOn) thursdayString = "THURSDAY ALARM ON ";
      else thursdayString = "THURSDAY ALARM OFF ";
      if (fridayAlarmOn) fridayString = "FRIDAY ALARM ON ";
      else fridayString = "FRIDAY ALARM OFF ";
      if (saturdayAlarmOn) saturdayString = "SATURDAY ALARM ON ";
      else saturdayString = "SATURDAY ALARM OFF ";
      
      int dayInLoop = now.dayOfWeek();
      int displayPositionIndex = 0;
      String message;
      
      DateTime sundayDateTime = DateTime(2000, 1, 1, sundayAlarmHour, sundayAlarmMinute, 0, 0, 0);
      DateTime mondayDateTime = DateTime(2000, 1, 1, mondayAlarmHour, mondayAlarmMinute, 0, 0, 0);
      DateTime tuesdayDateTime = DateTime(2000, 1, 1, tuesdayAlarmHour, tuesdayAlarmMinute, 0, 0, 0);
      DateTime wednesdayDateTime = DateTime(2000, 1, 1, wednesdayAlarmHour, wednesdayAlarmMinute, 0, 0, 0);
      DateTime thursdayDateTime = DateTime(2000, 1, 1, thursdayAlarmHour, thursdayAlarmMinute, 0, 0, 0);
      DateTime fridayDateTime = DateTime(2000, 1, 1, fridayAlarmHour, fridayAlarmMinute, 0, 0, 0);
      DateTime saturdayDateTime = DateTime(2000, 1, 1, saturdayAlarmHour, saturdayAlarmMinute, 0, 0, 0);
      
      int oldDisplayType = displayType;
      displayType = 10; // Set this to an extraneous number so the bottom display isn't updated
      
      matrix1.drawColon(true);
      
      delay(500);
      
      while (!shouldExit) {
        matrix1.clear();
        
        switch (dayInLoop) {
           case 0:
             message = sundayString;
             updateDisplay(sundayDateTime, false);
             break;
           case 1:
             message = mondayString;
             updateDisplay(mondayDateTime, false);
             break;
           case 2:
             message = tuesdayString;
             updateDisplay(tuesdayDateTime, false);
             break;
           case 3:
             message = wednesdayString;
             updateDisplay(wednesdayDateTime, false);
             break;
           case 4:
             message = thursdayString;
             updateDisplay(thursdayDateTime, false);
             break; 
           case 5:
             message = fridayString;
             updateDisplay(fridayDateTime, false);
             break;
           case 6:
             message = saturdayString;
             updateDisplay(saturdayDateTime, false);
             break;
           case 7:
             message = "EXIT";
             matrix1.writeDisplay();
             break;
           default:
             break;
        }
        
        matrix2.clear();
        matrix2.writeDigitAscii(0, message.charAt(displayPositionIndex % message.length()));
        matrix2.writeDigitAscii(1, message.charAt((displayPositionIndex+1) % message.length()));
        matrix2.writeDigitAscii(2, message.charAt((displayPositionIndex+2) % message.length()));
        matrix2.writeDigitAscii(3, message.charAt((displayPositionIndex+3) % message.length()));
        matrix2.writeDisplay();
        if (dayInLoop != 7) displayPositionIndex++;
        
        if (digitalRead(LEFT_BUTTON_PIN) == LOW) {
          dayInLoop--;
          if (dayInLoop == -1) dayInLoop = 7;
          displayPositionIndex = 0;
          delay(500);
        }
        
        else if (digitalRead(RIGHT_BUTTON_PIN) == LOW) {
          dayInLoop++;
          if (dayInLoop == 8) dayInLoop = 0;
          displayPositionIndex = 0;
          delay(500);
        }
        
        else if (digitalRead(TOP_BUTTON_PIN) == LOW) {
          if (dayInLoop == 7) {
            shouldExit = true;
            displayType = oldDisplayType;
          }
          else {
            switch (dayInLoop) {
               case 0:
                 sundayAlarmOn = !sundayAlarmOn;
                 EEPROM.write(addrSundayAlarmOn, sundayAlarmOn);
                 break;
               case 1:
                 mondayAlarmOn = !mondayAlarmOn;
                 EEPROM.write(addrMondayAlarmOn, mondayAlarmOn);
                 break;
               case 2:
                 tuesdayAlarmOn = !tuesdayAlarmOn;
                 EEPROM.write(addrTuesdayAlarmOn, tuesdayAlarmOn);
                 break;
               case 3:
                 wednesdayAlarmOn = !wednesdayAlarmOn;
                 EEPROM.write(addrWednesdayAlarmOn, wednesdayAlarmOn);
                 break;
               case 4:
                 thursdayAlarmOn = !thursdayAlarmOn;
                 EEPROM.write(addrThursdayAlarmOn, thursdayAlarmOn);
                 break;
               case 5:
                 fridayAlarmOn = !fridayAlarmOn;
                 EEPROM.write(addrFridayAlarmOn, fridayAlarmOn);
                 break;
               case 6:
                 saturdayAlarmOn = !saturdayAlarmOn;
                 EEPROM.write(addrSaturdayAlarmOn, saturdayAlarmOn);
                 break;
               default:
                 break;
            }
            
            displayPositionIndex = 0;
            if (sundayAlarmOn) sundayString = "SUNDAY ALARM ON ";
            else sundayString = "SUNDAY ALARM OFF ";
            if (mondayAlarmOn) mondayString = "MONDAY ALARM ON ";
            else mondayString = "MONDAY ALARM OFF ";
            if (tuesdayAlarmOn) tuesdayString = "TUESDAY ALARM ON ";
            else tuesdayString = "TUESDAY ALARM OFF ";
            if (wednesdayAlarmOn) wednesdayString = "WEDNESDAY ALARM ON ";
            else wednesdayString = "WEDNESDAY ALARM OFF ";
            if (thursdayAlarmOn) thursdayString = "THURSDAY ALARM ON ";
            else thursdayString = "THURSDAY ALARM OFF ";
            if (fridayAlarmOn) fridayString = "FRIDAY ALARM ON ";
            else fridayString = "FRIDAY ALARM OFF ";
            if (saturdayAlarmOn) saturdayString = "SATURDAY ALARM ON ";
            else saturdayString = "SATURDAY ALARM OFF ";
            
            delay(500);
          }
        }
        
        delay(300);
      }
    }
    
    updateDisplay(now, false);
    
    // Check to see if it is time for the Alarm to go off, depending on the day and if the alarm is enabled for that day
    if (now.dayOfWeek() == 0 && sundayAlarmOn && sundayAlarmHour == now.hour() && sundayAlarmMinute == now.minute() && now.second() <= 1) {
      updateDisplay(now, true);
      alarm("Sunday");
    } else if (now.dayOfWeek() == 1 && mondayAlarmOn && mondayAlarmHour == now.hour() && mondayAlarmMinute == now.minute() && now.second() <= 1) {
      updateDisplay(now, true);
      alarm("Monday");
    } else if (now.dayOfWeek() == 2 && tuesdayAlarmOn && tuesdayAlarmHour == now.hour() && tuesdayAlarmMinute == now.minute() && now.second() <= 1) {
      updateDisplay(now, true);
      alarm("Tuesday");
    } else if (now.dayOfWeek() == 3 && wednesdayAlarmOn && wednesdayAlarmHour == now.hour() && wednesdayAlarmMinute == now.minute() && now.second() <= 1) {
      updateDisplay(now, true);
      alarm("Wednesday");
    } else if (now.dayOfWeek() == 4 && thursdayAlarmOn && thursdayAlarmHour == now.hour() && thursdayAlarmMinute == now.minute() && now.second() <= 1) {
      updateDisplay(now, true);
      alarm("Thursday");
    } else if (now.dayOfWeek() == 5 && fridayAlarmOn && fridayAlarmHour == now.hour() && fridayAlarmMinute == now.minute() && now.second() <= 1) {
      updateDisplay(now, true);
      alarm("Friday");
    } else if (now.dayOfWeek() == 6 && saturdayAlarmOn && saturdayAlarmHour == now.hour() && saturdayAlarmMinute == now.minute() && now.second() <= 1) {
      updateDisplay(now, true);
      alarm("Saturday");
    }
    
    if (Serial.available()) {
      char c = Serial.read();
      
      if (c=='P') {
        Serial.println("Entering Programming Mode\nAll commands are followed by a newline character\nTo leave programming mode, type EXIT\nFor a list of commands, type HELP");
        inProgrammingMode = true;
        matrix1.clear();
        matrix2.clear();
        matrix2.writeDigitAscii(0, 'P');
        matrix2.writeDigitAscii(1, 'R');
        matrix2.writeDigitAscii(2, 'O');
        matrix2.writeDigitAscii(3, 'G');
        matrix1.writeDisplay();
        matrix2.writeDisplay();
        while (Serial.available()) { Serial.read(); } // Flush out any unnecessary serial data
        Serial.print("-> ");
      }
    }
    
    delay(500);
  }
}

void updateDisplay(DateTime now, boolean alarmActive) {
  matrix1.clear();
  matrix2.clear();
  
  // Print out current time to 7-Segment Backpack
    
  // Display time in appropriate format, either 12 or 24 hour
  if (!is24HourFormat && now.hour() > 12) { // 12 Hour Code uses 24 code unless it is past 12 PM
    matrix1.writeDigitNum(1, (now.hour() - 12) % 10);
    if ((now.hour() - 12) > 9) matrix1.writeDigitNum(0, (now.hour() - 12) / 10); // Prevent printing 0 as leading digit
  } else {
    if (now.hour() == 0 && !is24HourFormat) { // Code to handle exception - hour is 12AM
      matrix1.writeDigitNum(0, 1);
      matrix1.writeDigitNum(1, 2);
    } else {
      matrix1.writeDigitNum(1,now.hour() % 10);
      if (now.hour() > 9) matrix1.writeDigitNum(0,now.hour() / 10);
      else if (is24HourFormat && now.hour() < 10) matrix1.writeDigitNum(0, 0);
    }
  }
  
  matrix1.writeDigitNum(4,now.minute() % 10, (now.hour() >= 12 && !is24HourFormat)); // Place dot after last number to indicate PM if appropriate
  if (now.minute() > 9) matrix1.writeDigitNum(3,now.minute() / 10);
  else matrix1.writeDigitNum(3, 0);
  
  matrix1.drawColon(clockColon || speakInfo); // Toggle between displaying and hiding colon
  clockColon = !clockColon;
  
  matrix1.writeDisplay();
  if (speakInfo && whatToSpeak == "Clock") {
    musicPlayer.playFullFile("timeis.mp3");
    int hourToSay = now.hour();
    if (!is24HourFormat && now.hour() > 12) hourToSay -= 12;
    else if (!is24HourFormat && now.hour() == 0) hourToSay = 12;
    String hourFilename = "";
    hourFilename += hourToSay;
    hourFilename += ".mp3";
    char hourArray[hourFilename.length() + 1];
    hourFilename.toCharArray(hourArray, hourFilename.length() + 1);
    if (is24HourFormat && now.hour() < 10) musicPlayer.playFullFile("oh.mp3");
    musicPlayer.playFullFile(hourArray);
    if (now.minute() > 0 && now.minute() < 20) {
      if (now.minute() < 10) {
        musicPlayer.playFullFile("oh.mp3");
      }
      String minuteFilename = "";
      minuteFilename += now.minute();
      minuteFilename += ".mp3";
      char minuteArray[minuteFilename.length() + 1];
      minuteFilename.toCharArray(minuteArray, minuteFilename.length() + 1);
      musicPlayer.playFullFile(minuteArray);
    } else {
      if (now.minute() == 0 && is24HourFormat) {
        musicPlayer.playFullFile("100.mp3");
      } else if (now.minute() >= 20 && now.minute() < 30) {
        musicPlayer.playFullFile("20.mp3");
      } else if (now.minute() >= 30 && now.minute() < 40) {
        musicPlayer.playFullFile("30.mp3");
      } else if (now.minute() >= 40 && now.minute() < 50) {
        musicPlayer.playFullFile("40.mp3");
      } else if (now.minute() >= 50 && now.minute() < 60) {
        musicPlayer.playFullFile("50.mp3");
      }
      switch (now.minute() % 10) {
        case 1:
          musicPlayer.playFullFile("1.mp3");
          break;
        case 2:
          musicPlayer.playFullFile("2.mp3");
          break;
        case 3:
          musicPlayer.playFullFile("3.mp3");
          break;
        case 4:
          musicPlayer.playFullFile("4.mp3");
          break;
        case 5:
          musicPlayer.playFullFile("5.mp3");
          break;
        case 6:
          musicPlayer.playFullFile("6.mp3");
          break;
        case 7:
          musicPlayer.playFullFile("7.mp3");
          break;
        case 8:
          musicPlayer.playFullFile("8.mp3");
          break;
        case 9:
          musicPlayer.playFullFile("9.mp3");
          break;
      }
    }
    if (!is24HourFormat) {
      if (now.hour() < 12) musicPlayer.playFullFile("AM.mp3");
      else musicPlayer.playFullFile("PM.mp3");
    }
    speakInfo = false;
  }
  
  if (displayType == 0) {  // Display Temperature
    int temp;
    float c = tempsensor.readTempC();
    float f = c * 9.0 / 5.0 + 32;
    if (inFahrenheit) {
      temp = f;
      matrix2.writeDigitAscii(3, 'F');
    }
    else { 
      temp = c;
      matrix2.writeDigitAscii(3, 'C');
    }
    
    String tempString = String(temp);
    
    if (temp >= 100) { // Numbers greater than or equal to 100
      matrix2.writeDigitAscii(0, tempString.charAt(0));
      matrix2.writeDigitAscii(1, tempString.charAt(1));
      matrix2.writeDigitAscii(2, tempString.charAt(2), true);
    } else if (temp >= 10) { // Numbers between 10 and 99 inclusive
      matrix2.writeDigitAscii(1, tempString.charAt(0));
      matrix2.writeDigitAscii(2, tempString.charAt(1), true);
    } else if (temp >= 0) {  // Numbers between 0 and 9 inclusive
      matrix2.writeDigitAscii(2, tempString.charAt(0), true);
    } else if (temp > -10) { // Numbers between -9 and -1 inclusive
      matrix2.writeDigitAscii(1, '-'); // Print Negative Sign
      matrix2.writeDigitAscii(2, tempString.charAt(1), true);
    } else if (temp > -100) { // Numbers between -99 and -10 inclusive
      matrix2.writeDigitAscii(0, '-');
      matrix2.writeDigitAscii(1, tempString.charAt(1));
      matrix2.writeDigitAscii(2, tempString.charAt(2), true);
    }
    matrix2.writeDisplay();
    if (speakInfo && whatToSpeak == "Temp") {
      musicPlayer.playFullFile("tempis.mp3");
      if (temp < 0) {
        musicPlayer.playFullFile("negative.mp3");
        temp = -temp;
      }
      if (temp > 100) {
        musicPlayer.playFullFile("100.mp3");
        temp -= 100;
      }
      if (temp == 100) musicPlayer.playFullFile("100.mp3");
      if (temp > 10) {
        String tempPartFilename = "";
        tempPartFilename += (temp / 10)*10;
        tempPartFilename += ".mp3";
        char tempPartArray[tempPartFilename.length() + 1];
        tempPartFilename.toCharArray(tempPartArray, tempPartFilename.length() + 1);
        musicPlayer.playFullFile(tempPartArray);
        temp = temp % 10;
      }
      if (temp <= 10) {
        switch (temp) {
          case 1:
            musicPlayer.playFullFile("1.mp3");
            break;
          case 2:
            musicPlayer.playFullFile("2.mp3");
            break;
          case 3:
            musicPlayer.playFullFile("3.mp3");
            break;
          case 4:
            musicPlayer.playFullFile("4.mp3");
            break;
          case 5:
            musicPlayer.playFullFile("5.mp3");
            break;
          case 6:
            musicPlayer.playFullFile("6.mp3");
            break;
          case 7:
            musicPlayer.playFullFile("7.mp3");
            break;
          case 8:
            musicPlayer.playFullFile("8.mp3");
            break;
          case 9:
            musicPlayer.playFullFile("9.mp3");
            break;
          case 10:
            musicPlayer.playFullFile("10.mp3");
            break;
        }
      }
      if (inFahrenheit) musicPlayer.playFullFile("degF.mp3");
      else musicPlayer.playFullFile("degC.mp3");
      speakInfo = false;
    }
  }
  
  else if (displayType == 1) {
    if (dateDisplayPart < 4) {
      //  7  -
      //  2 |  | 6
      //  1  -
      //  3 |  | 5
      //  4  -  
      
      int morningHourCutoff = 12;
      int afternoonHourCutoff = 18;
      switch (now.dayOfWeek()) {
        case 0:
          matrix2.writeDigitAscii(0, 'S');
          matrix2.writeDigitAscii(1, 'U');
          matrix2.writeDigitAscii(2, 'N');
          matrix2.writeDisplay();
          if (speakInfo && whatToSpeak == "Date" && dateDisplayPart == 0) {
            if (now.hour() < morningHourCutoff) musicPlayer.playFullFile("sunmorn.mp3");
            else if (now.hour() < afternoonHourCutoff) musicPlayer.playFullFile("sunaft.mp3");
            else musicPlayer.playFullFile("suneven.mp3");
          }
          break;
        case 1:
          matrix2.writeDigitAscii(0, 'M');
          matrix2.writeDigitAscii(1, 'O');
          matrix2.writeDigitAscii(2, 'N');
          matrix2.writeDisplay();
          if (speakInfo && whatToSpeak == "Date" && dateDisplayPart == 0) {
            if (now.hour() < morningHourCutoff) musicPlayer.playFullFile("monmorn.mp3");
            else if (now.hour() < afternoonHourCutoff) musicPlayer.playFullFile("monaft.mp3");
            else musicPlayer.playFullFile("moneven.mp3");
          }
          break;
        case 2:
          matrix2.writeDigitAscii(0, 'T');
          matrix2.writeDigitAscii(1, 'U');
          matrix2.writeDigitAscii(2, 'E');
          matrix2.writeDisplay();
          if (speakInfo && whatToSpeak == "Date" && dateDisplayPart == 0) {
            if (now.hour() < morningHourCutoff) musicPlayer.playFullFile("tuemorn.mp3");
            else if (now.hour() < afternoonHourCutoff) musicPlayer.playFullFile("tueaft.mp3");
            else musicPlayer.playFullFile("tueeven.mp3");
          }
          break;
        case 3:
          matrix2.writeDigitAscii(0, 'W');
          matrix2.writeDigitAscii(1, 'E');
          matrix2.writeDigitAscii(2, 'D');
          matrix2.writeDisplay();
          if (speakInfo && whatToSpeak == "Date" && dateDisplayPart == 0) {
            if (now.hour() < morningHourCutoff) musicPlayer.playFullFile("wedmorn.mp3");
            else if (now.hour() < afternoonHourCutoff) musicPlayer.playFullFile("wedaft.mp3");
            else musicPlayer.playFullFile("wedeven.mp3");
          }
          break;
        case 4:
          matrix2.writeDigitAscii(0, 'T');
          matrix2.writeDigitAscii(1, 'H');
          matrix2.writeDigitAscii(2, 'U');
          matrix2.writeDisplay();
          if (speakInfo && whatToSpeak == "Date" && dateDisplayPart == 0) {
            if (now.hour() < morningHourCutoff) musicPlayer.playFullFile("thumorn.mp3");
            else if (now.hour() < afternoonHourCutoff) musicPlayer.playFullFile("thuaft.mp3");
            else musicPlayer.playFullFile("thueven.mp3");
          }
          break;
        case 5:
          matrix2.writeDigitAscii(0, 'F');
          matrix2.writeDigitAscii(1, 'R');
          matrix2.writeDigitAscii(2, 'I');
          matrix2.writeDisplay();
          if (speakInfo && whatToSpeak == "Date" && dateDisplayPart == 0) {
            if (now.hour() < morningHourCutoff) musicPlayer.playFullFile("frimorn.mp3");
            else if (now.hour() < afternoonHourCutoff) musicPlayer.playFullFile("friaft.mp3");
            else musicPlayer.playFullFile("frieven.mp3");
          }
          break;
        case 6:
          matrix2.writeDigitAscii(0, 'S');
          matrix2.writeDigitAscii(1, 'A');
          matrix2.writeDigitAscii(2, 'T');
          matrix2.writeDisplay();
          if (speakInfo && whatToSpeak == "Date" && dateDisplayPart == 0) {
            if (now.hour() < morningHourCutoff) musicPlayer.playFullFile("satmorn.mp3");
            else if (now.hour() < afternoonHourCutoff) musicPlayer.playFullFile("sataft.mp3");
            else musicPlayer.playFullFile("sateven.mp3");
          }
          break;
        default:
          break;
      }
    } else if (dateDisplayPart < 8) {
      if (now.month() > 9) {
        matrix2.writeDigitAscii(0, String(now.month()).charAt(0));
        matrix2.writeDigitAscii(1, String(now.month()).charAt(1), true);
      } else {
        matrix2.writeDigitAscii(0, String(now.month()).charAt(0));
        matrix2.writeDigitAscii(1, '/');
      }
      if (now.day() > 9) {
        matrix2.writeDigitAscii(2, String(now.day()).charAt(0));
        matrix2.writeDigitAscii(3, String(now.day()).charAt(1), true);
      } else {
        matrix2.writeDigitAscii(2, '0');
        matrix2.writeDigitAscii(3, String(now.day()).charAt(0));
      }
      matrix2.writeDisplay();
      if (speakInfo && whatToSpeak == "Date" && dateDisplayPart == 4) {
        switch (now.month()) {
          case 1:
            musicPlayer.playFullFile("jan.mp3");
            break;
          case 2:
            musicPlayer.playFullFile("feb.mp3");
            break;
          case 3:
            musicPlayer.playFullFile("mar.mp3");
            break;
          case 4:
            musicPlayer.playFullFile("apr.mp3");
            break;
          case 5:
            musicPlayer.playFullFile("may.mp3");
            break;
          case 6:
            musicPlayer.playFullFile("jun.mp3");
            break;
          case 7:
            musicPlayer.playFullFile("jul.mp3");
            break;
          case 8:
            musicPlayer.playFullFile("aug.mp3");
            break;
          case 9:
            musicPlayer.playFullFile("sep.mp3");
            break;
          case 10:
            musicPlayer.playFullFile("oct.mp3");
            break;
          case 11:
            musicPlayer.playFullFile("nov.mp3");
            break;
          case 12:
            musicPlayer.playFullFile("dec.mp3");
            break;
        }
        switch (now.day()) {
          case 1:
            musicPlayer.playFullFile("1st.mp3");
            break;
          case 2:
            musicPlayer.playFullFile("2nd.mp3");
            break;
          case 3:
            musicPlayer.playFullFile("3rd.mp3");
            break;
          case 4:
            musicPlayer.playFullFile("4th.mp3");
            break;
          case 5:
            musicPlayer.playFullFile("5th.mp3");
            break;
          case 6:
            musicPlayer.playFullFile("6th.mp3");
            break;
          case 7:
            musicPlayer.playFullFile("7th.mp3");
            break;
          case 8:
            musicPlayer.playFullFile("8th.mp3");
            break;
          case 9:
            musicPlayer.playFullFile("9th.mp3");
            break;
          case 10:
            musicPlayer.playFullFile("10th.mp3");
            break;
          case 11:
            musicPlayer.playFullFile("11th.mp3");
            break;
          case 12:
            musicPlayer.playFullFile("12th.mp3");
            break;
          case 13:
            musicPlayer.playFullFile("13th.mp3");
            break;
          case 14:
            musicPlayer.playFullFile("14th.mp3");
            break;
          case 15:
            musicPlayer.playFullFile("15th.mp3");
            break;
          case 16:
            musicPlayer.playFullFile("16th.mp3");
            break;
          case 17:
            musicPlayer.playFullFile("17th.mp3");
            break;
          case 18:
            musicPlayer.playFullFile("18th.mp3");
            break;
          case 19:
            musicPlayer.playFullFile("19th.mp3");
            break;
          case 20:
            musicPlayer.playFullFile("20th.mp3");
            break;
          case 21:
            musicPlayer.playFullFile("20.mp3");
            musicPlayer.playFullFile("1st.mp3");
            break;
          case 22:
            musicPlayer.playFullFile("20.mp3");
            musicPlayer.playFullFile("2nd.mp3");
            break;
          case 23:
            musicPlayer.playFullFile("20.mp3");
            musicPlayer.playFullFile("3rd.mp3");
            break;
          case 24:
            musicPlayer.playFullFile("20.mp3");
            musicPlayer.playFullFile("4th.mp3");
            break;
          case 25:
            musicPlayer.playFullFile("20.mp3");
            musicPlayer.playFullFile("5th.mp3");
            break;
          case 26:
            musicPlayer.playFullFile("20.mp3");
            musicPlayer.playFullFile("6th.mp3");
            break;
          case 27:
            musicPlayer.playFullFile("20.mp3");
            musicPlayer.playFullFile("7th.mp3");
            break;
          case 28:
            musicPlayer.playFullFile("20.mp3");
            musicPlayer.playFullFile("8th.mp3");
            break;
          case 29:
            musicPlayer.playFullFile("20.mp3");
            musicPlayer.playFullFile("9th.mp3");
            break;
          case 30:
            musicPlayer.playFullFile("30th.mp3");
            break;
          case 31:
            musicPlayer.playFullFile("30.mp3");
            musicPlayer.playFullFile("1st.mp3");
            break;
        }
      }
    } else if (dateDisplayPart < 12) {
      String yearString = String(now.year());
      matrix2.writeDigitAscii(0, yearString.charAt(0));
      matrix2.writeDigitAscii(1, yearString.charAt(1));
      matrix2.writeDigitAscii(2, yearString.charAt(2));
      matrix2.writeDigitAscii(3, yearString.charAt(3));
      matrix2.writeDisplay();
      if (speakInfo && whatToSpeak == "Date" && dateDisplayPart == 8) {
        String firstYearPart = "";
        firstYearPart += (now.year() / 100);
        firstYearPart += ".mp3";
        String secondYearPart = "";
        secondYearPart += (now.year() % 100);
        secondYearPart += ".mp3";
        char firstArray[firstYearPart.length() + 1];
        firstYearPart.toCharArray(firstArray, firstYearPart.length() + 1);
        char secondArray[secondYearPart.length() + 1];
        secondYearPart.toCharArray(secondArray, secondYearPart.length() + 1);
        musicPlayer.playFullFile(firstArray);
        musicPlayer.playFullFile(secondArray);
      }
    }
    if (!speakInfo) dateDisplayPart++;
    else dateDisplayPart += 4;
    if (dateDisplayPart == 12) {
      dateDisplayPart = 0;
      speakInfo = false;
    }
  }
  
  matrix2.writeDisplay();
}

void alarm(String dayOfWeek) {
  boolean alarmOn = true;
  
  Serial.println("ALARM!");
  
  matrix2.writeDigitAscii(0, 'W');
  matrix2.writeDigitAscii(1, 'A');
  matrix2.writeDigitAscii(2, 'K');
  matrix2.writeDigitAscii(3, 'E');
  
  matrix1.drawColon(true);
  matrix1.writeDisplay();
  matrix2.writeDisplay();
  matrix1.blinkRate(2); // Blink Display
  matrix1.setBrightness(15); // Set to Max Brightness while alarm goes off
  matrix2.setBrightness(15);
  
  int songNumber = 255;
  if (dayOfWeek == "Sunday") {
    songNumber = sundaySong;
  } else if (dayOfWeek == "Monday") {
    songNumber = mondaySong;
  } else if (dayOfWeek == "Tuesday") {
    songNumber = tuesdaySong;
  } else if (dayOfWeek == "Wednesday") {
    songNumber = wednesdaySong;
  } else if (dayOfWeek == "Thursday") {
    songNumber = thursdaySong;
  } else if (dayOfWeek == "Friday") {
    songNumber = fridaySong;
  } else if (dayOfWeek == "Saturday") {
    songNumber = saturdaySong;
  }
  
  String filename = "track";
  if (songNumber < 10) filename += '0';
  if (songNumber < 100) filename += '0';
  filename += songNumber;
  filename += ".mp3";
  
  Serial.println("Playing " + filename);
  
  char charBuf[15];
  filename.toCharArray(charBuf, 15);
  
  musicPlayer.startPlayingFile(charBuf);
  
  DateTime lastTime = RTC.now();
  
  int messageWaitCounter = 0;
  
  while (alarmOn) {
    messageWaitCounter++;
    if (messageWaitCounter == 400) {
      matrix2.clear();
      matrix2.writeDigitAscii(0, 'U');
      matrix2.writeDigitAscii(1, 'P');
      matrix2.writeDisplay();
    } else if (messageWaitCounter == 800) {
      matrix2.clear();
      matrix2.writeDigitAscii(0, 'W');
      matrix2.writeDigitAscii(1, 'A');
      matrix2.writeDigitAscii(2, 'K');
      matrix2.writeDigitAscii(3, 'E');
      matrix2.writeDisplay();
      messageWaitCounter = 0;
    }
    DateTime now = RTC.now();
    if (lastTime.hour() != now.hour() || lastTime.minute() != now.minute()) {
      updateDisplay(now, true);
      lastTime = now;
    }
    if (Serial.available()) {
      if (Serial.read() == 'S') {
        alarmOn = false;
        musicPlayer.stopPlaying();
      }
    }
    
    // Check to dismiss the alarm and if so, play wake up message (greeting & date)
    if (digitalRead(LEFT_BUTTON_PIN) == LOW || digitalRead(RIGHT_BUTTON_PIN) == LOW) {
      alarmOn = false;
      musicPlayer.stopPlaying();
      delay(1000);
      matrix1.blinkRate(0);
      matrix1.setBrightness(brightness);
      matrix2.setBrightness(brightness);
      int tempDisplayType = displayType;
      displayType = 1;
      dateDisplayPart = 0;
      speakInfo = true;
      whatToSpeak = "Date";
      updateDisplay(now, false);
      updateDisplay(now, false);
      updateDisplay(now, false);
      displayType = tempDisplayType;
    }
  }
}

// Prints time in 24-Hour Format with proper leading zeroes and the option to end with a newline character
void printTime(int hour, int minute, boolean newline) {
  if (hour < 10) Serial.print("0");
  Serial.print(hour);
  Serial.print(":");
  if (minute < 10) Serial.print("0");
  Serial.print(minute);
  if (newline) Serial.println("");
}

// Prints track filename with proper leading zeroes and the option to end with a newline character
void printTrackName(int number, boolean newline) {
  Serial.print("track");
  if (number < 100) Serial.print("0");
  if (number < 10) Serial.print("0");
  Serial.print(number);
  Serial.print(".mp3");
  if (newline) Serial.println("");
}

void printTrackList(File dir) {
   while(true) {
     File entry =  dir.openNextFile();
     if (! entry) {
       // no more files
       break;
     }
     String filename = String(entry.name());
     filename.toLowerCase();
     if (String(entry.name()).indexOf(".MP3") != -1) Serial.println(filename);
     entry.close();
   }
}

void printCurrentData(DateTime now) {
  switch (now.dayOfWeek()) {
    case 0:
      Serial.print("Sunday ");
      break;
    case 1:
      Serial.print("Monday ");
      break;
    case 2:
      Serial.print("Tuesday ");
      break;
    case 3:
      Serial.print("Wednesday ");
      break;
    case 4:
      Serial.print("Thursday ");
      break;
    case 5:
      Serial.print("Friday ");
      break;
    case 6:
      Serial.print("Saturday ");
      break;
    default:
      break;
  }
  
  if(now.month() < 10) Serial.print("0");
  Serial.print(now.month(), DEC);
  Serial.print('/');
  if(now.day() < 10) Serial.print("0");
  Serial.print(now.day(), DEC);
  Serial.print('/');
  Serial.print(now.year(), DEC);
  Serial.print(' ');
  
  if(now.hour() < 10) Serial.print("0");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  if(now.minute() < 10) Serial.print("0");
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  if(now.second() < 10) Serial.print("0");
  Serial.print(now.second(), DEC);
  Serial.println();

  float c = tempsensor.readTempC();
  float f = c * 9.0 / 5.0 + 32;
  Serial.print(c);
  Serial.print(" degrees C ");
  Serial.print(f);
  Serial.println(" degrees F");
}
