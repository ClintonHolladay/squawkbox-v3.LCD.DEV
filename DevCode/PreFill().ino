#include <LibPrintf.h>
#include <EEPROM.h>

struct alarmVariable
{
int alarm;
int year;
byte month;
byte day;
byte hour;
byte minute;
byte second;
};
const int eepromAlarmDataSize = sizeof(alarmVariable);
static int EEPROMinputCounter {};
const int EEPROMInitializationAddress {4020};
const int EEPROMInitializationKey {69};
const int EEPROMinputCounterAddress {4000};
const int EEPROMLastFaultAddress {3600};

void setup ()
{
Serial.begin (9600);
}

void loop ()
{
  printf("\nPreFilling EEPROM Faults now.\n\n");
  EEPROM.write(EEPROMInitializationAddress, EEPROMInitializationKey);
  EEPROM.put (EEPROMInitializationAddress, EEPROMinputCounter);
  const alarmVariable prefillSTRUCT = {255, 1111, 11, 11, 11, 11, 11};
  for (int i = 0; i < EEPROMLastFaultAddress; i += eepromAlarmDataSize)
  {
    EEPROM.put(i,prefillSTRUCT);
  }
  
//  alarmVariable readingSTRUCT;
//  for ( int i = 0; i < EEPROMLastFaultAddress; i += eepromAlarmDataSize)
//  {
//    EEPROM.get(i,readingSTRUCT);
//    printf("EEPROMinputCounter is %i \n",i);
//    printf("EEPROM ALARM is retrieved from slot %i.\n",((i/eepromAlarmDataSize) + 1));
//    printf("Alarm code: %i\n", readingSTRUCT.alarm);
//    printf("Date: %i/%i/%i\n",readingSTRUCT.year,readingSTRUCT.month,readingSTRUCT.day);
//    printf("Time: %i:%i:%i\n",readingSTRUCT.hour,readingSTRUCT.minute,readingSTRUCT.second);
//  }
}
