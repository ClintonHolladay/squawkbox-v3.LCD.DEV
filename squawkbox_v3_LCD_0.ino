#include <SD.h>
#include <ModbusMaster.h>
#include <Wire.h> // Library for I2C communication
#include <LiquidCrystal_I2C.h>

//The following const int pins are all pre-run in the PCB:
const int low1 = 5;  //to screw terminal
const int low2 = 6;   //to screw terminal
const int alarmPin = 4;   //to screw terminal //honeywell alarm terminal (terminal 3 on honeywell programmer)
const int hplcIN {14};
const int hplcOUT {15};
const int MAX485_DE = 3;  //to modbus module
const int MAX485_RE_NEG = 2;   //to modbus module
const int SIMpin = A3;  // this pin is routed to SIM pin 12 for boot (DF Robot SIM7000A module)

// Pins added for LCD development. These allow the LCD to reset after the alarm has been reset by the boiler operator
const int PLWCOoutletPin {17};
const int SLWCOoutletPin {16};

// to prevent false alarms from electrical noise and also prevents repeat messages from trivial PLWCOs.   
// Setting this debounce too high will prevent the annunciation of instantaneous alarms like a bouncing LWCO.
const int debounceInterval = 3000; //NEEDS TO BE MADE CHANGEABLE ON SD CARD

unsigned long currentMillis {}; // may not be needed

// debounce timer math
unsigned long difference {};
unsigned long difference2 {};
unsigned long difference3 {};
unsigned long difference4 {};
unsigned long difference5 {};
unsigned long ClearScreendifference {};
unsigned long alarmTime {};
unsigned long alarmTime2 {};
unsigned long alarmTime3 {};
unsigned long alarmTime4 {};
unsigned long ClearScreenTime {};
unsigned long msgtimer1 = 0;

// messages based on time
unsigned long fifmintimer = 900000;
unsigned long fivmintimer = 300000;
unsigned long dailytimer = 86400000;

// variables indicating whether or not an alarm message has been sent or not
bool PLWCOSent{};
bool SLWCOSent{};
bool HWAlarmSent{};
bool hlpcSent{};

// state-reading variables
bool alarm{};
bool primaryCutoff{};
bool secondaryCutoff{};
bool hlpcCOMMON{};
bool hlpcNC{};

// Pins added for LCD development. These allow the LCD to reset after the alarm has been reset by the boiler operator 
bool PLWCOoutlet{}; 
bool SLWCOoutlet{};

// variables storing whether the particular timer been activated or not? 
bool alarmSwitch = false;
bool alarmSwitch2 = false;
bool alarmSwitch3 = false;
bool alarmSwitch4 = false;
bool msgswitch = false;

// Pins added for LCD development. These allow the LCD to reset after the alarm has been reset by the boiler operator
bool ClearScreenSwitch {};

// alarm text printed to the LCD screen
const String PrimaryString {"Primary Low Water"};
const String SecondaryString {"Secondary Low Water"};
const String hlpcString {"High Limit"};
const String AlarmString {"FSG Alarm"};

// Twilio end point url (twilio might changes this!)
//String URLheader = "";
char urlHeaderArray[100];

// contacts to recieve text messages
// String conFrom1 = "";
// String conTo1 = "";
// String conTo2 = "";
// String conTo3 = "";

// holds the phone number to recieve text messages
char contactFromArray1[25];
char contactToArray1[25];
char contactToArray2[25];
char contactToArray3[25];

// message to be sent
char SetCombody[] = "Body=Setup%20Complete\"\r";
char LWbody[] = "Body=Low%20Water\"\r";
char LW2body[] = "Body=Low%20Water2\"\r";
char REPbody[] = "Body=Routine%20Timer\"\r";
char HLPCbody[] = "Body=High%20Pressure%20Alarm\"\r";
char CHECKbody[] = "Body=Good%20Check\"\r";
char BCbody[] = "Body=Boiler%20Down\"\r";

// in the SMS request function to recieve text messages from the user
char incomingChar;

ModbusMaster node;
LiquidCrystal_I2C lcd(0x3F, 20, 4);
File myFile;

//=======================================================================

void setup() 
{
  Serial.begin(9600);
  Serial1.begin(19200);
  Serial.println(F("This is squawkbox V3.LCD.0 sketch."));
  // LCD initialization
  lcd.init();
  lcd.backlight();
  lcd.begin(20, 4); // initialize LCD screen (columns, rows)
  lcd.setCursor(2, 1);
  lcd.print("AB3D Squawk Box");
  delay(2000);
  lcd.clear();
  lcd.noBacklight();
  
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

  node.begin(1, Serial);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  SIMboot();
  delay(8000); // Give time for the SIM module to turn on and wake up
  loadContacts(); //run the SD card function.
  Serial.println(F("Contacts Loaded.  Booting SIM module.  Initiating wakeup sequence..."));
  delay(2000);
  initiateSim();
  Serial.println(F("Setup() Function complete. Entering Main Loop() Function"));
}

//=========================================================================

void loop()
{
  primaryCutoff = digitalRead(low1);
  secondaryCutoff = digitalRead(low2);
  alarm = digitalRead(alarmPin);
  hlpcCOMMON = digitalRead(hplcIN);
  hlpcNC = digitalRead(hplcOUT);
  PLWCOoutlet = digitalRead(PLWCOoutletPin);
  SLWCOoutlet = digitalRead(SLWCOoutletPin);
  currentMillis = millis(); //??why isnt this used in the funtcions??

  if (ClearScreenSwitch == false)
    {
      ClearScreenTime = millis(); 
      ClearScreenSwitch = true;
    }
    
  ClearScreendifference = millis() - ClearScreenTime;

  if ( ClearScreendifference >= debounceInterval)
    {
      lcd.clear();
      print_alarms();
      ClearScreendifference = 0;
      ClearScreenSwitch = false;
      //ClearScreenTime = 0;
    }
  primary_LW();
  secondary_LW();
  Honeywell_alarm();
  HPLC();
  timedmsg();
  SMSRequest();
}

void print_alarms()
{
  if (!PLWCOSent && !SLWCOSent && !HWAlarmSent && !hlpcSent)
  {
    //lcd.clear();
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
}

void primary_LW()
{
  if ((primaryCutoff == HIGH) && (PLWCOSent == 0))
  {
    if (alarmSwitch == false)
    {
      alarmTime = currentMillis; 
      alarmSwitch = true;
      Serial.println("alarmSwitch is true");
    }
    difference = currentMillis - alarmTime;

    if ( difference >= debounceInterval)
    {
      Serial.println(F("Primary low water.  Sending message"));
      //sendSMS(urlHeaderArray, contactToArray1, contactFromArray1, LWbody);
      sendSMS(urlHeaderArray, contactToArray2, contactFromArray1, LWbody);
      //sendSMS(urlHeaderArray, contactToArray3, contactFromArray1, LWbody);
      Serial.println(F("message sent or simulated"));
      PLWCOSent = 1;
      difference = 0;
      alarmSwitch = false;
      alarmTime = 0;
    }
    else Serial.println(difference); 
  }
  else
  {
    if(!primaryCutoff && PLWCOoutlet)
    {
      alarmSwitch = false;
      difference = 0;
      alarmTime = 0;
      PLWCOSent = 0;
    }
  }
}

void secondary_LW()
{
  if ((secondaryCutoff == HIGH) && (SLWCOSent == 0))
  {
    if (alarmSwitch2 == false)
    {
      alarmTime2 = currentMillis;
      alarmSwitch2 = true;
      Serial.println(F("alarmSwitch2 is true"));
    }
    difference2 = currentMillis - alarmTime2;

    if ( difference2 >= debounceInterval)
    {
      Serial.println(F("Secondary low water.  Sending message."));
      //sendSMS(urlHeaderArray, contactToArray1, contactFromArray1, LW2body);
      sendSMS(urlHeaderArray, contactToArray2, contactFromArray1, LW2body);
      //sendSMS(urlHeaderArray, contactToArray3, contactFromArray1, LW2body);
      Serial.println(F("message sent or simulated"));
      SLWCOSent = 1;
      difference2 = 0;
      alarmSwitch2 = false;
      alarmTime2 = 0;
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
      difference2 = 0;
      alarmTime2 = 0;
      SLWCOSent = 0;
    }
  }
}
void Honeywell_alarm()
{
  if (alarm == HIGH && HWAlarmSent == 0)
  {
    if (alarmSwitch3 == false)
    {
      alarmTime3 = currentMillis;
      alarmSwitch3 = true;
      Serial.println("HW alarmSwitch is true");
    }
    
    difference3 = currentMillis - alarmTime3;

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
      HWAlarmSent = 1;
      difference3 = 0;
      alarmSwitch3 = false;
      alarmTime3 = 0;
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
      difference3 = 0;
      alarmTime3 = 0;
      HWAlarmSent = 0;
    }
  }
}

void HPLC()
{
  if ((hlpcCOMMON == HIGH) && (hlpcNC == LOW) && (hlpcSent == 0))
  {
    if (alarmSwitch4 == false)
    {
      alarmTime4 = currentMillis;
      alarmSwitch4 = true;
      Serial.println("alarmSwitch is true");
    }
    difference4 = currentMillis - alarmTime4;

    if ( difference4 >= debounceInterval)
    {
      Serial.println("Sending HPLC alarm message");
      //sendSMS(urlHeaderArray, contactToArray1, contactFromArray1, HLPCbody);
      sendSMS(urlHeaderArray, contactToArray2, contactFromArray1, HLPCbody);
      //sendSMS(urlHeaderArray, contactToArray3, contactFromArray1, HLPCbody);
      Serial.println(F("message sent or simulated"));
      hlpcSent = 1;
      difference4 = 0;
      alarmSwitch4 = false;
      alarmTime4 = 0;
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
      difference4 = 0;
      alarmTime4 = 0;
      hlpcSent = 0;
    }
  }
}

void sendSMS(char pt1[], char pt2[], char pt3[], char pt4[])
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
  getResponse();
  Serial1.print("AT+SAPBR=3,1,\"APN\",\"super\"\r");
  delay(300);
  getResponse();
  Serial1.print("AT+SAPBR=1,1\r");
  delay(1000);
  getResponse();
  Serial1.print("AT+HTTPINIT\r");
  delay(100);
  getResponse();
  Serial1.print("AT+HTTPPARA=\"CID\",1\r");
  delay(100);
  getResponse();
  Serial1.println(finalURL);
  delay(100);
  getResponse();
  Serial1.print("AT+HTTPACTION=1\r");
  delay(5000);
  getResponse();
}

void getResponse()
{
unsigned char data {};
  while (Serial1.available())
  {
    data = Serial1.read();
    Serial.write(data);
    delay(5);
  }
}

void timedmsg()
{
  if (msgswitch == false)
  {
    msgtimer1 = currentMillis;
    msgswitch = true;
  }
  difference5 = currentMillis - msgtimer1;

  if (difference5 >= dailytimer)
  {
    sendSMS(urlHeaderArray, contactToArray1, contactFromArray1, REPbody);
    difference5 = 0;
    msgswitch = false;
    msgtimer1 = 0;
  }
}

void SMSRequest() // maybe use a while loop but need to fix the random letters that are recieved
{
  //add message for changing the operators phone number
  delay(100);
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
//add an endpoint for data logging
{
// contacts to recieve text messages
String conFrom1 = "";
String conTo1 = "";
String conTo2 = "";
String conTo3 = "";
String URLheader = "";
bool SDbegin {true};  
  while (SDbegin)
  {
    if (!SD.begin(10)) Serial.println(F("SD card initialization failed! Remaining in SD.begin while loop"));
    else SDbegin {false};
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
  
  //------------------load second contact number-------------//

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

  //------------------load third contact number-------------//

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
{
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
        sendSMS(urlHeaderArray, contactToArray1, contactFromArray1, "Body=Fault%20Code1%20No%20Purge%20Card\"\r" );
        break;
      case 15:
        sendSMS(urlHeaderArray, contactToArray1, contactFromArray1, "Body=Code15%20Unexpected%20Flame\"\r" );
        break;
      case 17:
        sendSMS(urlHeaderArray, contactToArray1, contactFromArray1, "Body=Code17%20Main%20Flame%20Failure\"\r" );
        break;
      case 19:
        sendSMS(urlHeaderArray, contactToArray1, contactFromArray1, "Body=Code19%20MainIgn%20Failure\"\r" );
        break;
      case 28:
        sendSMS(urlHeaderArray, contactToArray1, contactFromArray1, "Body=Code28%20Pilot%20Failure\"\r" );
        break;
      case 29:
        sendSMS(urlHeaderArray, contactToArray1, contactFromArray1, "Body=Code29%20Interlock\"\r" );
        break;
      default:
        sendSMS(urlHeaderArray, contactToArray1, contactFromArray1, "Body=Check%20Fault%20Code\"\r" );
        break;
    }
  }
  else
  {
    sendSMS(urlHeaderArray, contactToArray1, contactFromArray1, "Body=Modbus%20Com%20Fail\"\r");
  }
}

void SIMboot()
{
  digitalWrite(SIMpin, HIGH);
  delay(3000);
  digitalWrite(SIMpin, LOW);
}

void initiateSim()
{
  //The remainder of the setup is for waking the SIM module, logging on, and sending the first
  //test message to verify proper booting.
  Serial.println("Hey!  Wake up!");
  Serial1.print("AT\r"); //Toggling this blank AT command elicits a mirror response from the
  //SIM module and helps to activate it.
  getResponse();
  Serial1.print("AT\r");
  getResponse();
  Serial1.print("AT\r");
  getResponse();
  Serial1.print("AT\r");
  getResponse();
  Serial1.print("AT\r");
  getResponse();

  //========   SIM MODULE SETUP   =======//

  Serial1.print("AT+CGDCONT=1,\"IP\",\"super\"\r");//"super" is the key required to log onto the network using Twilio SuperSIM
  delay(500);
  getResponse();
  Serial1.print("AT+COPS=1,2,\"310410\"\r"); //310410 is AT&T's network code https://www.msisdn.net/mccmnc/310410/
  delay(5000);
  getResponse();
  Serial1.print("AT+SAPBR=3,1,\"APN\",\"super\"\r"); //establish SAPBR profile.  APN = "super"
  delay(3000);
  getResponse();
  Serial1.print("AT+SAPBR=1,1\r");
  delay(2000);
  getResponse();
  Serial1.print("AT+CMGD=0,4\r"); //this line deletes any existing text messages to ensure
  //that the message space is empy and able to accept new messages
  delay(100);
  getResponse();
  Serial1.print("AT+CMGF=1\r");
  delay(100);
  getResponse();
  Serial1.print("AT+CNMI=2,2,0,0,0\r"); //
  delay(100);
  getResponse();
  //the sendSMS function takes four parameters.
  //sendSMS(urlHeaderArray, contactFromArray1, contactToArray1, SetCombody);
  sendSMS(urlHeaderArray, contactFromArray1, contactToArray2, SetCombody);
  //sendSMS(urlHeaderArray, contactFromArray1, contactToArray3, SetCombody);
  delay(2000);
  Serial.println(F("Setup complete. Entering main loop"));
}
