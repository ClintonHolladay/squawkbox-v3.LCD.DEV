#include <EEPROM.h>
#include <LibPrintf.h>
#include "RTClib.h"

RTC_PCF8523 rtc;

const String PrimaryString {"Primary Low Water"};     // ====================================//
const String SecondaryString {"Secondary Low Water"}; // alarm text printed to the LCD screen//
const String hlpcString {"High Limit"};               // alarm text printed to the LCD screen//
const String AlarmString {"FSG Alarm"};               // ====================================//

struct alarmVariable
{
  String alarm;
  String timestampDate;
  String timestampTime; 
};
void setup ()
{
    Serial.begin(9600) ;
    if (! rtc.begin()) 
    {
      Serial.println("Couldn't find RTC");
      Serial.flush();
      abort();
    }
    if (! rtc.initialized() || rtc.lostPower()) 
    {
      Serial.println("RTC is NOT initialized, let's set the time!");
      // When time needs to be set on a new device, or after a power loss, the
      // following line sets the RTC to the date & time this sketch was compiled
      delay(2000);// Note: allow 2 seconds after inserting battery or applying external power
                  // without battery before calling adjust(). This gives the PCF8523's
                  // crystal oscillator time to stabilize. If you call adjust() very quickly
                  // after the RTC is powered, lostPower() may still return true.
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      
    }
    rtc.start();
}
void loop ()
{
  static alarmVariable bob;
  static alarmVariable fred;
  //Serial.println(sizeof(bob));
  // EEPROMalarm(bob, fred, PrimaryString);
  // delay(1000);
  // EEPROMalarm(bob, fred, hlpcString);
  // delay(1000);

    EEPROMalarmInput(bob, PrimaryString);
    delay (1000);
    EEPROMalarmInput(bob, SecondaryString);
    delay (1000);
    EEPROMalarmInput(bob, hlpcString);
    delay (1000); 
    EEPROMalarmInput(bob, AlarmString); 
    delay (1000); 
    EEPROMalarmInput(bob, PrimaryString); 
    delay (1000); 
    EEPROMalarmInput(bob, AlarmString); 
    delay (1000); 
    EEPROMalarmInput(bob, hlpcString); 
    delay (1000); 
    EEPROMalarmInput(bob, AlarmString); 
    delay (1000); 
    EEPROMalarmInput(bob, SecondaryString); 
    delay (1000); 
    EEPROMalarmInput(bob, PrimaryString); 

    EEPROMalarmPrint(fred);
    delay (1000);
    EEPROMalarmPrint(fred);
    delay (1000);
    EEPROMalarmPrint(fred);
    delay (1000);
    EEPROMalarmPrint(fred);
    delay (1000);
    EEPROMalarmPrint(fred);
    delay (1000);
    EEPROMalarmPrint(fred);
    delay (1000);
    EEPROMalarmPrint(fred);
    delay (1000);
    EEPROMalarmPrint(fred);
    delay (1000);
    EEPROMalarmPrint(fred);
    delay (1000);
    EEPROMalarmPrint(fred);
    delay (1000);
}

void EEPROMalarmInput(alarmVariable& STRUCT, String ALARM)
{
  static int inputCounter{};
  DateTime now = rtc.now();
  STRUCT = {ALARM, now.timestamp(now.TIMESTAMP_DATE), now.timestamp(now.TIMESTAMP_TIME)};
  EEPROM.put(inputCounter, STRUCT);
  inputCounter += 18;
  Serial.print("ALARM ");
  Serial.println(inputCounter/18);
  Serial.print("inputCounter = ");
  Serial.println(inputCounter);
  Serial.println();
  //printf("ALARM %i\n", (inputCounter/18));
  //printf("\ninputCounter = %i\n", inputCounter);
  if(inputCounter==180) 
  {
    inputCounter = 0;
    printf("REST inputCounter NOW\n");
  }
}

void EEPROMalarmPrint(alarmVariable& fred)
{
  static int outputCounter{};
  EEPROM.get(outputCounter, fred);
  outputCounter += 18;
  printf("ALARM %i\n", (outputCounter/18));
  Serial.println(fred.alarm);
  Serial.println(fred.timestampDate);
  Serial.println(fred.timestampTime);
  printf("\noutputCounter = %i\n", outputCounter);
  if(outputCounter==180) 
  {
    outputCounter = 0;
    printf("REST outputCounter NOW\n");
  }
}

// includes booth read and wirte to EEPROM STEP 2
void EEPROMalarm(alarmVariable& bob, alarmVariable& fred, String ALARM)
{
 static int counter{};
 DateTime now = rtc.now();
 bob = {ALARM, now.timestamp(now.TIMESTAMP_DATE), now.timestamp(now.TIMESTAMP_TIME)};
 EEPROM.put(counter, bob);
 EEPROM.get(counter, fred);
 counter+=18;
 printf("ALARM %i\n", (counter/18));
 Serial.println(fred.alarm);
 Serial.println(fred.timestampDate);
 Serial.println(fred.timestampTime);
 printf("\ncounter = %i\n", counter);
 if(counter==180) 
  {
    counter = 0;
    printf("REST counter NOW\n");
  }
}

//// OLD FUNCTION BEFORE I KNEW ABOUT THE TIME STAMP
//void EEPROMalarm(alarmVariable& bob, alarmVariable& fred, String ALARM)
//{
//  static int counter{};
//  DateTime now = rtc.now();
//  Serial.print("Date: ");
//  Serial.println(now.timestamp(now.TIMESTAMP_DATE));
//  Serial.print("Time: ");
//  Serial.println(now.timestamp(now.TIMESTAMP_TIME));
//  bob = {ALARM, now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second()};
//  EEPROM.put(counter, bob);
//  EEPROM.get(counter, fred);
//  Serial.println(fred.alarm);
//  printf("Date: %i/%i/%i\n", fred.year,fred.month,fred.day);
//  printf("Time: %i:%i:%i\n",fred.hour,fred.minute,fred.second);
//  counter+=18;
//  printf("Couonter = %i\n", counter);
//  if(counter>180) counter = 0;
//}
