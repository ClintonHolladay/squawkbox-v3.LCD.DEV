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
   int year;
   byte month;
   byte day;
   byte hour;
   byte minute;
   byte second; 
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
  static alarmVariable AlarmArray[10];
  static alarmVariable bob;
  static alarmVariable fred;
//  EEPROMalarm(bob, fred, SecondaryString);
//  delay(2000);
//  EEPROMalarm(bob, fred, AlarmString);
//  delay(2000);

  EEPROMalarmInput(AlarmArray, hlpcString);
  delay(2000); 
  EEPROM.get(0, fred);
  Serial.println("1st EEPROM slot of 13.");
  Serial.println(fred.alarm);
  printf("Date: %i/%i/%i\n", fred.year,fred.month,fred.day);
  printf("Time: %i:%i:%i\n\n",fred.hour,fred.minute,fred.second);
  EEPROMalarmInput(AlarmArray, AlarmString);
  EEPROM.get(13, fred);
  Serial.println("2nd EEPROM slot of 13.");
  Serial.println(fred.alarm);
  printf("Date: %i/%i/%i\n", fred.year,fred.month,fred.day);
  printf("Time: %i:%i:%i\n\n",fred.hour,fred.minute,fred.second);
  EEPROM.get(0, fred);
  Serial.println("1st EEPROM slot of 13 AGAIN....");
  Serial.println(fred.alarm);
  printf("Date: %i/%i/%i\n", fred.year,fred.month,fred.day);
  printf("Time: %i:%i:%i\n\n",fred.hour,fred.minute,fred.second);
  EEPROMalarmInput(AlarmArray, PrimaryString);
  delay(2000); 
  EEPROM.get(26, fred);
  Serial.println("3rd EEPROM slot of 13.");
  Serial.println(fred.alarm);
  printf("Date: %i/%i/%i\n", fred.year,fred.month,fred.day);
  printf("Time: %i:%i:%i\n\n",fred.hour,fred.minute,fred.second);
  EEPROMalarmInput(AlarmArray, SecondaryString);
  delay(2000); 
  EEPROM.get(39, fred);
  Serial.println("4th EEPROM slot of 13.");
  Serial.println(fred.alarm);
  printf("Date: %i/%i/%i\n", fred.year,fred.month,fred.day);
  printf("Time: %i:%i:%i\n\n",fred.hour,fred.minute,fred.second);
  EEPROM.get(13, fred);
  Serial.println("2nd EEPROM slot of 13 AGAIN....");
  Serial.println(fred.alarm);
  printf("Date: %i/%i/%i\n", fred.year,fred.month,fred.day);
  printf("Time: %i:%i:%i\n\n",fred.hour,fred.minute,fred.second);
//========================================================================
  EEPROMalarmInput(AlarmArray, hlpcString);
  delay(2000); 
  EEPROM.get(52, fred);
  Serial.println("5th EEPROM slot of 13.");
  Serial.println(fred.alarm);
  printf("Date: %i/%i/%i\n", fred.year,fred.month,fred.day);
  printf("Time: %i:%i:%i\n\n",fred.hour,fred.minute,fred.second);
  EEPROMalarmInput(AlarmArray, AlarmString);
  EEPROM.get(65, fred);
  Serial.println("6th EEPROM slot of 13.");
  Serial.println(fred.alarm);
  printf("Date: %i/%i/%i\n", fred.year,fred.month,fred.day);
  printf("Time: %i:%i:%i\n\n",fred.hour,fred.minute,fred.second);
  EEPROM.get(39, fred);
  Serial.println("4th EEPROM slot of 13 AGAIN....");
  Serial.println(fred.alarm);
  printf("Date: %i/%i/%i\n", fred.year,fred.month,fred.day);
  printf("Time: %i:%i:%i\n\n",fred.hour,fred.minute,fred.second);
  EEPROMalarmInput(AlarmArray, PrimaryString);
  delay(2000); 
  EEPROM.get(78, fred);
  Serial.println("7th EEPROM slot of 13.");
  Serial.println(fred.alarm);
  printf("Date: %i/%i/%i\n", fred.year,fred.month,fred.day);
  printf("Time: %i:%i:%i\n\n",fred.hour,fred.minute,fred.second);
  EEPROMalarmInput(AlarmArray, SecondaryString);
  EEPROM.get(91, fred);
  Serial.println("8th EEPROM slot of 13.");
  Serial.println(fred.alarm);
  printf("Date: %i/%i/%i\n", fred.year,fred.month,fred.day);
  printf("Time: %i:%i:%i\n\n",fred.hour,fred.minute,fred.second);
  EEPROMalarmInput(AlarmArray, SecondaryString);
  EEPROM.get(104, fred);
  Serial.println("9th EEPROM slot of 13.");
  Serial.println(fred.alarm);
  printf("Date: %i/%i/%i\n", fred.year,fred.month,fred.day);
  printf("Time: %i:%i:%i\n\n",fred.hour,fred.minute,fred.second);
  delay(2000);
  EEPROMalarmInput(AlarmArray, hlpcString);
  EEPROM.get(117, fred);
  Serial.println("10th EEPROM slot of 13.");
  Serial.println(fred.alarm);
  printf("Date: %i/%i/%i\n", fred.year,fred.month,fred.day);
  printf("Time: %i:%i:%i\n\n",fred.hour,fred.minute,fred.second);
  delay(2000);
//========================================================================
EEPROMalarmInput(AlarmArray, PrimaryString);
  EEPROM.get(0, fred);
  Serial.println("************************************");
  Serial.println("*******RECYCLE 1st EEPROM***********");
  Serial.println("************************************");
  Serial.println(fred.alarm);
  printf("Date: %i/%i/%i\n", fred.year,fred.month,fred.day);
  printf("Time: %i:%i:%i\n\n",fred.hour,fred.minute,fred.second);
  delay(2000);
}

void EEPROMalarmInput(alarmVariable Array[], String ALARM)
{
  static int inputCounter{};
  static int arrayCounter{};
  printf("inputCounter at start of function = %i.\n\n", inputCounter);
  DateTime now = rtc.now();
  //bob = {ALARM, now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second()};
  Array[arrayCounter] = {ALARM, now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second()};
  EEPROM.put(inputCounter, Array[arrayCounter]);
  inputCounter += 13;
  arrayCounter++;
  printf("ALARM %i saved.\n", (inputCounter/13));
  printf("inputCounter is now = %i.\n\n", inputCounter);
  if(inputCounter==130) 
  {
    inputCounter = 0;
    printf("\nREST inputCounter NOW.\n\n");
  }
  if(arrayCounter==10) 
  {
    arrayCounter = 0;
    printf("\nREST arrayCounter NOW.\n\n");
  }
}

void EEPROMalarmPrint(alarmVariable& fred)
{
  static int outputCounter{};
  EEPROM.get(outputCounter, fred);
  outputCounter += 13;
  printf("ALARM %i is retrieved.\n", (outputCounter/13));
  Serial.println(fred.alarm);
  printf("Date: %i/%i/%i\n", fred.year,fred.month,fred.day);
  printf("Time: %i:%i:%i\n",fred.hour,fred.minute,fred.second);
  printf("outputCounter is now = %i.\n\n", outputCounter);
  if(outputCounter==130) 
  {
    outputCounter = 0;
    printf("REST outputCounter NOW.\n\n");
  }
}

// includes both read and wirte to EEPROM STEP 2
//void EEPROMalarm(alarmVariable& bob, alarmVariable& fred, char ALARM[])
//{
// static int counter{};
// DateTime now = rtc.now();
// bob = {ALARM, now.timestamp(now.TIMESTAMP_DATE), now.timestamp(now.TIMESTAMP_TIME)};
// EEPROM.put(counter, bob);
// EEPROM.get(counter, fred);
// counter+=60;
// printf("ALARM %i\n", (counter/60));
// Serial.println(fred.alarm);
// Serial.println(fred.timestampDate);
// Serial.println(fred.timestampTime);
// printf("\ncounter = %i\n", counter);
// if(counter==600)      
//    counter = 0;
//    printf("REST counter NOW\n");
//  }
//}

// OLD FUNCTION BEFORE I KNEW ABOUT THE TIME STAMP
void EEPROMalarm(alarmVariable& bob, alarmVariable& fred, String ALARM)
{
  static int counter{};
  DateTime now = rtc.now();
  bob = {ALARM, now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second()};
  EEPROM.put(counter, bob);
  EEPROM.get(counter, fred);
  Serial.println(fred.alarm);
  printf("Date: %i/%i/%i\n", fred.year,fred.month,fred.day);
  printf("Time: %i:%i:%i\n",fred.hour,fred.minute,fred.second);
  counter += 13;
  printf("Counter = %i\n", counter);
  if(counter ==   130) counter = 0;
}



