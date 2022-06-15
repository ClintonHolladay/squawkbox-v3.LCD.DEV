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
    String alarm {};
    int year {};
    byte month {};
    byte day {};
    byte hour {};
    byte minute {} ;
    byte second {};

    alarmVariable(String Alarm, int Year, byte Month, byte Day, byte Hour, byte Minute, byte Second)
    {
      alarm = Alarm;
      year = Year;
      month = Month;
      day = Day;
      hour = Hour;
      minute = Minute;
      second = Second;
    }
    ~alarmVariable(){}; 
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
  Serial.println("fuck you");
  DateTime now = rtc.now();
      alarmVariable bob = alarmVariable(PrimaryString, now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
      EEPROM.put(0, bob.alarm);
      static String fault{};
      EEPROM.get(0, fault);
      Serial.println(fault);
//Serial.flush();
//  printf("Pstring size is: %i\n", sizeof PrimaryString);
//  printf("Pstring size is: %i\n", sizeof SecondaryString);
//  printf("Pstring size is: %i\n", sizeof hlpcString);
//  printf("Pstring size is: %i\n\n", sizeof AlarmString);
//  printf("Pstring size is: %i\n\n", sizeof bob);
//  printf("Pstring size is: %i\n\n", sizeof bob.alarm);
    

    
//    Serial.print(now.year(), DEC);
//    Serial.print('/');
//    Serial.print(now.month(), DEC);
//    Serial.print('/');
//    Serial.print(now.day(), DEC);
//    Serial.print(" - ");
//    Serial.print(now.hour(), DEC);
//    Serial.print(':');
//    Serial.print(now.minute(), DEC);
//    Serial.print(':');
//    Serial.print(now.second(), DEC);
//    Serial.println();
  
  delay (1000) ;
}
