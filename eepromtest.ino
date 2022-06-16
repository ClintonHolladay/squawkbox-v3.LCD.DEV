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
//    String timestampDate;
//    String timestampTime;
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
  printf("\n\nFIRST ALARM\n\n\n");
      static alarmVariable bob;
      static alarmVariable fred;
      EEPROMalarm(bob, fred, PrimaryString);
      printf("\n\nSECOND ALARM\n\n\n");
      EEPROMalarm(bob, fred, hlpcString);

      
//      bob = {hlpcString, now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second()};
//      printf("\n\nTEST Time in seconds after the delay: %i\n",now.second());
//      EEPROM.put(13, bob);
//      EEPROM.get(13, fred);
//      printf("\n\nSECOND ALARM\n\n\n");
//      Serial.println(fred.alarm);
//      printf("Date: %i/%i/%i\n", fred.year,fred.month,fred.day);
//      printf("Time - %i:%i:%i\n",fred.hour,fred.minute,fred.second);
//      printf("Size is: %i\n\n", sizeof bob);
  delay (3000) ;
}


void EEPROMalarm(alarmVariable& bob, alarmVariable& fred, String ALARM)
{
  static char buffer[40];
  static int counter{};
  DateTime now = rtc.now();
//  Serial.print("Date: ");
  sprintf(buffer,"Time stamp: %s",now.timestamp(now.TIMESTAMP_DATE)); //still figuring out printf strings...
  Serial.println(buffer);
  //printf("%c\n",buffer);
//  Serial.println(now.timestamp(now.TIMESTAMP_DATE));
//  Serial.print("Time: ");
//  Serial.println(now.timestamp(now.TIMESTAMP_TIME));
  bob = {ALARM, now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second()};
  EEPROM.put(counter, bob);
  EEPROM.get(counter, fred);
  Serial.println(fred.alarm);
  printf("Date: %i/%i/%i\n", fred.year,fred.month,fred.day);
  printf("Time: %i:%i:%i\n",fred.hour,fred.minute,fred.second);
  counter+=13;
  printf("Couonter = %i\n", counter);
  if(counter>130) counter = 0;
  delay(3000);
}









