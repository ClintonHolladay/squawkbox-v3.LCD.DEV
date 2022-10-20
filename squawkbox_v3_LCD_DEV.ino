// squawkbox_v3.0.0 20 Oct 2022 @ 1630
// continued going back through the code and fixing the comments. 

// GNU GENERAL PUBLIC LICENSE Version 2, June 1991

// TODO:
// Add functions for Pump amps() / Aw Na box() / any others???
// Cycle count function to divide the number of cycles by the last x number of hours to provide a current cycle rate.
// Display blowdown reminder after 48 hours of no PLWCO.
// Send blow down text after 72 hours of no PLWCO.
// Make an RTC function to clean up the setup().
// Add function to send a Flame signal text message.
// Prompt when all contacts are inactive.

                               // Library licensing information  //
//#include <Arduino.h>         // GNU Lesser General Public    // Included automatically by the Arduino IDE. 
#include <SD.h>                // GNU General Public License V3
#include <ModbusMaster.h>      // Apache License Version 2.0
#include <LiquidCrystal_I2C.h> // NOT posted in the documentation
#include <EEPROM.h>            // GNU Lesser General Public
#include <RTClib.h>            // MIT License
#include <LibPrintf.h>         // MIT License
#include <MemoryFree.h>        // GNU GENERAL PUBLIC LICENSE V2

//#define PRINTF_DISABLE_ALL // Not working as advertised in the LibPrintf.h GutHub
//#define printf(x...) // Deletes ALL prinf() functions in order to save SRAM amd make the production code faster/smoother. 

//Customer activated contact # ON/OFF definitons
#define ACTIVE 1
#define INACTIVE 0

//Rotary encoder definitons
#define CW 1
#define CCW -1
#define PUSH_BUTTON 0

//PINs 10-13 are NOT to be used, unless PCB v3 is altered. These are jumpered to the SD module pins
//The following const int pins are all pre-run in the PCB:
const int low1 {5};         //to screw terminal
const int low2 {6};         //to screw terminal
const int alarmPin {4};     //to screw terminal //honeywell alarm terminal (terminal 3 on honeywell programmer)
const int hplcIN {14};      //to screw terminal
const int hplcOUT {15};     //to screw terminal
const int MAX485_DE {3};    //to modbus module
const int MAX485_RE_NEG {2};//to modbus module
const int SIMpin {A3};      // this pin is routed to SIM pin 12 for boot (DF Robot SIM7000A module)
//SerialPort #1 (Serial1) Default is pin19 RX1 and pin18 TX1 on the Mega. These are how the SIM7000A module communicates with the mega.

// Pins added for LCD development. These allow the LCD to reset after the alarm has been reset by the boiler operator
const int PLWCOoutletPin {17};
const int SLWCOoutletPin {16};

const int pushButton {42};   //=====================//
const int encoderPinA {44};  // Rotary encoder pins //
const int encoderPinB {46};  //=====================//
static int Cursor{};//Making this a global allowed for saving the cursor position when EXITing the contact menus

const int debounceInterval {3000};//NEEDS TO BE MADE CHANGEABLE ON SD CARD
                                  // to prevent false alarms from electrical noise and also prevents repeat messages from bouncing PLWCOs.   
                                  // Setting this debounce too high will prevent the annunciation of instantaneous alarms like a bouncing LWCO.
const char PrimaryString[] {"Primary Low Water"};     //====================================//
const char SecondaryString[] {"Secondary Low Water"}; //====================================//
const char hlpcString[] {"High Limit"};               //alarm text printed to the LCD screen//
const char AlarmString[] {"FSG Alarm"};               //====================================//
const char DefaultString[] {"Open Fault Memory"};     //====================================//

// variables indicating whether or not an alarm message has been sent or not
static bool PLWCOSent{};
static bool SLWCOSent{};
static bool HWAlarmSent{};
static bool hlpcSent{};

static char urlHeaderArray[100]; // Twilio end point URL (twilio might change this!) AT+HTTPPARA="URL","http://relay-post-8447.twil.io/recipient_loop?
static char contactFromArray1[25];// holds the phone number to recieve text messages
static char conToTotalArray[60] {"To="};// holds the customer phone numbers that will recieve text messages

// Bools used to maintain LCD screen until the user presses the button to go to the next screen
static bool userInput{};
static bool userInput2{};
static bool userInput3{};
static bool userInput4{};
static bool faultHistory{};
static bool stopRotaryEncoder{};

// Customer phone numbers
static char contact1[11] {"1111111111"};
static char contact2[11] {"2222222222"};
static char contact3[11] {"3333333333"};
static char contact4[11] {"4444444444"};
static char contact5[11] {"5555555555"};
static char contact6[11] {"6666666666"};

// Customer phone number activation bools
static bool contact1Status {INACTIVE}; 
static bool contact2Status {INACTIVE};
static bool contact3Status {INACTIVE};
static bool contact4Status {INACTIVE};
static bool contact5Status {INACTIVE};
static bool contact6Status {INACTIVE};

//LCD Menu Names
const char* MainScreen[8] = {"Fault History","Contact 1","Contact 2","Contact 3","Contact 4","Contact 5","Contact 6","EXIT"};
const char* contactScreen[6] = {"CURRENT#","STATUS: ","EDIT#","EXIT","SAVE#","REDO#"};

//=============Custom Data Types===================//

struct alarmVariable // Used to smoothly pull EEPROM fault history to the LCD display
{
   int alarm;
   int year;
   byte month;
   byte day;
   byte hour;
   byte minute;
   byte second; 
};

enum EEPROMAlarmCode // Used to encode which alarm is to be saved into the EEPROM
{
  PLWCO = 1, 
  SLWCO, 
  FSGalarm, 
  HighLimit
};

//EEPROM variables
const int numberOfSavedFaults {400};
const int eepromAlarmDataSize = sizeof(alarmVariable); 
static int EEPROMinputCounter{}; // Used to keep track of where the next fault should be saved in the EEPROM
const uint8_t EEPROMInitializationKey {69};

//EEPROM addresses 0 - 4096 (8 bits each)
const int EEPROMInitializationAddress {4020};
const int EEPROMinputCounterAddress {4000};
const int EEPROMLastFaultAddress {3600};
const int contact1Address {3801};
const int contact2Address {3802};
const int contact3Address {3803};
const int contact4Address {3804};
const int contact5Address {3805};
const int contact6Address {3806};

//================INSTANTIATION================//

ModbusMaster node;
LiquidCrystal_I2C lcd(0x3F, 20, 4);
File myFile; 
RTC_PCF8523 rtc;
// consider DateTime now = rtc.now() instansiation only once in the loop??? Heap frag concerns??

//=======================================================================//
//================================SETUP()================================//
//=======================================================================//

void setup() 
{
  Serial.begin(9600);
  Serial1.begin(19200);
  printf("This is squawkbox V3.LCD.0 sketch.\n");
  if (! rtc.begin()) 
  {
    printf("Couldn't find RTC.\n");
    Serial.flush();
    //Needs a function to send an SMS to notify/ (Not sure what this will do to the rest of the program if this fails??) Needs to be tested
  }
  if (! rtc.initialized() || rtc.lostPower()) 
  {
    printf("RTC is NOT initialized, let's set the time!\n");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
                // Note: allow 2 seconds after inserting battery or applying external power
                // without battery before calling adjust(). This gives the PCF8523's
                // crystal oscillator time to stabilize. If you call adjust() very quickly
                // after the RTC is powered, lostPower() may still return true.
    delay(3000);
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  
  }
  rtc.start();

  lcd.init(); 
  lcd.backlight();
  lcd.begin(20, 4); // Initialize LCD screen (columns, rows)
  lcd.setCursor(2, 1);
  lcd.print("AB3D Squawk Box");
  delay(2000);// Allows LCD screen to be visualized
  
  pinMode(low1, INPUT);
  pinMode(low2, INPUT);
  pinMode(alarmPin, INPUT);
  pinMode(hplcIN, INPUT);
  pinMode(hplcOUT, INPUT);
  pinMode(PLWCOoutletPin, INPUT);
  pinMode(SLWCOoutletPin, INPUT);
  pinMode(MAX485_RE_NEG, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);
  pinMode(SIMpin, OUTPUT);
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
  pinMode (encoderPinA, INPUT);
  pinMode (encoderPinB, INPUT);
  pinMode (pushButton, INPUT_PULLUP);

  node.begin(1, Serial);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  lcd.clear();
  lcd.setCursor(2, 1);
  lcd.print("Initializing SIM");
  lcd.setCursor(7, 2);
  lcd.print("Module");
  SIMboot();
  boot_SD();
  EEPROM_Prefill();
  loadContacts();
  printf("Contacts Loaded.  Booting SIM module.  Initiating wakeup sequence...\n");
  initiateSim();
  lcd.clear();
  lcd.noBacklight();
  Serial.print(F("Free RAM = "));
  Serial.println(freeMemory());
  printf("Setup() Function complete. Entering Main Loop() Function.\n");
}


//=======================================================================//
//=======================================================================//
//============================= MAIN LOOP()==============================//
//=======================================================================//
//=======================================================================//


void loop()
{
  print_alarms();
  primary_LW();
  secondary_LW();
  Honeywell_alarm();
  HLPC();
  timedmsg();
  SMSRequest();
  User_Input_Access_Menu();
  //printf("%u\n",millis());
}


//=======================================================================//
//=======================================================================//
//====================== FUNCTION DECLARATIONS ==========================//
//=======================================================================//
//=======================================================================//


void EEPROMalarmInput(int ALARM) // we are inputing into the EEPROM not SRAM
{
  alarmVariable writingSTRUCT;
  printf("EEPROMinputCounter at start of function = %i.\n\n", EEPROMinputCounter);
  DateTime now = rtc.now();
  writingSTRUCT = {ALARM, now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second()};
  EEPROM.put(EEPROMinputCounter, writingSTRUCT);
  EEPROMinputCounter += eepromAlarmDataSize;
  printf("ALARM saved in EEPROM slot %i.\n", (EEPROMinputCounter/eepromAlarmDataSize));
  printf("Alarm code: %i\n", ALARM);
  printf("Date: %i/%i/%i\n", writingSTRUCT.year, writingSTRUCT.month, writingSTRUCT.day);
  printf("Time: %i:%i:%i\n", writingSTRUCT.hour, writingSTRUCT.minute, writingSTRUCT.second);
  printf("EEPROMinputCounter is now = %i.\n\n", EEPROMinputCounter);
  if(EEPROMinputCounter == (eepromAlarmDataSize * numberOfSavedFaults)) 
  {
    EEPROMinputCounter = 0;
    printf("\nRESET EEPROMinputCounter NOW.\n\n");
  }
  EEPROM.put(EEPROMinputCounterAddress, EEPROMinputCounter);
}

void EEPROMalarmPrint(int& outputCounter, int encoderTurnDirection)// Print saved EEPROM faults onto the LCD
{
  char buffer[20];
  static int SavedAlarmCounter {};// The number displayed on the LCD screen after "SAVED ALARM:" (1 indicates the most recent saved alarm)
  alarmVariable readingSTRUCT;
  
  // Logic for determining if the user is turning the knob CW, CCW, or just pushed the button
  if(encoderTurnDirection == CW)//User turned the dial ClockWise
  {
    outputCounter -= eepromAlarmDataSize;/*outputCounter will always be one eepromAlarmDataSize less than the EEPROMinputCounter,
                                           this is because the outputCounter allows the user to read the saved fault where as the 
                                           inputCounter is a place holder for the next fault that is yet to be saved*/
    SavedAlarmCounter++;
    if(SavedAlarmCounter > numberOfSavedFaults)// Recycle
    {
      SavedAlarmCounter = 1;
    }
  }
  else if (encoderTurnDirection == CCW)//User tunred the dial CounterClockWise
  {
    outputCounter += eepromAlarmDataSize;
    SavedAlarmCounter--;
    if(SavedAlarmCounter < 1)// Recycle
    {
      SavedAlarmCounter = numberOfSavedFaults;
    }
  }
  else // encoderTurnDirection == PUSH_BUTTON == (0) This is when the user first pushes the rotary encoder
  {
    outputCounter = EEPROMinputCounter - eepromAlarmDataSize;
    SavedAlarmCounter = 1;
  }

  // Recycle the EEPROM address (outputCounter) in a loop fashion
  if(outputCounter < 0)
  {
    outputCounter = eepromAlarmDataSize * (numberOfSavedFaults - 1); //The inputCounter recyles when it gets to numberOfSavedFaults, so that exact address never actually gets used, hence the - 1.
  }
  if(outputCounter == (eepromAlarmDataSize * numberOfSavedFaults)) 
  {
    outputCounter = 0;
  }
  // Now that we have decided what our counters actually are we can pull the data from the EEPROM and display it.   
  EEPROM.get(outputCounter, readingSTRUCT);
  printf("EEPROMinputCounter is %i\n",EEPROMinputCounter);
  printf("EEPROM ALARM is retrieved from slot %i.\n", ((outputCounter / eepromAlarmDataSize) + 1)); //outputCounter will always be one eepromAlarmDataSize less than the EEPROMinputCounter
  printf("Alarm code: %i\n", readingSTRUCT.alarm);
  printf("Date: %i/%i/%i\n", readingSTRUCT.year,readingSTRUCT.month,readingSTRUCT.day);
  printf("Time: %i:%i:%i\n",readingSTRUCT.hour,readingSTRUCT.minute,readingSTRUCT.second);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.backlight();
  lcd.print("SAVED ALARM: ");
  lcd.print(SavedAlarmCounter);
  lcd.setCursor(0, 1);
  switch(readingSTRUCT.alarm)//Need to add more options here for the FSG alarms
  {
    case 1: lcd.print(PrimaryString);break;
    case 2: lcd.print(SecondaryString);break;
    case 3: lcd.print(AlarmString);break;
    case 4: lcd.print(hlpcString);break;
    case 255: lcd.print(DefaultString);break;
    default: lcd.print("Generic Fault ERROR"); 
  }
  lcd.setCursor(0, 2);
  sprintf(buffer,"Date: %i/%.2i/%.2i",readingSTRUCT.year,readingSTRUCT.month,readingSTRUCT.day);
  lcd.print(buffer);
  lcd.setCursor(0, 3);
  sprintf(buffer,"Time: %.2i:%.2i:%.2i",readingSTRUCT.hour,readingSTRUCT.minute,readingSTRUCT.second);
  lcd.print(buffer);
  printf("outputCounter is now = %i.\n\n", outputCounter);
}

void print_alarms()//rethink how this function is laid out and structured... There has to be a way to make it nowhere near as long. 
{   
  static bool ClearScreenSwitch {false};         // to be put into the complete LCD function
  static unsigned long ClearScreendifference {}; // to be put into the complete LCD function
  static unsigned long ClearScreenTime {};       // to be put into the complete LCD function

  if(!userInput)
  {
    if (ClearScreenSwitch == false)
    {
      ClearScreenTime = millis(); 
      ClearScreenSwitch = true;
    }
    
    ClearScreendifference = millis() - ClearScreenTime;

    if ( ClearScreendifference >= debounceInterval)
    {
      lcd.clear();
      if (!PLWCOSent && !SLWCOSent && !HWAlarmSent && !hlpcSent)
      {
        lcd.noBacklight();
        lcd.setCursor(6, 1);
        lcd.print("No Alarm");
      }
      else if (PLWCOSent && SLWCOSent && hlpcSent && HWAlarmSent )
      {
        lcd.print(PrimaryString);
        lcd.setCursor(0, 1);
        lcd.print(SecondaryString);
        lcd.setCursor(0, 2);
        lcd.print(hlpcString);
        lcd.setCursor(0, 3);
        lcd.print(AlarmString); 
      }
      else if (SLWCOSent && hlpcSent && HWAlarmSent)
      {
        lcd.print(SecondaryString);
        lcd.setCursor(0, 1);
        lcd.print(hlpcString);
        lcd.setCursor(0, 2);
        lcd.print(AlarmString); 
      }
      else if (PLWCOSent && SLWCOSent && hlpcSent)
      {
        lcd.print(PrimaryString);
        lcd.setCursor(0, 1);
        lcd.print(SecondaryString);
        lcd.setCursor(0, 2);
        lcd.print(hlpcString); 
      }
      else if (PLWCOSent && SLWCOSent && HWAlarmSent)
      {
        lcd.print(PrimaryString);
        lcd.setCursor(0, 1);
        lcd.print(SecondaryString);
        lcd.setCursor(0, 2);
        lcd.print(AlarmString);  
      }
      else if (PLWCOSent && hlpcSent && HWAlarmSent)
      {
        lcd.print(PrimaryString);
        lcd.setCursor(0, 1);
        lcd.print(hlpcString);
        lcd.setCursor(0, 2);
        lcd.print(AlarmString); 
      }
      else if (PLWCOSent && SLWCOSent)
      {
        lcd.print(PrimaryString);
        lcd.setCursor(0, 1);
        lcd.print(SecondaryString);
      }
      else if (PLWCOSent && hlpcSent)
      {
        lcd.print(PrimaryString);
        lcd.setCursor(0, 1);
        lcd.print(hlpcString);
      }
      else if (PLWCOSent && HWAlarmSent)
      {
        lcd.print(PrimaryString);
        lcd.setCursor(0, 1);
        lcd.print(AlarmString);
      }
      else if (SLWCOSent && hlpcSent)
      {
        lcd.print(SecondaryString);
        lcd.setCursor(0, 1);
        lcd.print(hlpcString);
      }
      else if (SLWCOSent && HWAlarmSent)
      {
        lcd.print(SecondaryString);
        lcd.setCursor(0, 1);
        lcd.print(AlarmString);
      }
      else if (hlpcSent && HWAlarmSent)
      {
        lcd.print(hlpcString);
        lcd.setCursor(0, 1);
        lcd.print(AlarmString);
      }
      else if (PLWCOSent)
      {
        lcd.backlight();
        lcd.print(PrimaryString);
      }
      else if (SLWCOSent)
      {
        lcd.backlight();
        lcd.print(SecondaryString);
      }
      else if (HWAlarmSent)
      {
        lcd.backlight();
        lcd.print(AlarmString);
      }
      else if (hlpcSent)
      {
        lcd.backlight();
        lcd.print(hlpcString);
      }
      ClearScreenSwitch = false;
    }
  }
}

void primary_LW()
{
  static bool alarmSwitch {false};
  static bool primaryCutoff{}; 
  static bool PLWCOoutlet{}; // added for LCD development. These allow the LCD to reset after the alarm has been reset by the boiler operator
  static unsigned long difference {};
  static unsigned long alarmTime {};
  primaryCutoff = digitalRead(low1);
  PLWCOoutlet = digitalRead(PLWCOoutletPin);
  if ((primaryCutoff == HIGH) && (PLWCOSent == 0))
  {
    if (alarmSwitch == false)
    {
      alarmTime = millis(); 
      alarmSwitch = true;
      printf("alarmSwitch is true\n");
    }
    difference = millis() - alarmTime;

    if ( difference >= debounceInterval)
    {
      LCDwaiting();
      printf("Primary low water.  Sending message.\n");
      sendSMS(urlHeaderArray, conToTotalArray, contactFromArray1, "Body=Primary%20Low%20Water\"\r");
      printf("message sent or simulated.\n");
      EEPROMalarmInput(PLWCO);
      printf("EEPROM() function Primary Low Water complete.\n");
      Data_Logger(PrimaryString);
      printf("Data_Logger(PrimaryString)function complete.\n");
      PLWCOSent = 1;
    }
    else printf("%lu\n",difference); 
  }
  else
  {
    if(!primaryCutoff && PLWCOoutlet && alarmSwitch)
    {
      alarmSwitch = false;
      PLWCOSent = 0;
    }
  }
}

void secondary_LW()
{
  static bool alarmSwitch2 {false};
  static bool secondaryCutoff{};
  static bool SLWCOoutlet{}; // added for LCD development. These allow the LCD to reset after the alarm has been reset by the boiler operator 
  static unsigned long difference2 {};
  static unsigned long alarmTime2 {};
  secondaryCutoff = digitalRead(low2);
  SLWCOoutlet = digitalRead(SLWCOoutletPin);
  if ((secondaryCutoff == HIGH) && (SLWCOSent == 0))
  {
    if (alarmSwitch2 == false)
    {
      alarmTime2 = millis();
      alarmSwitch2 = true;
      printf("alarmSwitch2 is true.\n");
    }
    difference2 = millis() - alarmTime2;

    if ( difference2 >= debounceInterval)
    {
      LCDwaiting();
      printf("Secondary low water.  Sending message.\n");
      sendSMS(urlHeaderArray, conToTotalArray, contactFromArray1, "Body=Secondary%20Low%20Water\"\r");
      printf("message sent or simulated.\n");
      EEPROMalarmInput(SLWCO);
      printf("EEPROM() function 2nd Low Water complete.\n");
      Data_Logger(SecondaryString);
      printf("Data_Logger(SecondaryString)function complete.\n");
      SLWCOSent = 1;
    }
    if (difference2 < debounceInterval)
    {
      printf("%lu\n",difference2);
    }
  }
  else
  {
    if(!secondaryCutoff && SLWCOoutlet && alarmSwitch2)
    {
      alarmSwitch2 = false;
      SLWCOSent = 0;
    }
  }
}

void Honeywell_alarm()
{
  static bool alarmSwitch3 {false};
  static bool alarm{};
  static unsigned long difference3 {};
  static unsigned long alarmTime3 {};
  alarm = digitalRead(alarmPin);
  if (alarm == HIGH && HWAlarmSent == 0)
  {
    if (alarmSwitch3 == false)
    {
      alarmTime3 = millis();
      alarmSwitch3 = true;
      printf("HW alarmSwitch is true.\n");
    }
    
    difference3 = millis() - alarmTime3;

    if ( difference3 >= debounceInterval)
    {
      LCDwaiting();
      printf("sending alarm message.\n");
      printf("about to enter modbus reading function....\n");
      sendSMS(urlHeaderArray, conToTotalArray, contactFromArray1, "Body=Boiler%20Down\"\r");
      delay(100);
      printf("about to enter modbus reading function....\n");
      readModbus();
      printf("message sent or simulated.\n");
      EEPROMalarmInput(FSGalarm);
      printf("EEPROM() function FSG Alarm complete.\n");
      Data_Logger(AlarmString);
      printf("Data_Logger(AlarmString)function complete.\n");
      HWAlarmSent = 1;
    }
    if (difference3 < debounceInterval)
    {
      printf("%lu\n",difference3);
    }
  }
  else
  {
    if(alarm == LOW && alarmSwitch3)
    {
      alarmSwitch3 = false;
      HWAlarmSent = 0;
    }
  }
}

void HLPC()
{
  static bool alarmSwitch4 {false};
  static bool hlpcCOMMON{};
  static bool hlpcNC{};
  static unsigned long difference4 {};
  static unsigned long alarmTime4 {};
  hlpcCOMMON = digitalRead(hplcIN);
  hlpcNC = digitalRead(hplcOUT);
  if ((hlpcCOMMON == HIGH) && (hlpcNC == LOW) && (hlpcSent == 0))
  {
    if (alarmSwitch4 == false)
    {
      alarmTime4 = millis();
      alarmSwitch4 = true;
      printf("AlarmSwitch is true.\n");
    }
    difference4 = millis() - alarmTime4;

    if ( difference4 >= debounceInterval)
    {
      LCDwaiting();
      printf("Sending HPLC alarm message.\n");
      sendSMS(urlHeaderArray, conToTotalArray, contactFromArray1, "Body=High%20Pressure%20Alarm\"\r");
      printf("message sent or simulated.\n");
      EEPROMalarmInput(HighLimit);
      printf("EEPROM() function High Limit complete.\n");
      Data_Logger(hlpcString);
      printf("Data_Logger(hlpcString)function complete.\n");
      hlpcSent = 1;
    }
    if (difference4 < debounceInterval)
    {
      printf("%lu\n",difference4);
    }
  }
  else
  {
    if (hlpcNC && alarmSwitch4)
    {
      alarmSwitch4 = false;
      hlpcSent = 0;
    }
  }
}

void sendSMS(char URL[], char to[], char from[], char body[])//SIM7000A module
{
  char finalURL[250] = "";
  strcpy(finalURL, URL);
  strcat(finalURL, to);
  strcat(finalURL, from);
  strcat(finalURL, body);
  Serial.println(finalURL);
  delay(20);
  Serial1.print("AT+HTTPTERM\r");
  delay(1000);
  Serial1.print("AT+SAPBR=3,1,\"APN\",\"super\"\r");
  delay(300);
  Serial1.print("AT+SAPBR=1,1\r");
  delay(1000);
  Serial1.print("AT+HTTPINIT\r");
  delay(100);
  Serial1.print("AT+HTTPPARA=\"CID\",1\r");
  delay(100);
  Serial1.println(finalURL);
  delay(100);
  Serial1.print("AT+HTTPACTION=1\r");
  delay(4000);
}

// void getResponse() // serial monitor printing for troubleshooting REMOVE FROM PRODUCTION CODE
// {
//   unsigned char data {};
//   while (Serial1.available())
//   {
//     data = Serial1.read();
//     Serial.write(data);
//     delay(5);
//   }
// }

void timedmsg() // daily timer message to ensure Squawk is still operational. For testing ONLY (REMOVE FROM PRODUCTION CODE)
{
  static bool msgswitch {false};
  static const unsigned long dailytimer {86400000};  
  static unsigned long difference5 {};
  static unsigned long msgtimer1 {};
  if (msgswitch == false)
  {
    msgtimer1 = millis();
    msgswitch = true;
  }
  difference5 = millis() - msgtimer1;

  if (difference5 >= dailytimer)
  {
    sendSMS(urlHeaderArray, conToTotalArray, contactFromArray1, "Body=LCDV3%20Routine%20Timer\"\r");
    msgswitch = false;
  }
}

void SMSRequest()//Allows the customer to text the Squawk and receive responses.
{
  char incomingChar {};
  if (Serial1.available() > 0) 
  {
    incomingChar = Serial1.read();
    if (incomingChar == 'C') 
    {
      delay(100);
      printf("%c\n",incomingChar);
      incomingChar = Serial1.read();
      if (incomingChar == 'H') 
      {
        delay(100);
        printf("%c\n", incomingChar);
        incomingChar = Serial1.read();
        if (incomingChar == 'E') 
        {
          delay(100);
          printf("%c\n",incomingChar);
          incomingChar = Serial1.read();
          if (incomingChar == 'C') 
          {
            delay(100);
            printf("%c\n",incomingChar);
            incomingChar = Serial1.read();
            if (incomingChar == 'K') 
            {
              delay(100);
              printf("%c\n",incomingChar);
              printf("GOOD CHECK. SMS SYSTEMS ONLINE.\n");
              printf("SENDING CHECK VERIFICATION MESSAGE.\n") ;
              sendSMS(urlHeaderArray, conToTotalArray, contactFromArray1, "Body=Good%20Check\"\r");
              printf("Verification message sent.\n");
              Serial1.print("AT+CMGD=0,4\r");
              delay(100);
            }
          }
        }
      }
    }
  }
}

void Data_Logger(const char FAULT[])//Saves boiler data to the SD card in CSV format. 
{
  DateTime now = rtc.now();
  myFile = SD.open("DataLog.txt", FILE_WRITE);
  if (myFile) 
  {//add in cycle count and run time hours?
    myFile.print(FAULT);
    myFile.print(",");
    myFile.print(now.unixtime());
    myFile.print(",");
    myFile.print(now.year());  
    myFile.print(",");      
    myFile.print(now.month());  
    myFile.print(",");
    myFile.print(now.day());  
    myFile.print(",");
    myFile.print(now.hour());  
    myFile.print(",");
    myFile.print(now.minute());  
    myFile.print(",");
    myFile.print(now.second());
    myFile.println();  //End of Row move to next row
    myFile.close();    //Close the file
    printf("Data Log Complete.\n");
  } 
  else
  {
    printf("Data Log failed.\n");//add in fault code LED function?
  }
}

void boot_SD() //see if the card is present and can be initialized
{
  if (!SD.begin(10)) 
  {
    printf("***ERROR SD Card failed or not present.***\n");
    // LCD display code for SD failure***********************************************************************
  }
  else
  {
    printf("SD.begin() is GOOD.\n");
    //const char* FILE {"DataLog.txt"}; //Arduino file names can only be <= 8 characters, this took a while to figure out...:(
    myFile = SD.open("DataLog.txt", FILE_WRITE);
    if (myFile) 
    {
      if(myFile.position() == 0)//Initialize the top data row of the CSV file if it has NOT been done already.
      {
        myFile.println("Fault, UnixTime, Year, Month, Day, Hour, Minute, Second");
        myFile.close();
        printf("SD Card initialization complete.\n");
      }
      else //if CSV file was already initialized then DataLog that a reset occured.
      {
        DateTime now = rtc.now();
        myFile.print("SystemRESET");
        myFile.print(",");
        myFile.print(now.unixtime());
        myFile.print(",");
        myFile.print(now.year());  
        myFile.print(",");      
        myFile.print(now.month());  
        myFile.print(",");
        myFile.print(now.day());  
        myFile.print(",");
        myFile.print(now.hour());  
        myFile.print(",");
        myFile.print(now.minute());  
        myFile.print(",");
        myFile.print(now.second());
        myFile.println();  //End of Row move to next row
        myFile.close();
        printf("SD Prior initialization recognition & SystemRESET Data Log Complete.\n");
      }
    }
    else
    {
      printf("***ERROR SD Prior initialization recognition OR SystemRESET Data Log Failed.***\n");
      //add in Exception catch
    }
  }
}

void loadContacts()//Pull customer and Squawk phone numbers from the SD card and load them into the local variables
{
  fill_from_SD("from1.txt", contactFromArray1);
  printf("From number is: ");
  Serial.println(contactFromArray1);
  fill_from_SD("to1.txt", contact1);
  if (contact1[0] > 0 && contact1Status) // maybe do if (contact1[0] > 48 && contact1[0] < 58 && contact1Status) ASCII will check for an actual phon number
  {
    strcat(conToTotalArray, contact1);
  }
  fill_from_SD("to2.txt", contact2);
  if (contact2[0] > 0 && contact2Status) 
  {
    if(conToTotalArray[3] == '\0')
    {
      strcat(conToTotalArray, contact2);
    }
    else
    {
      strcat(conToTotalArray, ",");
      strcat(conToTotalArray, contact2);
    }
  }
  fill_from_SD("to3.txt",contact3);
  if (contact3[0] > 0 && contact3Status) 
  {
    if(conToTotalArray[3] == '\0')
    {
      strcat(conToTotalArray, contact3);
    }
    else
    {
      strcat(conToTotalArray, ",");
      strcat(conToTotalArray, contact3);
    }
  }
  fill_from_SD("to4.txt", contact4);
  if (contact4[0] > 0 && contact4Status) 
  {
    if(conToTotalArray[3] == '\0')
    {
      strcat(conToTotalArray, contact4);
    }
    else
    {
      strcat(conToTotalArray, ",");
      strcat(conToTotalArray, contact4);
    }
  }
  fill_from_SD("to5.txt", contact5);
  if (contact5[0] > 0 && contact5Status) 
  {
    if(conToTotalArray[3] == '\0')
    {
      strcat(conToTotalArray, contact5);
    }
    else
    {
      strcat(conToTotalArray, ",");
      strcat(conToTotalArray, contact5);
    }
  }
  fill_from_SD("to6.txt", contact6);
  if (contact6[0] > 0 && contact6Status) 
  {
    if(conToTotalArray[3] == '\0')
    {
      strcat(conToTotalArray, contact6);
    }
    else
    {
      strcat(conToTotalArray, ",");
      strcat(conToTotalArray, contact6);
    }
  }
  strcat(conToTotalArray, "&");//format the "to" list of numbers for being in the URL by ending it with '&' so that next parameter can come after
  printf("The total contact list is: \n");
  Serial.println(conToTotalArray);
  printf("fourth position character: ");
  Serial.println(conToTotalArray[3]);
  fill_from_SD("URL.txt", urlHeaderArray);
  printf("URL header is: ");
  Serial.println(urlHeaderArray);
}

void fill_from_SD(char file_name[], char CONTACT[])
{
  myFile = SD.open(file_name, FILE_WRITE);
  if (myFile) 
  {
    int i{}; // reconsider this naming???
    myFile.seek(0);
    if(myFile.available())
    {
      while (myFile.available())
      {
        char c = myFile.read();  //gets one byte from serial buffer
        CONTACT[i] = c;
        i++;
      }
    myFile.close();
    CONTACT[i] = '\0'; //may not be needed
    }
    printf("CONTACT is: ");
    Serial.println(CONTACT);
  }
  else
  {
    // if the file didn't open, print an error:
    printf("***ERROR opening SD contactTo file.***\n");
  }
}

void preTransmission() // user designated action required by the MODBUS library
{                      // writing these terminals to HIGH / LOW tells the RS-485 board to send or recieve
  digitalWrite(MAX485_RE_NEG, 1);
  digitalWrite(MAX485_DE, 1);
}

void postTransmission() // user designated action required by the MODBUS library
{
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
}

void readModbus() // getting FSG faults from the FSG // ALL DELAYS ARE NECESSARY
{
  printf("In the readModbus() function now.\n");
  uint16_t result = node.readHoldingRegisters (0x0000, 1);

  if (result == node.ku8MBSuccess) // ku8MBSuccess == 0x00
  {
    int alarmRegister = node.getResponseBuffer(result);
    printf("Register response:  ");
    Serial.println(alarmRegister);

    switch (alarmRegister)
    {
      case  1: sendSMS(urlHeaderArray, conToTotalArray, contactFromArray1, "Body=Fault%20Code1%20No%20Purge%20Card\"\r");
        break;
      case 10: sendSMS(urlHeaderArray, conToTotalArray, contactFromArray1, "Body=Fault%20Code10%20PreIgnition%20Interlock%20Standby\"\r");
        break;
      case 14: sendSMS(urlHeaderArray, conToTotalArray, contactFromArray1, "Body=Fault%20Code14%20High%20Fire%20Interlock%20Switch\"\r");
        break;
      case 15: sendSMS(urlHeaderArray, conToTotalArray, contactFromArray1, "Body=Fault%20Code15%20Unexpected%20Flame\"\r");
        break;
      case 17: sendSMS(urlHeaderArray, conToTotalArray, contactFromArray1, "Body=Fault%20Code17%20Main%20Flame%20Failure%20RUN\"\r");
        break;
      case 19: sendSMS(urlHeaderArray, conToTotalArray, contactFromArray1, "Body=Fault%20Code19%20Main%20Flame%20Ignition%20Failure\"\r");
        break;
      case 20: sendSMS(urlHeaderArray, conToTotalArray, contactFromArray1, "Body=Fault%20Code20%20Low%20Fire%20Interlock%20Switch\"\r");
        break;
      case 28: sendSMS(urlHeaderArray, conToTotalArray, contactFromArray1, "Body=Fault%20Code28%20Pilot%20Flame%20Failure\"\r");
        break;
      case 29: sendSMS(urlHeaderArray, conToTotalArray, contactFromArray1, "Body=Fault%20Code29%20Lockout%20Interlock\"\r");
        break;
      case 33: sendSMS(urlHeaderArray, conToTotalArray, contactFromArray1, "Body=Fault%20Code33%20PreIgnition%20Interlock\"\r");
        break;
      case 47: sendSMS(urlHeaderArray, conToTotalArray, contactFromArray1, "Body=Fault%20Code47%20Jumpers%20Changed\"\r");
        break;
      default: sendSMS(urlHeaderArray, conToTotalArray, contactFromArray1, "Body=Fault%20Check%20Fault%20Code\"\r");
        break;
    }
  }
  else
  {
    sendSMS(urlHeaderArray, conToTotalArray, contactFromArray1, "Body=Modbus%20Com%20Fail\"\r");
  }
}

void SIMboot()// ALL DELAYS ARE NECESSARY
{
  printf("Starting SIMboot().\n");
//This function only boots the SIM module if it needs to be booted
//This prevents nuisance SIM module power-downs upon Mega reboot
  unsigned char sim_buffer {};
  for (int i = 0; i < 10; i++)
  {
    for (int j = 0; j < 5; j++)
    {
      Serial1.print("AT\r"); //blank AT command elicits a response of OK
      delay(50);
    }
    if (Serial1.available())
    {
      while (Serial1.available())
      {
        sim_buffer = Serial1.read();
        Serial.write(sim_buffer);
      }
      printf("SIM module appears to be on.  No need to boot\n");
      return;
    }
    else 
    {
      printf("SIM module appears to be off.  Attempting boot...\n");
      digitalWrite(SIMpin, HIGH);
      delay(3000);
      digitalWrite(SIMpin, LOW);
      printf("boot attempted.  wait 10 seconds\n");
      delay(10000);
    }
  }
}

void initiateSim()// ALL DELAYS ARE NECESSARY
{
  //The remainder of the setup is for waking the SIM module, logging on, and sending the first
  //test message to verify proper booting.
  printf("Hey!  Wake up!\n");
  Serial1.print("AT\r"); //Toggling this blank AT command elicits a mirror response from the
                         //SIM module and helps to activate it.
  Serial1.print("AT\r");
  delay(50);
  Serial1.print("AT\r");
  delay(50);
  Serial1.print("AT\r");
  delay(50);
  Serial1.print("AT\r");
  delay(50);

  //========   SIM MODULE SETUP   =======//

  Serial1.print("AT+CGDCONT=1,\"IP\",\"super\"\r");//"super" is the key required to log onto the network using Twilio SuperSIM
  delay(500);
  Serial1.print("AT+COPS=1,2,\"310410\"\r"); //310410 is AT&T's network code https://www.msisdn.net/mccmnc/310410/
  delay(5000);
  Serial1.print("AT+SAPBR=3,1,\"APN\",\"super\"\r"); //establish SAPBR profile.  APN = "super"
  delay(3000);
  Serial1.print("AT+SAPBR=1,1\r");
  delay(2000);
  Serial1.print("AT+CMGD=0,4\r"); //this line deletes any existing text messages to ensure
                                  //that the message space is empy and able to accept new messages
  delay(100);
  Serial1.print("AT+CMGF=1\r");
  delay(100);
  Serial1.print("AT+CNMI=2,2,0,0,0\r"); //
  delay(100);
  sendSMS(urlHeaderArray, contactFromArray1, conToTotalArray, "Body=Setup%20Complete\"\r");
  delay(2000);
  printf("initiateSim() complete.\n");
}

void LCDwaiting()//Display to the customer that an SMS is being sent and that the LCD will be unresponsive during these few seconds. 
{
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(4, 0);
  lcd.print("PLEASE WAIT");
  lcd.setCursor(3, 1);
  lcd.print("SENDING ALARM");
  lcd.setCursor(6, 2);
  lcd.print("MESSAGE");
  lcd.setCursor(4, 3);
  lcd.print("PLEASE WAIT");
  if(userInput || userInput2)
  {
    userInput = false; //If the user is accessing the EEPROM memory faults while a new fault occurs, 
    userInput2 = false; //this will kick them out of that menu and allow for the display of the new fault.
  }
}

void EEPROM_Prefill()// EEPROM initialization function
{
  // Do we need redundancy here? 
  if(EEPROM.read(EEPROMInitializationAddress) == EEPROMInitializationKey) //If this is true then the EEPROM has already been initialized.  
  {
    printf("\n***EEPROM has been previously initialized.***\n");
    EEPROM.get(EEPROMinputCounterAddress, EEPROMinputCounter); //Retrieve the saved inputCounter
    contact1Status = EEPROM.read(contact1Address);
    contact2Status = EEPROM.read(contact2Address);
    contact3Status = EEPROM.read(contact3Address);    //determine if these contacts are active or not
    contact4Status = EEPROM.read(contact4Address);
    contact5Status = EEPROM.read(contact5Address);
    contact6Status = EEPROM.read(contact6Address);
  }
  else //EEPROM has NOT already been initialized and needs the default data saved into it. 
  {
    printf("\n***EEPROM is uninitialized... PreFilling EEPROM Faults now.***\n\n");
    EEPROM.write(EEPROMInitializationAddress, EEPROMInitializationKey);//Key being saved shows that the EEPROM has been initialized.
    EEPROM.put (EEPROMinputCounterAddress, EEPROMinputCounter);
    const alarmVariable prefillSTRUCT = {255, 1111, 11, 11, 11, 11, 11};
    for (int i = 0; i < EEPROMLastFaultAddress; i += eepromAlarmDataSize)//Iterate through the EEPROM and fill it with the default data.
    {
      EEPROM.put(i, prefillSTRUCT);
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
}

float get_flame_signal()
{
  uint16_t result = node.readHoldingRegisters (0x000A , 1);//As per the S7800A1146 Display manual 
  if (result == node.ku8MBSuccess)
  {
    float flameSignal = node.getResponseBuffer(result);
    flameSignal = map(flameSignal,0,105,0,5);//As per the S7800A1146 Display manual 
    return flameSignal;
  }
  else
  {
    printf("***ERROR Flame signal retrival failed.***\n");
    return -1.0;
  }
}

void User_Input_Main_Screen(int CURSOR)// Prints the Main Contact screen onto the LCD
{
  lcd.backlight();
  if(CURSOR < 4)
  {
    lcd.clear();
    lcd.setCursor(0,CURSOR);
    lcd.print("-");
    lcd.setCursor(1,CURSOR);
    lcd.print(">");
    for(int i = 0; i < 4; i++)
    {
      lcd.setCursor(2,i);
      lcd.print(MainScreen[i]);
    }
  }
  else
  {
    lcd.clear();
    lcd.setCursor(0,CURSOR - 4);
    lcd.print("-");
    lcd.setCursor(1,CURSOR - 4);
    lcd.print(">");
    for(int i = 0; i < 4; i++)
    {
      lcd.setCursor(2,i);
      lcd.print(MainScreen[i + 4]);
    }
  }
}

void User_Input_Contact_Screen(const char* SCREEN[], int CURSOR, char CONTACT[], bool STATUS, char CONTACTNAME[]) 
{ //Once the user chooses a specific contact this function will print that contact's data to the LCD. 
    lcd.clear();
    lcd.setCursor(0,CURSOR);
    lcd.print("-");
    lcd.setCursor(1,CURSOR);
    lcd.print(">");
    lcd.setCursor(0,0);
    lcd.print(CONTACTNAME);
    lcd.print(" ");
    lcd.print(CONTACT);
    lcd.setCursor(2,1);
    lcd.print(SCREEN[1]);
    if(STATUS)
    {
      lcd.print("ACTIVE");
    }
    else
    {
      lcd.print("INACTIVE");
    }
    lcd.setCursor(2,2);
    lcd.print(SCREEN[2]);
    lcd.setCursor(2,3);
    lcd.print(SCREEN[3]);
}

void Contact_Edit_Screen( char* SCREEN[], int CURSOR, char CONTACT[], bool STATUS) 
{//Once the user chooses to EDIT a specific contact this function will print that contact's EDIT data to the LCD.
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(SCREEN[0]);
    lcd.print("  ");
    lcd.print(CONTACT);
    lcd.setCursor(0,1);
    lcd.print("ENTER NEW#0");
    lcd.setCursor(2,2);
    lcd.print(SCREEN[4]);
    lcd.setCursor(2,3);
    lcd.print(SCREEN[5]);
    lcd.setCursor(10,3);
    lcd.print(SCREEN[3]);
    lcd.setCursor(10,2);
    lcd.print("TIMER:180");
    lcd.setCursor(10,1);
    lcd.blink();
}

void User_Input_Access_Menu() //Controls the logic behind the functionality of the LCD menu
{
 //==============================================================================================//
 //==============================================================================================//
 //==================================MAIN MENU FUNCTION==========================================//
 //==============================================================================================//
 //==============================================================================================//
 
  //static int Cursor{}; was changed to be a GLOBAL variable
  static int encoderPinALast {LOW}; 
  static int n {LOW};
  static unsigned long LCDdebounce{};
  static int debounceDelay{350};
  static bool LCDTimerSwitch {false};
  static int whichContactToEdit{};

  // Using a timer to latch the momentary user input into a constant ON or OFF position
  // LCDTimerSwitch is used to allow main loop to continue and LCDdebounce to only be set to millis() once per pushButton push
  if(digitalRead(pushButton) == LOW && LCDTimerSwitch == false && !userInput && !userInput2)
  {
    LCDdebounce = millis();
    LCDTimerSwitch = true;
  }
  /*  Once userInput has been recieved and the debounce time has passed we ! the userInput bool and turn 
   *  off the LCDTimerSwitch to stop running through the timer code until a new userinput is recieved.*/
  if (LCDTimerSwitch && (millis() - LCDdebounce) > debounceDelay && !userInput && !userInput2)
  {
    delay(50);//not needed?
    userInput = true;
    LCDTimerSwitch = false;
    printf("User Input Recieved ON.\n");
    Cursor = 0;//this line not needed?
    User_Input_Main_Screen(Cursor);//change to take no arguments and default cursor to 0 in the body of the function?
  }
  /*  The userInput bool is used as a latch so that the LCD screen continues to display what the user wants until the pushButton 
  *  is activated again. When the above timer algorithm has been satisfied the squawk will begin tracking rotational rotary 
  *  encoder input from the user in order to traverse and LCD display.*/
  if(userInput && !userInput2)//change and log the position of the cursor on the MAIN screen
  {
    n = digitalRead(encoderPinA); // make into a funciton with the number of lines as a parameter
    if ((encoderPinALast == LOW) && (n == HIGH)) 
    {
      if (digitalRead(encoderPinB) == LOW) 
      {
        delay(10);//UI tuning. Makes the display user's interaction lees choppy.
        //Counter Clockwise turn
        Cursor--;
        if(Cursor < 0)
        {
          Cursor = 0;
          delay(10);//UI tuning. Makes the display user's interaction lees choppy.
        }
        if(Cursor < 4)
        {
          if(Cursor == 3)
          {
            lcd.clear();
            for(int i = 0; i < 4; i++)
            {
              lcd.setCursor(2,i);
              lcd.print(MainScreen[i]);
            }
          }                                  //====================//
          lcd.setCursor(0,Cursor + 1);       //->Fault History     // 
          lcd.print(" ");                    //->Contact 1         //
          lcd.setCursor(1,Cursor + 1);       //->Contact 2         //
          lcd.print(" ");                    //->Contact 3         //
          lcd.setCursor(0,Cursor);           //->Contact 4         //
          lcd.print("-");                    //->Contact 5         //
          lcd.setCursor(1,Cursor);           //->Contact 6         //  
          lcd.print(">");                    //->EXIT              //
        }                                    //====================//
        else
        {
          lcd.setCursor(0,Cursor - 3);
          lcd.print(" ");
          lcd.setCursor(1,Cursor - 3);
          lcd.print(" ");
          lcd.setCursor(0,Cursor - 4);
          lcd.print("-");
          lcd.setCursor(1,Cursor - 4);
          lcd.print(">");
        }
      } 
      else 
      {
        delay(10);//UI tuning. Makes the display user's interaction lees choppy.
        //Clockwise turn
        Cursor++;
        if(Cursor > 7)
        {
          Cursor = 7;
        }
        if(Cursor < 4)
        {
          lcd.setCursor(0,Cursor - 1);
          lcd.print(" ");
          lcd.setCursor(1,Cursor - 1);
          lcd.print(" ");
          lcd.setCursor(0,Cursor);
          lcd.print("-");
          lcd.setCursor(1,Cursor);
          lcd.print(">");
        }
        else
        {
          if(Cursor == 4)
          {
            lcd.clear();
            for(int i = 0; i < 4; i++)
            {
              lcd.setCursor(2,i);
              lcd.print(MainScreen[i + 4]);
            }
          }
          lcd.setCursor(0,Cursor - 5);
          lcd.print(" ");
          lcd.setCursor(1,Cursor - 5);
          lcd.print(" ");
          lcd.setCursor(0,Cursor - 4);
          lcd.print("-");
          lcd.setCursor(1,Cursor - 4);
          lcd.print(">");
        }
      }
    }
   }
    encoderPinALast = n; 
   //==============================================================================================//
   //==============================================================================================//
   //======================================SUB MENU CODE===========================================//
   //==============================================================================================//
   //==============================================================================================//

  if(digitalRead(pushButton) == LOW && LCDTimerSwitch == false && userInput && !userInput2)
  {
    LCDdebounce = millis();
    LCDTimerSwitch = true;
    whichContactToEdit = 0;
  }
  if (LCDTimerSwitch && (millis() - LCDdebounce) > debounceDelay && userInput && !userInput2)
  { 
    userInput2 = true;
    LCDTimerSwitch = false;
    if(Cursor == 0) // TURN INTO A SWITCH CASE*************************************************************************************
    {
      faultHistory = true;
      EEPROMalarmPrint(Cursor, 0);
    }
    else if(Cursor == 1)
    {
      User_Input_Contact_Screen(contactScreen, 1, contact1, contact1Status, "CONTACT-1");
      whichContactToEdit = 1;
      //Cursor = 0; // Allowed for saving the cursor position when EXITing the contact menus
    }
    else if(Cursor == 2)
    {
      User_Input_Contact_Screen(contactScreen, 1, contact2, contact2Status, "CONTACT-2");
      whichContactToEdit = 2;
      //Cursor = 0;
    }
    else if(Cursor == 3)
    {
      User_Input_Contact_Screen(contactScreen, 1, contact3, contact3Status, "CONTACT-3");
      whichContactToEdit = 3;
      //Cursor = 0;
    }
    else if(Cursor == 4)
    {
      User_Input_Contact_Screen(contactScreen, 1, contact4, contact4Status, "CONTACT-4");
      whichContactToEdit = 4;
      //Cursor = 0;
    }
    else if(Cursor == 5)
    {
      User_Input_Contact_Screen(contactScreen, 1, contact5, contact5Status, "CONTACT-5");
      whichContactToEdit = 5;
      //Cursor = 0;
    }
    else if(Cursor == 6)
    {
      User_Input_Contact_Screen(contactScreen, 1, contact6, contact6Status, "CONTACT-6");
      whichContactToEdit = 6;
      //Cursor = 0;
    }
    else if(Cursor == 7)
    {
      userInput2 = false;
      userInput = false;
    }
  }

 //==============================================================================================//
 //================================FAULT HISTORY MENU FUNCTION===================================//
 //==============================================================================================//

  if(userInput && userInput2 && faultHistory)
  { 
    n = digitalRead(encoderPinA);
    if ((encoderPinALast == LOW) && (n == HIGH)) 
    {
      if (digitalRead(encoderPinB) == LOW) 
      {
        //Counter Clockwise turn
        EEPROMalarmPrint(Cursor, -1);
      } 
      else
      {
        //Clockwise turn
        EEPROMalarmPrint(Cursor, 1);
      }
    }
    encoderPinALast = n;
    // If user pushes the button again it will back out of the fault history menu. 
    if(digitalRead(pushButton) == LOW && LCDTimerSwitch == false)
    {
      LCDdebounce = millis();
      LCDTimerSwitch = true;
    }
    if (LCDTimerSwitch && (millis() - LCDdebounce) > debounceDelay)
    { 
      userInput3 = true;
      LCDTimerSwitch = false;
      userInput3 = false;
      userInput2 = false;
      faultHistory = false;
      Cursor = 0;
      User_Input_Main_Screen(Cursor);
    }
  }

 //==============================================================================================//
 //=====================================SUB MENU 2 CODE==========================================//
 //==============================================================================================//

  switch(whichContactToEdit)
  {
    case 0:break;
    case 1: Contact_Edit_Menu(contact1, "to1.txt", contact1Address, contact1Status, "CONTACT-1");
    break;
    case 2: Contact_Edit_Menu(contact2, "to2.txt", contact2Address, contact2Status, "CONTACT-2");
    break;
    case 3: Contact_Edit_Menu(contact3, "to3.txt", contact3Address, contact3Status, "CONTACT-3");
    break;
    case 4: Contact_Edit_Menu(contact4, "to4.txt", contact4Address, contact4Status, "CONTACT-4");
    break;
    case 5: Contact_Edit_Menu(contact5, "to5.txt", contact5Address, contact5Status, "CONTACT-5");
    break;
    case 6: Contact_Edit_Menu(contact6, "to6.txt", contact6Address, contact6Status, "CONTACT-6");
    break;
    default: break;
  } 
}

void Contact_Edit_Menu(char CONTACT[], char txtDOC[], int ADDRESS, bool& CONTACTSTATUS, char CONTACTNAME[])
{
  static unsigned long LCDdebounce2{};
  static bool LCDTimerSwitch2 {false};
  static int debounceDelay2{350};
  static int Cursor2{1};
  static int encoderPinALast2 {LOW}; 
  static int n2 {LOW};
  
  if(userInput && userInput2 && !faultHistory)// I dont think this if statement is needed
  { 
    n2 = digitalRead(encoderPinA);
    if ((encoderPinALast2 == LOW) && (n2 == HIGH)) 
    {
      if (digitalRead(encoderPinB) == LOW) 
      {
        //Counter Clockwise turn
        Cursor2--;
        delay(10);//UI tuning. Makes the display user's interaction lees choppy.
        if(Cursor2 < 1)
        {
          Cursor2 = 1;
          delay(10);//UI tuning. Makes the display user's interaction lees choppy. 
        }
        lcd.setCursor(0,Cursor2 + 1);
        lcd.print(" ");
        lcd.setCursor(1,Cursor2 + 1);
        lcd.print(" ");
        lcd.setCursor(0,Cursor2);
        lcd.print("-");
        lcd.setCursor(1,Cursor2);
        lcd.print(">");
      } 
      else 
      {
        //Clockwise turn
        Cursor2++;
        delay(10);//UI tuning. Makes the display user's interaction lees choppy.
        if(Cursor2 > 3)
        {
          Cursor2 = 3;
          delay(10);//UI tuning. Makes the display user's interaction lees choppy.
        }
        lcd.setCursor(0,Cursor2 - 1);
        lcd.print(" ");
        lcd.setCursor(1,Cursor2 - 1);
        lcd.print(" ");
        lcd.setCursor(0,Cursor2);
        lcd.print("-");
        lcd.setCursor(1,Cursor2);
        lcd.print(">");
      }
    }
    encoderPinALast2 = n2;
    
    if(digitalRead(pushButton) == LOW && LCDTimerSwitch2 == false)
    {
      LCDdebounce2 = millis();
      LCDTimerSwitch2 = true;
    }
    if (LCDTimerSwitch2 && (millis() - LCDdebounce2) > debounceDelay2)
    { 
      LCDTimerSwitch2 = false;
      if(Cursor2 == 1)// TURN CONTACT ON/OFF
      {
        CONTACTSTATUS = !CONTACTSTATUS;
        EEPROM_Save_Contact_Status(ADDRESS, CONTACTSTATUS);
        lcd.setCursor(10,1);
        lcd.print("        ");
        lcd.setCursor(10,1);
        if(CONTACTSTATUS)
        {
          lcd.print("ACTIVE");
        }
        else
        {
          lcd.print("INACTIVE");
        }
        strcpy(conToTotalArray,"To=");
        loadContacts();
      }
      else if(Cursor2 == 2)//EDIT CONTACT
      {
        lcd.setCursor(0,2);
        lcd.print("ENTER NEW#");
        lcd.setCursor(10,2);
        lcd.blink();
        Cursor2 = 10;
        Contact_Edit_Screen(contactScreen, 1, CONTACT, CONTACTSTATUS);
        userInput3 = true;
      }
      else if(Cursor2 == 3)//EXIT CONTACT
      {
        userInput3 = false;
        userInput2 = false;
        Cursor2 = 1;
        User_Input_Main_Screen(Cursor);
      }
    }
  }
  
  //==============================================================================================//
  //==============================CONTACT NUMBER EDITING CODE=====================================//
  //==============================================================================================//
 
  while(userInput3)
  {
    static int i {};
    static bool timerSwitch {false};
    static const unsigned long threemintimer {180000};  
    static unsigned long difference6 {};
    static unsigned long timerStart {};
    if (timerSwitch == false)
    {
      timerStart = millis();
      timerSwitch = true;
    }
    
    difference6 = millis() - timerStart;
    
    if (difference6 > threemintimer)      //IF TIMER IS UP
    {
      timerSwitch = false;
      userInput4 = false;
      userInput3 = false;
      Cursor2 = 1;
      i = 0;
      lcd.clear();
      lcd.setCursor(4,1);
      lcd.print("TIME OUT");
      lcd.noBlink();
      delay(5000);
      User_Input_Contact_Screen(contactScreen, 1, CONTACT, CONTACTSTATUS, CONTACTNAME);
      break;
    }
    static bool Switch {false};
    static const unsigned int oneSecTimer {1000};  
    static unsigned long difference7 {};
    static unsigned long Start {};
    
    if (Switch == false)
    {
      Start = millis();
      Switch = true;
    }
    
    difference7 = millis() - Start;
    
    if (difference7 > oneSecTimer)
    {
      int counter{};
      Switch = false;
      lcd.noBlink();
      counter = 180 - (difference6/1000);
      if(counter >= 100)
      {
        lcd.setCursor(16,2);
        lcd.print(counter);
      }
      else if(counter >= 10)
      {
        lcd.setCursor(16,2);
        lcd.print(counter);
      }
      else //(counter >= 0)
      {
        lcd.setCursor(16,2);
        lcd.print(counter);
      }
      if(counter == 99)
      {
        lcd.setCursor(18,2);
        lcd.print(" ");
      }
      if(counter == 9)
      {
        lcd.setCursor(17,2);
        lcd.print(" ");
      }
      lcd.blink();
      lcd.setCursor(Cursor2,1);
    }
    static char newContact[11];
    //if(newContact[10] != '\0')newContact[10] = '\0';
    static char newDigit{48};
    n2 = digitalRead(encoderPinA);
    if ((encoderPinALast2 == LOW) && (n2 == HIGH)) 
    {
      if (digitalRead(encoderPinB) == LOW) 
      {
        //Counter Clockwise turn
        newDigit--;                               
        if(newDigit < 48)                           //====================//
        {                                           //CONTACT-1 0123456789//
          newDigit =57;                             //->STATUS: ACTIVE    //
        }                                           //  EDIT#             //
        lcd.setCursor(Cursor2,1);                   //  EXIT              //
        lcd.print(newDigit);                        //====================// 
        lcd.setCursor(Cursor2,1);
      } 
      else //Clockwise turn
      {
        newDigit++;
        if(newDigit == 58)
        {
          newDigit = 48;
        }
        lcd.setCursor(Cursor2,1);
        lcd.print(newDigit);
        lcd.setCursor(Cursor2,1);
      }
    }
    encoderPinALast2 = n2;
    
    if(digitalRead(pushButton) == LOW && LCDTimerSwitch2 == false)
    {
      LCDdebounce2 = millis();
      LCDTimerSwitch2 = true;
    }
    if (LCDTimerSwitch2 && (millis() - LCDdebounce2) > debounceDelay2)
    { 
      LCDTimerSwitch2 = false;
        Cursor2++;
        if(Cursor2< 10)
        {
          Cursor2 = 19;
        }
        if(Cursor2 == 20)
        {
          Cursor2 = 10;
        }
        //static int i {}; THIS WAS REDECLARED HIGHER UP//////////////////////
        newContact[i] = newDigit;
        i++;
        newDigit = 48; //ASCII '48' == 0
        if(i < 10)
        {
          lcd.setCursor(Cursor2,1);
          lcd.print(newDigit);
          lcd.setCursor(Cursor2,1);
        }
        if(i == 10)
        {
          printf("Inside the if(i == 10) condition.\n");
          i = 0;
          userInput4 = true;
          lcd.noBlink();
          lcd.setCursor(0,2);
          lcd.print("-");
          lcd.setCursor(1,2);
          lcd.print(">");
          
    //=====================================================================================//          
    //============================Saving New Contact Menu==================================//  
    //=====================================================================================//  
    
          while(userInput4)
          {
            difference6 = millis() - timerStart;
            
            if (difference6 > threemintimer)    //IF TIMER IS UP
            {
              timerSwitch = false;
              userInput4 = false;
              userInput3 = false;
              Cursor2 = 1;
              i = 0;
              lcd.clear();
              lcd.setCursor(4,1);
              lcd.print("TIME OUT");
              lcd.noBlink();
              delay(5000);
              User_Input_Contact_Screen(contactScreen, 1, CONTACT, CONTACTSTATUS, CONTACTNAME);
              break;
            }
            if (Switch == false)
            {
              Start = millis();
              Switch = true;
            }
            
            difference7 = millis() - Start;
            
            if (difference7 > oneSecTimer)  // print the count down on the screen every sec
            {
              int counter{};
              Switch = false;
              counter = 180 - (difference6/1000);
              if(counter >= 100)
              {
                lcd.setCursor(16,2);
                lcd.print(counter);
              }
              else if(counter >= 10)
              {
                lcd.setCursor(18,2);
                lcd.print(" ");
                lcd.setCursor(16,2);
                lcd.print(counter);
              }
              else //(counter >= 0)
              {
                lcd.setCursor(17,2);
                lcd.print(" ");
                lcd.setCursor(16,2);
                lcd.print(counter);
              }
              //lcd.blink();
              //lcd.setCursor(Cursor2,1);
            }
            static int selector{};
            if(!stopRotaryEncoder)
            {
              delay(50);
              n2 = digitalRead(encoderPinA);
              if ((encoderPinALast2 == LOW) && (n2 == HIGH)) 
              {
                if (digitalRead(encoderPinB) == LOW) 
                {
                  //Counter Clockwise turn
                  selector--;
                  delay(20);                          //====================//
                  if(selector < 0)                    //CURRENT#  0000000000//
                  {                                   //ENTER NEW#          //
                    selector = 2;                     //->SAVE     TIME 180 //
                  }                                   //  REDO     EXIT     //
                }                                     //====================//
                else //Clockwise turn
                {
                  selector++;
                  delay(20);
                  if(selector == 3)
                  {
                    selector = 0;
                  }
                }
                switch(selector)
                {
                  case 0: lcd.setCursor(0,2);lcd.print("-");lcd.setCursor(1,2);lcd.print(">");
                          lcd.setCursor(0,3);lcd.print(" ");lcd.setCursor(1,3);lcd.print(" ");
                          lcd.setCursor(8,3);lcd.print(" ");lcd.setCursor(9,3);lcd.print(" ");
                  break;
                  case 1: lcd.setCursor(0,2);lcd.print(" ");lcd.setCursor(1,2);lcd.print(" ");
                          lcd.setCursor(0,3);lcd.print("-");lcd.setCursor(1,3);lcd.print(">");
                          lcd.setCursor(8,3);lcd.print(" ");lcd.setCursor(9,3);lcd.print(" ");
                  break;
                  case 2: lcd.setCursor(0,2);lcd.print(" ");lcd.setCursor(1,2);lcd.print(" ");
                          lcd.setCursor(0,3);lcd.print(" ");lcd.setCursor(1,3);lcd.print(" ");
                          lcd.setCursor(8,3);lcd.print("-");lcd.setCursor(9,3);lcd.print(">");
                  break;
                  default: 
                  break; 
                }
              }
              encoderPinALast2 = n2;
            }
            
            if(digitalRead(pushButton) == LOW && LCDTimerSwitch2 == false)
            {
              stopRotaryEncoder = true;
              LCDdebounce2 = millis();
              LCDTimerSwitch2 = true;
            }
            if (LCDTimerSwitch2 && (millis() - LCDdebounce2) > debounceDelay2)
            { 
              Serial.print(F("Free RAM = "));
              Serial.println(freeMemory());
              stopRotaryEncoder = false;
              LCDTimerSwitch2 = false;
                if(selector == 0)// SAVE NEW CONTACT 
                {
                  lcd.clear();
                  lcd.setCursor(6,1);
                  lcd.print("SAVING...");
                  lcd.setCursor(5,2);
                  lcd.print("PLEASE WAIT");
                  timerSwitch = false;
                  char confirmContact[15]{"To="};
                  strcpy(CONTACT,newContact);
                  strcat(confirmContact,newContact);
                  confirmContact[13] = '&';
                  confirmContact[14] = '\0';
                  Save_New_Contact(txtDOC, CONTACT);
                  strcpy(conToTotalArray,"To=");
                  loadContacts();
                  userInput4 = false;
                  userInput3 = false;
                  Cursor2 = 1;
                  sendSMS(urlHeaderArray, confirmContact, contactFromArray1, "Body=Contact%20Changed\"\r");
                  User_Input_Contact_Screen(contactScreen, 1, CONTACT, CONTACTSTATUS, CONTACTNAME);
                  delay(20);
                }
                else if(selector == 1)//REDO NEW CONTACT
                {
                  userInput4 = false;
                  lcd.setCursor(0,2);
                  lcd.print("ENTER NEW#");
                  lcd.setCursor(10,2);
                  lcd.blink();
                  Cursor2 = 10;
                  Contact_Edit_Screen(contactScreen, 1, CONTACT, CONTACTSTATUS);
                  timerStart = millis();
                }
                else if(selector == 2)//EXIT
                {
                  userInput4 = false;
                  userInput3 = false;
                  timerSwitch = false;
                  Cursor2 = 1;
                  User_Input_Contact_Screen(contactScreen, 1, CONTACT, CONTACTSTATUS, CONTACTNAME);
                }
             }
          }
        }
     }
  } 
}

void EEPROM_Save_Contact_Status(int ADDRESS, bool CONTACTSTATUS)
{
  EEPROM.write(ADDRESS,CONTACTSTATUS);
}

void Save_New_Contact(char txtDOC[], char NEWCONTACT[])
{
  printf("Save_New_Contact().\n");
  myFile = SD.open(txtDOC, O_WRITE);
  if (myFile) 
  {
    myFile.print(NEWCONTACT);
    myFile.close();
    printf("Save_New_Contact() Complete.\n");
  } 
  else
  {
    printf("Save_New_Contact() failed.\n");
  }
}

void memoryTest()
{
  int threshold = 3000;
  int memory = freeMemory();
  Serial.print(F("memoryTest() Free RAM = "));
  Serial.print(memory);
  Serial.println(F(" bytes "));
  if (memory < threshold)
  {
    sendSMS(urlHeaderArray, "To=%2b16158122833&", contactFromArray1, "Body=Memory%20Alert%20Possible%20Leak\"\r");
  }
}
