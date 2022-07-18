// TODO:
// Change the Serial.print() to printf().
// Change Strings to char[].
// reassess how values are passed in UserInputAccessEEPROM().
// Add functions for Pump amps() / Aw Na box() / any others???
// Create function for User input Text to change personal phone number.
// Create function for User input Text to turn OFF or ON personal text messages.
// Cycle count function to divide the number of cycles by the last x number of hours to provide a current cycle rate.
// Display blow down reminder after 48 hours of no PLWCO.
// Send blow down text after 72 hours of no PLWCO.
// make an RTC function to clean up the setup()
// Add function to send a Flame signal text message

#include <SD.h>
#include <ModbusMaster.h>
#include <Wire.h> // Library for I2C communication
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <RTClib.h>
#include <LibPrintf.h>

//#define PRINTF_DISABLE_ALL // Not working as advertised
//#define printf(x...) // Deletes ALL prinf() functions 

//Rotary encoder definitons
#define CW 1
#define CCW -1
#define PUSH_BUTTON 0

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

const int debounceInterval {3000};//NEEDS TO BE MADE CHANGEABLE ON SD CARD
                                  // to prevent false alarms from electrical noise and also prevents repeat messages from bouncing PLWCOs.   
                                  // Setting this debounce too high will prevent the annunciation of instantaneous alarms like a bouncing LWCO.
const char PrimaryString[] {"Primary Low Water"};     // ====================================//
const char SecondaryString[] {"Secondary Low Water"}; // alarm text printed to the LCD screen//
const char hlpcString[] {"High Limit"};               // alarm text printed to the LCD screen//
const char AlarmString[] {"FSG Alarm"};               // ====================================//
const char DefaultString[] {"Open Fault Memory"};     // ====================================//
// message to be sent
const char SetCombody[] = "Body=Setup%20Complete\"\r";
const char LWbody[] = "Body=Low%20Water\"\r";
const char LW2body[] = "Body=Low%20Water2\"\r";
const char REPbody[] = "Body=Routine%20Timer\"\r";
const char HLPCbody[] = "Body=High%20Pressure%20Alarm\"\r";
const char CHECKbody[] = "Body=Good%20Check\"\r";
const char BCbody[] = "Body=Boiler%20Down\"\r";
// variables indicating whether or not an alarm message has been sent or not
static bool PLWCOSent{};
static bool SLWCOSent{};
static bool HWAlarmSent{};
static bool hlpcSent{};
static bool userInput = false; // when true display EEPROM faults
                               // when false display current faults
static char urlHeaderArray[100]; // Twilio end point url (twilio might changes this!)
// holds the phone number to recieve text messages
static char contactFromArray1[25];
static char conToTotalArray[60];

//=============User defined Data Types===================//

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

//EEPROM variables
const int numberOfSavedFaults {400};
const int eepromAlarmDataSize = sizeof(alarmVariable); 
static int EEPROMinputCounter{};
const uint8_t EEPROMInitializationKey {69};
const int EEPROMInitializationAddress {4020};
const int EEPROMinputCounterAddress {4000};
const int EEPROMLastFaultAddress {3600};
enum EEPROMAlarmCode {PLWCO = 1, SLWCO, FSGalarm, HighLimit};

//================INSTANTIATION================//

ModbusMaster node;
LiquidCrystal_I2C lcd(0x3F, 20, 4);
File myFile; // consider DateTime now = rtc.now() instansiation only once in the loop??? Heap memory concerns??
RTC_PCF8523 rtc;

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
     Serial.println("Couldn't find RTC");
     Serial.flush();
     abort();
   }
  if (! rtc.initialized() || rtc.lostPower()) 
   {
     Serial.println("RTC is NOT initialized, let's set the time!");
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

  lcd.init(); // LCD initialization
  lcd.backlight();
  lcd.begin(20, 4); // initialize LCD screen (columns, rows)
  lcd.setCursor(2, 1);
  lcd.print("AB3D Squawk Box");
  delay(2000);
  
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
  printf("Starting SIMboot().\n");
  SIMboot();
  boot_SD();
  loadContacts(); //run the SD card function.
  Serial.println(F("Contacts Loaded.  Booting SIM module.  Initiating wakeup sequence..."));
  initiateSim();
  EEPROM_Prefill();
  Serial.println(F("Setup() Function complete. Entering Main Loop() Function"));
  lcd.clear();
  lcd.noBacklight();
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
  UserInputAccessEEPROM();
  //printf("%u\n",millis());
}


//=======================================================================//
//=======================================================================//
//====================== FUNCTION DECLARATIONS ==========================//
//=======================================================================//
//=======================================================================//


void EEPROMalarmInput(int ALARM) // we are inputing into the EEPROM NOT the program
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
    printf("\nREST EEPROMinputCounter NOW.\n\n");
  }
  EEPROM.put(EEPROMinputCounterAddress, EEPROMinputCounter);
}

void EEPROMalarmPrint(int& outputCounter, int encoderTurnDirection)
{
  char buffer[20];
  static int SavedAlarmCounter {};// The number displayed on the LCD screen after "SAVED ALARM:" (1 indicates the most recent saved alarm)
  alarmVariable readingSTRUCT;
  
// Logic for determining if the user is turning the knob CW, CCW, or just pushed the button
  if(encoderTurnDirection == CW)//User turned the dial ClockWise
  {
    outputCounter -= eepromAlarmDataSize;//outputCounter will always be one eepromAlarmDataSize less than the EEPROMinputCounter
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
    printf("RESET outputCounter NOW.\n\n");
  }
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

void UserInputAccessEEPROM()
{
  static int LCDscreenPage {};
  static int encoderPinALast {LOW};
  static int n {LOW};
  static unsigned long LCDdebounce{};
  static int debounceDelay{350};
  static bool LCDTimerSwitch {false};

  // Using a timer to latch the momentary user input into a constant ON or OFF position
  // LCDTimerSwitch is used to allow main loop to continue and LCDdebounce to only be set to millis() once per pushButton push
  if(digitalRead(pushButton) == LOW && LCDTimerSwitch == false)
  {
    LCDdebounce = millis();
    LCDTimerSwitch = true;
  }
  /*  Once userInput has been recieved and the debounce time has passed we ! the userInput bool and turn 
   *  off the LCDTimerSwitch to stop running through the timer code until a new userinput is recieved.*/
  if (LCDTimerSwitch && (millis() - LCDdebounce) > debounceDelay)
  {
    userInput = !userInput;
    LCDscreenPage = 0;
    LCDTimerSwitch = false;
    if(userInput)
    {
      Serial.println("User Input Recieved ON");
      EEPROMalarmPrint(LCDscreenPage, 0);
    }
    else Serial.println ("User Input Recieved OFF");
  }
  /*  The userInput bool is used as a latch so that the LCD screen continues to display what the user wants until the pushButton 
   *  is activated again. When the above timer algorithm has been satisfied the squawk will begin tracking rotational rotary 
   *  encoder input from the user in order to traverse and LCD display the EEPROM fault data.*/
  if(userInput)
  {
    n = digitalRead(encoderPinA);
    if ((encoderPinALast == LOW) && (n == HIGH)) 
    {
      if (digitalRead(encoderPinB) == LOW) 
      {
        //Counter Clockwise turn
        EEPROMalarmPrint(LCDscreenPage, -1);
      } 
      else 
      {
        //Clockwise turn
        EEPROMalarmPrint(LCDscreenPage, 1);
      }
    }
    encoderPinALast = n;
  } 
}

void print_alarms()//rethink how this function is layed out and structured...
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
      Serial.println("alarmSwitch is true");
    }
    difference = millis() - alarmTime;

    if ( difference >= debounceInterval)
    {
      LCDwaiting();
      Serial.println(F("Primary low water.  Sending message"));
      sendSMS(urlHeaderArray, conToTotalArray, contactFromArray1, LWbody);
      Serial.println(F("message sent or simulated"));
      EEPROMalarmInput(PLWCO);
      printf("EEPROM() function Primary Low Water complete.\n");
      Data_Logger(PrimaryString);
      printf("Data_Logger(PrimaryString)function complete.\n");
      PLWCOSent = 1;
      alarmSwitch = false;
    }
    else Serial.println(difference); 
  }
  else
  {
    if(!primaryCutoff && PLWCOoutlet)
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
      Serial.println(F("alarmSwitch2 is true"));
    }
    difference2 = millis() - alarmTime2;

    if ( difference2 >= debounceInterval)
    {
      LCDwaiting();
      Serial.println(F("Secondary low water.  Sending message."));
      sendSMS(urlHeaderArray, conToTotalArray, contactFromArray1, LW2body);
      Serial.println(F("message sent or simulated"));
      EEPROMalarmInput(SLWCO);
      printf("EEPROM() function 2nd Low Water complete.\n");
      Data_Logger(SecondaryString);
      printf("Data_Logger(SecondaryString)function complete.\n");
      SLWCOSent = 1;
      alarmSwitch2 = false;
    }
    if (difference2 < debounceInterval)
    {
      Serial.println(difference2);
    }
  }
  else
  {
    if(!secondaryCutoff && SLWCOoutlet)
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
      Serial.println("HW alarmSwitch is true");
    }
    
    difference3 = millis() - alarmTime3;

    if ( difference3 >= debounceInterval)
    {
      LCDwaiting();
      Serial.println(F("sending alarm message"));
      Serial.println(F("about to enter modbus reading function..."));
      sendSMS(urlHeaderArray, conToTotalArray, contactFromArray1, BCbody);
      delay(100);
      Serial.println(F("about to enter modbus reading function..."));
      readModbus();
      Serial.println(F("message sent or simulated"));
      EEPROMalarmInput(FSGalarm);
      printf("EEPROM() function FSG Alarm complete.\n");
      Data_Logger(AlarmString);
      printf("Data_Logger(AlarmString)function complete.\n");
      HWAlarmSent = 1;
      alarmSwitch3 = false;
    }
    if (difference3 < debounceInterval)
    {
      Serial.println(difference3);
    }
  }
  else
  {
    if(alarm == LOW)
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
      Serial.println("alarmSwitch is true");
    }
    difference4 = millis() - alarmTime4;

    if ( difference4 >= debounceInterval)
    {
      LCDwaiting();
      Serial.println("Sending HPLC alarm message");
      sendSMS(urlHeaderArray, conToTotalArray, contactFromArray1, HLPCbody);
      Serial.println(F("message sent or simulated"));
      EEPROMalarmInput(HighLimit);
      printf("EEPROM() function High Limit complete.\n");
      Data_Logger(hlpcString);
      printf("Data_Logger(hlpcString)function complete.\n");
      hlpcSent = 1;
      alarmSwitch4 = false;
    }
    if (difference4 < debounceInterval)
    {
      Serial.println(difference4);
    }
  }
  else
  {
    if (hlpcNC)
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
  //getResponse();
  Serial1.print("AT+SAPBR=3,1,\"APN\",\"super\"\r");
  delay(300);
  //getResponse();
  Serial1.print("AT+SAPBR=1,1\r");
  delay(1000);
  //getResponse();
  Serial1.print("AT+HTTPINIT\r");
  delay(100);
  //getResponse();
  Serial1.print("AT+HTTPPARA=\"CID\",1\r");
  delay(100);
  //getResponse();
  Serial1.println(finalURL);
  delay(100);
  //getResponse();
  Serial1.print("AT+HTTPACTION=1\r");
  delay(4000);
  //getResponse();
}

void getResponse() // serial monitor printing for troubleshooting
{
unsigned char data {};
  while (Serial1.available())
  {
    data = Serial1.read();
    Serial.write(data);
    delay(5);
  }
}

void timedmsg() // daily timer message to ensure Squawk is still operational
{
  static bool msgswitch {false};
  static const unsigned long fifmintimer {900000};
  static const unsigned long fivmintimer {300000};
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
    sendSMS(urlHeaderArray, conToTotalArray, contactFromArray1, REPbody);
    msgswitch = false;
  }
}

void SMSRequest()//SIM7000A module // maybe use a while loop but need to fix the random letters that are recieved
{
  char incomingChar {};
  //add message for changing the operators phone number
  if (Serial1.available() > 0) 
  {
    incomingChar = Serial1.read();
    if (incomingChar == 'C') 
    {
      delay(100);
      Serial.print(incomingChar);
      incomingChar = Serial1.read();
      if (incomingChar == 'H') 
      {
        delay(100);
        Serial.print(incomingChar);
        incomingChar = Serial1.read();
        if (incomingChar == 'E') 
        {
          delay(100);
          Serial.print(incomingChar);
          incomingChar = Serial1.read();
          if (incomingChar == 'C') 
          {
            delay(100);
            Serial.print(incomingChar);
            incomingChar = Serial1.read();
            if (incomingChar == 'K') 
            {
              delay(100);
              Serial.print(incomingChar);
              Serial.println(F("GOOD CHECK. SMS SYSTEMS ONLINE"));
              Serial.println(F("SENDING CHECK VERIFICATION MESSAGE")) ;
              sendSMS(urlHeaderArray, conToTotalArray, contactFromArray1, CHECKbody);
              Serial.println("verification message sent");
              Serial1.print("AT+CMGD=0,4\r");
              delay(100);
            }
          }
        }
      }
    }
  }
}

void Data_Logger(const char FAULT[])//add in cycle count and run time hours?
{
  DateTime now = rtc.now();
  File myFile = SD.open("DataLog.txt", FILE_WRITE);
  if (myFile) 
  {
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
    printf("Data Log failed.\n");
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
    //const char* FILE {"DataLog.txt"}; //Arduino file names can only be <= 8 characters
    myFile = SD.open("DataLog.txt", FILE_WRITE);
    if (myFile) 
    {
      if(myFile.position() == 0)
      {
        myFile.println("Fault, UnixTime, Year, Month, Day, Hour, Minute, Second");
        myFile.close();
        printf("SD Card initialization complete.\n");
      }
      else
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
        myFile.close();    //Close the file
        printf("SD Prior initialization recognition & SystemRESET Data Log Complete.\n");
      }
    }
    else
    {
      printf("***ERROR SD Prior initialization recognition OR SystemRESET Data Log Failed.***\n");
    }
  }
}

void loadContacts()
{
  String URLheader = "";
  String conFrom1 = "";
  String conTo1 = "";
  String conTo2 = "";
  String conTo3 = "";
  String conToTotal = "To=";
  //load "from" number.  This is the number alert messages will apear to be from.
  conFrom1 = fill_from_SD("from1.txt");
  conFrom1.toCharArray(contactFromArray1, 25);
  Serial.print("From nums: ");
  Serial.print(contactFromArray1);
  conTo1 = fill_from_SD("To1.txt");
  if (conTo1[0] > 0) 
  {
    conToTotal += conTo1;
  }
  conTo2 = fill_from_SD("To2.txt");
  if (conTo2[0] > 0) 
  {
    conToTotal += "," + conTo2;
  }
  conTo3 = fill_from_SD("To3.txt");
  if (conTo3[0] > 0) 
  {
    conToTotal += "," + conTo3;
  }
  conToTotal += "&";    //format the "to" list of numbers for being in the URL by ending it with '&' so that next parameter can come after
  Serial.print(F("The total contact list is: "));
  Serial.println(conToTotal);
  Serial.print("fourth position character: ");
  Serial.println(conToTotal[3]);
  if (conToTotal[3] == ',')
  {
    conToTotal.remove(3, 1);
    Serial.print(F("New contact list: "));
    Serial.println(conToTotal);
  }
  conToTotal.toCharArray(conToTotalArray, 60);
  URLheader = fill_from_SD("URL.txt");
  URLheader.toCharArray(urlHeaderArray, 100);
  Serial.print("URL header is: ");
  Serial.println(urlHeaderArray);
}

String fill_from_SD(String file_name)
{
  String temporary_string = "";
  String info_from_SD = "";
  myFile = SD.open(file_name);
  if (myFile) 
  {
    // read from the file until there's nothing else in it:
    while (myFile.available())
    {
      char c = myFile.read();  //gets one byte from serial buffer
      info_from_SD += c;
    }
    myFile.close();
    return info_from_SD;
  }
  else
  {
    // if the file didn't open, print an error:
    Serial.println("***ERROR opening SD contactTo file.***");
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

void readModbus() // getting FSG faults from the FSG
{
  Serial.println("In the readModbus() function now");
  delay(300);
  uint16_t result = node.readHoldingRegisters (0x0000, 1);

  if (result == node.ku8MBSuccess) // ku8MBSuccess == 0x00
  {
    int alarmRegister = node.getResponseBuffer(result);
    Serial.print("Register response:  ");
    Serial.println(alarmRegister);

    switch (alarmRegister)
    {
      case 1: sendSMS(urlHeaderArray, conToTotalArray, contactFromArray1, "Body=Fault%20Code1%20No%20Purge%20Card\"\r");
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

void SIMboot()
{
//This function only boots the SIM module if it needs to be booted
//This prevents nuisance power-downs upon startup
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

void initiateSim()
{
  //The remainder of the setup is for waking the SIM module, logging on, and sending the first
  //test message to verify proper booting.
  printf("Hey!  Wake up!\n");
  Serial1.print("AT\r"); //Toggling this blank AT command elicits a mirror response from the
  //SIM module and helps to activate it.
  //getResponse();
  Serial1.print("AT\r");
  delay(50);
  //getResponse();
  Serial1.print("AT\r");
  delay(50);
  //getResponse();
  Serial1.print("AT\r");
  delay(50);
  //getResponse();
  Serial1.print("AT\r");
  delay(50);
  //getResponse();

  //========   SIM MODULE SETUP   =======//

  Serial1.print("AT+CGDCONT=1,\"IP\",\"super\"\r");//"super" is the key required to log onto the network using Twilio SuperSIM
  delay(500);
  //getResponse();
  Serial1.print("AT+COPS=1,2,\"310410\"\r"); //310410 is AT&T's network code https://www.msisdn.net/mccmnc/310410/
  delay(5000);
  //getResponse();
  Serial1.print("AT+SAPBR=3,1,\"APN\",\"super\"\r"); //establish SAPBR profile.  APN = "super"
  delay(3000);
  //getResponse();
  Serial1.print("AT+SAPBR=1,1\r");
  delay(2000);
  //getResponse();
  Serial1.print("AT+CMGD=0,4\r"); //this line deletes any existing text messages to ensure
  //that the message space is empy and able to accept new messages
  delay(100);
  //getResponse();
  Serial1.print("AT+CMGF=1\r");
  delay(100);
  //getResponse();
  Serial1.print("AT+CNMI=2,2,0,0,0\r"); //
  delay(100);
  //getResponse();
  sendSMS(urlHeaderArray, contactFromArray1, conToTotalArray, SetCombody);
  delay(2000);
  Serial.println(F("initiateSim() complete."));
}

void LCDwaiting()
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
  if(userInput)
  {
    userInput = false; /*If the user is accessing the EEPROM memory faults while a new fault occurs, 
                        this will kick them out of that menu and allow for the display of the new fault.*/
  }
}

void EEPROM_Prefill()// EEPROM initialization function
{
  // Do we need redundancy here? 
  if(EEPROM.read(EEPROMInitializationAddress) == EEPROMInitializationKey) //If this is true then the EEPROM has already been initialized.  
  {
    printf("\n***EEPROM has been previously initialized.***\n");
    EEPROM.get(EEPROMinputCounterAddress, EEPROMinputCounter);
  }
  else 
  {
    printf("\n***EEPROM is uninitialized... PreFilling EEPROM Faults now.***\n\n");
    EEPROM.write(EEPROMInitializationAddress, EEPROMInitializationKey);
    EEPROM.put (EEPROMinputCounterAddress, EEPROMinputCounter);
    const alarmVariable prefillSTRUCT = {255, 1111, 11, 11, 11, 11, 11};
    for (int i = 0; i < EEPROMLastFaultAddress; i += eepromAlarmDataSize)
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
    flameSignal = map(flameSignal,0,255,0,25.5);//As per the S7800A1146 Display manual 
    return flameSignal;
  }
  else
  {
    printf("***ERROR Flame signal retrival failed.***\n");
    return -1.0;
  }
}
