#include <SD.h>
#include <ModbusMaster.h>
#include <Wire.h> // Library for I2C communication
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <RTClib.h>
#include <LibPrintf.h>

//#define PRINTF_DISABLE_ALL
#define printf(x...)
//#define load_first_contact_number
#define load_second_contact_number
//#define load_third_contact_number

//The following const int pins are all pre-run in the PCB:
const int low1 {5};         //to screw terminal
const int low2 {6};         //to screw terminal
const int alarmPin {4};     //to screw terminal //honeywell alarm terminal (terminal 3 on honeywell programmer)
const int hplcIN {14};      //to screw terminal
const int hplcOUT {15};     //to screw terminal
const int MAX485_DE {3};    //to modbus module
const int MAX485_RE_NEG {2};//to modbus module
const int SIMpin {A3};      // this pin is routed to SIM pin 12 for boot (DF Robot SIM7000A module)
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
static char contactToArray1[25];
static char contactToArray2[25];
static char contactToArray3[25];

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

const int eepromAlarmDataSize = sizeof(alarmVariable); 

enum EEPROMAlarmCode {PLWCO = 1, SLWCO, FSGalarm, HighLimit};

//================INSTANTIATION================//

ModbusMaster node;
LiquidCrystal_I2C lcd(0x3F, 20, 4);
File myFile;
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
  delay(8000); // Give time for the SIM module to turn on and wake up
  loadContacts(); //run the SD card function.
  Serial.println(F("Contacts Loaded.  Booting SIM module.  Initiating wakeup sequence..."));
  delay(2000);
  initiateSim();
  Serial.println(F("Setup() Function complete. Entering Main Loop() Function"));
  lcd.clear();
  lcd.noBacklight();
}

//=======================================================================//
//============================= MAIN LOOP()==============================//
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
  LCDDisplayEEPROM();
}

//=======================================================================//
//=======================FUNCTION DECLARATIONS===========================//
//=======================================================================//

void EEPROMalarmInput(int ALARM)
{
  alarmVariable writingSTRUCT;
  static int inputCounter{};
  printf("EEPROM inputCounter at start of function = %i.\n\n", inputCounter);
  DateTime now = rtc.now();
  writingSTRUCT = {ALARM, now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second()};
  EEPROM.put(inputCounter, writingSTRUCT);
  inputCounter += eepromAlarmDataSize;
  printf("ALARM %i saved.\n", (inputCounter/eepromAlarmDataSize));
  printf("Alarm code: %i\n", ALARM);
  printf("Date: %i/%i/%i\n", writingSTRUCT.year, writingSTRUCT.month, writingSTRUCT.day);
  printf("Time: %i:%i:%i\n", writingSTRUCT.hour, writingSTRUCT.minute, writingSTRUCT.second);
  printf("inputCounter is now = %i.\n\n", inputCounter);
  if(inputCounter == eepromAlarmDataSize*10) 
  {
    inputCounter = 0;
    printf("\nREST inputCounter NOW.\n\n");
  }
}

void EEPROMalarmPrint(int& outputCounter, int upORdown)
{
  char buffer[20];
  alarmVariable readingSTRUCT;
//  static int outputCounter {};
  if(upORdown > 0)
  {
    outputCounter += eepromAlarmDataSize;
  }
  else if (upORdown < 0)
  {
    outputCounter -= eepromAlarmDataSize;
  }
  else outputCounter = 0;
  
  if(outputCounter < 0)
  {
    outputCounter = 0;
  }
  if(outputCounter == eepromAlarmDataSize*10) 
  {
    outputCounter = 0;
    printf("RESET outputCounter NOW.\n\n");
  }
  EEPROM.get(outputCounter, readingSTRUCT);
  printf("EEPROM ALARM %i is retrieved.\n", (outputCounter/eepromAlarmDataSize));
  printf("Alarm code: %i\n", readingSTRUCT.alarm);
  printf("Date: %i/%i/%i\n", readingSTRUCT.year,readingSTRUCT.month,readingSTRUCT.day);
  printf("Time: %i:%i:%i\n",readingSTRUCT.hour,readingSTRUCT.minute,readingSTRUCT.second);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.backlight();
  lcd.print("SAVED ALARM: ");
  lcd.print((outputCounter/eepromAlarmDataSize) + 1);
  lcd.setCursor(0, 1);
  switch(readingSTRUCT.alarm)
  {
    case 1: lcd.print(PrimaryString);break;
    case 2: lcd.print(SecondaryString);break;
    case 3: lcd.print(AlarmString);break;
    case 4: lcd.print(hlpcString);break;
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

void LCDDisplayEEPROM()
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
        EEPROMalarmPrint(LCDscreenPage, -1);
      } 
      else 
      {
        EEPROMalarmPrint(LCDscreenPage, 1);
      }
      Serial.println (LCDscreenPage);
    }
    encoderPinALast = n;
  } 
}

void print_alarms()
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
      Serial.println(F("Primary low water.  Sending message"));
      //sendSMS(urlHeaderArray, contactToArray1, contactFromArray1, LWbody);
       (urlHeaderArray, contactToArray2, contactFromArray1, LWbody);
      //sendSMS(urlHeaderArray, contactToArray3, contactFromArray1, LWbody);
      Serial.println(F("message sent or simulated"));
      EEPROMalarmInput(PLWCO);
      printf("EEPROM() function Primary Low Water complete.\n");
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
      Serial.println(F("Secondary low water.  Sending message."));
      //sendSMS(urlHeaderArray, contactToArray1, contactFromArray1, LW2body);
      sendSMS(urlHeaderArray, contactToArray2, contactFromArray1, LW2body);
      //sendSMS(urlHeaderArray, contactToArray3, contactFromArray1, LW2body);
      Serial.println(F("message sent or simulated"));
      EEPROMalarmInput(SLWCO);
      printf("EEPROM() function 2nd Low Water complete.\n");
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
      Serial.println(F("sending alarm message"));
      Serial.println(F("about to enter modbus reading function..."));
      //sendSMS(urlHeaderArray, contactToArray1, contactFromArray1, BCbody);
      sendSMS(urlHeaderArray, contactToArray2, contactFromArray1, BCbody);
      //sendSMS(urlHeaderArray, contactToArray3, contactFromArray1, BCbody);
      delay(100);
      Serial.println(F("about to enter modbus reading function..."));
      readModbus();
      Serial.println(F("message sent or simulated"));
      EEPROMalarmInput(FSGalarm);
      printf("EEPROM() function FSG Alarm complete.\n");
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
      Serial.println("Sending HPLC alarm message");
      //sendSMS(urlHeaderArray, contactToArray1, contactFromArray1, HLPCbody);
      sendSMS(urlHeaderArray, contactToArray2, contactFromArray1, HLPCbody);
      //sendSMS(urlHeaderArray, contactToArray3, contactFromArray1, HLPCbody);
      Serial.println(F("message sent or simulated"));
      EEPROMalarmInput(HighLimit);
      printf("EEPROM() function High Limit complete.\n");
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

void sendSMS(char pt1[], char pt2[], char pt3[], char pt4[])//SIM7000A module
{
  char finalURL[250] = "";
  strcpy(finalURL, pt1);
  strcat(finalURL, pt2);
  strcat(finalURL, pt3);
  strcat(finalURL, pt4);
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
    sendSMS(urlHeaderArray, contactToArray1, contactFromArray1, REPbody);
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
    Serial.print(incomingChar);
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
              sendSMS(urlHeaderArray, contactToArray1, contactFromArray1, CHECKbody);
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

void loadContacts()
{
//add an endpoint for data logging
// contacts to recieve text messages
static String conFrom1 = "";
static String conTo1 = "";
static String conTo2 = "";
static String conTo3 = "";
static String URLheader = "";
bool SDbegin {true};  
  while (SDbegin)  //what?*************************************************************************
  {
    if (!SD.begin(10)) Serial.println(F("SD card initialization failed! Remaining in SD.begin while loop"));
    else SDbegin = false;
  }
  Serial.println(F("SD card initialization done."));

  //------------------load FROM contact number-------------//  

  // This is the number boiler alert messages will appear to be from
  // also the number you text back to recieve info from the boiler.
  myFile = SD.open("from1.txt");
  if (myFile) 
  {
    Serial.println("pulling FROM phone number from SD");
    // read from the file until there's nothing else in it:
    while (myFile.available()) 
    {
      char c = myFile.read();  //gets one byte from serial buffer
      conFrom1 += c;
    }
    myFile.close();
  } 
  else 
  {
    // if the file didn't open, print an error:
    Serial.println("error opening from1.txt");
  }
  //convert the String into a character array
  conFrom1.toCharArray(contactFromArray1, 25);
  Serial.print(F("The first phone number FROM String is "));
  Serial.println(conFrom1);
  Serial.print(F("The first phone number FROM char array is "));
  Serial.println(contactFromArray1);

  //------------------load first contact number-------------//

#ifdef load_first_contact_number
  myFile = SD.open("to1.txt");
  if (myFile) 
  {
    Serial.println("pulling phone number 1 from SD");
    // read from the file until there's nothing else in it:
    while (myFile.available()) 
    {
      char c = myFile.read();  //gets one byte from serial buffer
      conTo1 += c;
    }
    myFile.close();
  } 
  else 
  {
    // if the file didn't open, print an error:
    Serial.println("error opening to1.txt");
  }
  conTo1.toCharArray(contactToArray1, 25);
  Serial.print(F("The first phone number TO String is "));
  Serial.println(conTo1);
  Serial.print(F("The first phone number TO char array is "));
  Serial.println(contactToArray1);
#endif

  //------------------load second contact number-------------//

#ifdef load_second_contact_number
  myFile = SD.open("to2.txt");
  if (myFile) 
  {
    Serial.println(F("pulling phone number 2 from SD"));
    // read from the file until there's nothing else in it:
    while (myFile.available()) 
    {
      char c = myFile.read();  //gets one byte from serial buffer
      conTo2 += c;
    }
    myFile.close();
  } 
  else 
  {
    // if the file didn't open, print an error:
    Serial.println(F("error opening to2.txt"));
  }
  conTo2.toCharArray(contactToArray2, 25);
  Serial.print(F("The second phone number TO String is "));
  Serial.println(conTo2);
  Serial.print(F("The second phone number TO char array is "));
  Serial.println(contactToArray2);
#endif

  //------------------load third contact number-------------//

#ifdef load_third_contact_number
  myFile = SD.open("to3.txt");
  if (myFile) 
  {
    Serial.println(F("pulling phone number 3 from SD"));
    // read from the file until there's nothing else in it:
    while (myFile.available()) 
    {
      char c = myFile.read();  //gets one byte from serial buffer
      conTo3 += c;
    }
    myFile.close();
  } 
  else 
  {
    // if the file didn't open, print an error:
    Serial.println(F("error opening to3.txt"));
  }
  conTo3.toCharArray(contactToArray3, 25);
  Serial.print(F("The third phone number TO String is "));
  Serial.println(conTo3);
  Serial.print(F("The third phone number TO char array is "));
  Serial.println(contactToArray3);
#endif

  //------------------load URL header-------------//

  // This is where the squawkbox sends data to the Twilio endpoint
  myFile = SD.open("URL.txt");
  if (myFile) 
  {
    Serial.println(F("loading URL header"));
    // read from the file until there's nothing else in it:
    while (myFile.available()) 
    {
      char c = myFile.read();  //gets one byte from serial buffer
      URLheader += c;
    }
    myFile.close();
  } 
  else 
  {
    // if the file didn't open, print an error:
    Serial.println(F("error opening URL.txt"));
  }
  URLheader.toCharArray(urlHeaderArray, 100);
  Serial.print(F("The URL header is "));
  Serial.println(URLheader);
  Serial.print(F("The URL header array is  "));
  Serial.println(urlHeaderArray);
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
      case 1:
        //sendSMS(urlHeaderArray, contactToArray1, contactFromArray1, "Body=Fault%20Code1%20No%20Purge%20Card\"\r");
        sendSMS(urlHeaderArray, contactToArray2, contactFromArray1, "Body=Fault%20Code1%20No%20Purge%20Card\"\r");
        break;
      case 15:
        //sendSMS(urlHeaderArray, contactToArray1, contactFromArray1, "Body=Code15%20Unexpected%20Flame\"\r");
        sendSMS(urlHeaderArray, contactToArray2, contactFromArray1, "Body=Code15%20Unexpected%20Flame\"\r");
        break;
      case 17:
        //sendSMS(urlHeaderArray, contactToArray1, contactFromArray1, "Body=Code17%20Main%20Flame%20Failure\"\r");
        sendSMS(urlHeaderArray, contactToArray2, contactFromArray1, "Body=Code17%20Main%20Flame%20Failure\"\r");
        break;
      case 19:
        //sendSMS(urlHeaderArray, contactToArray1, contactFromArray1, "Body=Code19%20MainIgn%20Failure\"\r");
        sendSMS(urlHeaderArray, contactToArray2, contactFromArray1, "Body=Code19%20MainIgn%20Failure\"\r");
        break;
      case 28:
       // sendSMS(urlHeaderArray, contactToArray1, contactFromArray1, "Body=Code28%20Pilot%20Failure\"\r");
        sendSMS(urlHeaderArray, contactToArray2, contactFromArray1, "Body=Code28%20Pilot%20Failure\"\r");
        break;
      case 29:
       // sendSMS(urlHeaderArray, contactToArray1, contactFromArray1, "Body=Code29%20Interlock\"\r");
        sendSMS(urlHeaderArray, contactToArray2, contactFromArray1, "Body=Code29%20Interlock\"\r");
        break;
      default:
       // sendSMS(urlHeaderArray, contactToArray1, contactFromArray1, "Body=Check%20Fault%20Code\"\r");
        sendSMS(urlHeaderArray, contactToArray2, contactFromArray1, "Body=Check%20Fault%20Code\"\r");
        break;
    }
  }
  else
  {
    //sendSMS(urlHeaderArray, contactToArray1, contactFromArray1, "Body=Modbus%20Com%20Fail\"\r");
    sendSMS(urlHeaderArray, contactToArray2, contactFromArray1, "Body=Modbus%20Com%20Fail\"\r");
  }
}

void SIMboot() // starts up the SIM module
{
  digitalWrite(SIMpin, HIGH);
  delay(3000);
  digitalWrite(SIMpin, LOW);
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
  //sendSMS(urlHeaderArray, contactFromArray1, contactToArray1, SetCombody);
  sendSMS(urlHeaderArray, contactFromArray1, contactToArray2, SetCombody);
  //sendSMS(urlHeaderArray, contactFromArray1, contactToArray3, SetCombody);
  delay(2000);
  Serial.println(F("initiateSim() complete."));
}
