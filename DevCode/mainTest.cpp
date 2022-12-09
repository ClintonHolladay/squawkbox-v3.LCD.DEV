#include <Arduino.h>
#include <EEPROM.h>          
#include <LibPrintf.h> 

// Declared weak in Arduino.h to allow user redefinitions.
int atexit(void (* /*func*/ )()) { return 0; }

// Weak empty variant initialization function.
// May be redefined by variant files.
void initVariant() __attribute__((weak));
void initVariant() { }

void setupUSB() __attribute__((weak));
void setupUSB() { }

int main(void)
{
  init();
  initVariant();
  #if defined(USBCON)
  USBDevice.attach();
  #endif
  
  const uint8_t EEPROMInitializationKey {42};
  const int EEPROMInitializationAddress {3700};
//setup();
  Serial.begin(9600);
  Serial.println("Hello World");
     
  if(EEPROM.read(EEPROMInitializationAddress) == EEPROMInitializationKey)
  {
    printf("\n***EEPROM has been previously initialized. RESETTING NOW...***\n");
    for (int i = 0; i < 4096; i++)
    {
      EEPROM.write(i, 255);
    }
  }
  else
  {
    printf("\n***EEPROM is uninitialized... nothin to do.***\n\n");
  }
  printf("THE END GOOD BYE.\n");
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
   for (;;) 
  { 
    if (serialEventRun) serialEventRun();
  }
        
  return 0;
}
