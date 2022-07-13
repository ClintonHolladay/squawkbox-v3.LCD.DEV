#include <SD.h>
#include <Wire.h> // Library for I2C communication
#include <LiquidCrystal_I2C.h> // Library for LCD

LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x3F, 20, 4);


File myFile;
//The following const int pins are all pre-run in the PCB:
const int low1 = 5;  //to screw terminal
const int low2 = 6;   //to screw terminal
const int SIMpin = A3;  // this pin is routed to SIM pin 12 for boot (DF Robot SIM7000A module)
const int debounceInterval = 3000;  //to prevent false alarms from electrical noise.
//Setting this debounce too high will prevent the annunciation of instantaneous alarms like a bouncing LWC.

int primaryCutoff;
int counter1;
int secondaryCutoff;
int counter2;


char SetCombody[] = "Body=Setup%20Complete\"\r";
char LWbody[] = "Body=Low%20Water\"\r";
char LW2body[] = "Body=Low%20Water2\"\r";

String URLheader = "";
String conFrom1 = "";
String conFrom2 = "";
String conTo1 = "";
String conTo2 = "";
String conTo3 = "";

char contactFromArray1[25];
char contactFromArray2[25];
char contactToArray1[25];
char contactToArray2[25];
char contactToArray3[25];
char urlHeaderArray[100];
unsigned char data = 0;

unsigned long currentMillis = 0;
unsigned long difference = 0;
unsigned long difference2 = 0;

unsigned long msgtimer1 = 0;
unsigned long alarmTime = 0;
unsigned long alarmTime2 = 0;

bool alarmSwitch = false;
bool alarmSwitch2 = false;

void setup() {

  Serial.begin(9600);
  Serial1.begin(19200);
  Serial.println(F("This is squawkbox v3.LCD.I2C sketch."));
  lcd.init();
  lcd.backlight();

  // set up the LCD's number of columns and rows:
  lcd.setCursor(2, 0);
  // Print an initial message to the LCD.
  lcd.print("AB3D Squawk Box");
  delay(2000);
  lcd.clear();

  pinMode(low1, INPUT);
  pinMode(low2, INPUT);

  SIMboot();
  // Give time to your GSM shield log on to network
  //delay(15000);

  //loadContacts();
  //  Serial.println(F("Contacts Loaded.  Booting SIM module.  Initiating wakeup sequence..."));
  //  delay(2000);
  //  //PUT SIM MODULE WAKEUP HERE
  //  Serial.println("Hey!  Wake up!");
  //  Serial1.print("AT\r"); //Manufacturer identification
  //  getResponse();
  //  Serial1.print("AT\r"); //Manufacturer identification
  //  getResponse();
  //  Serial1.print("AT\r"); //Manufacturer identification
  //  getResponse();
  //  Serial1.print("AT\r"); //Manufacturer identification
  //  getResponse();
  //  Serial1.print("AT\r"); //Manufacturer identification
  //  getResponse();
  //  //SIM MODULE SETUP---
  //  Serial1.print("AT+CGDCONT=1,\"IP\",\"super\"\r");
  //  delay(500);
  //  getResponse();
  //  Serial1.print("AT+COPS=1,2,\"310410\"\r");
  //  delay(5000);
  //  getResponse();
  //  Serial1.print("AT+SAPBR=3,1,\"APN\",\"super\"\r");
  //  delay(3000);
  //  getResponse();
  //  Serial1.print("AT+SAPBR=1,1\r");
  //  delay(2000);
  //  getResponse();
  //  Serial1.print("AT+CMGD=0,4\r");
  //  delay(100);
  //  getResponse();
  //  Serial1.print("AT+CMGF=1\r");
  //  //PUT TEST MESSAGE HERE
  //  delay(100);
  //  getResponse();
  //  Serial1.print("AT+CNMI=2,2,0,0,0\r");
  //  delay(100);
  //  getResponse();
  //  sendSMS(urlHeaderArray, contactFromArray1, contactToArray1, SetCombody);
  //  sendSMS(urlHeaderArray, contactFromArray1, contactToArray2, SetCombody);
  //  //sendSMS(urlHeaderArray, contactFromArray1, contactToArray3, SetCombody);
  delay(2000);

  Serial.println(F("Setup() Function complete. Entering Main Loop() Function"));

}
void loop()
{
  primaryCutoff = digitalRead(low1);
  secondaryCutoff = digitalRead(low2);
  currentMillis = millis();
  print_alarms();
  primary_LW();
  secondary_LW();

}

void primary_LW()
{
//  if ((primaryCutoff == HIGH) && (counter1 == 1)) {
//    lcd.clear();
//    lcd.print("LOW WATER");
//    delay(20);
//  }
  if ((primaryCutoff == HIGH) && (counter1 == 0))
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
      //sendSMS(urlHeaderArray, contactToArray2, contactFromArray1, LWbody);
      //sendSMS(urlHeaderArray, contactToArray3, contactFromArray1, LWbody);

      Serial.println(F("message sent or simulated"));
      delay(10);
      counter1 = 1;
      difference = 0;
      alarmSwitch = false;
      alarmTime = 0;
    }
    if (difference < debounceInterval)
    {
      Serial.println(difference);
      return;
    }
  }
  else
  {

    if (primaryCutoff == LOW)
    {
      alarmSwitch = false;
      difference = 0;
      alarmTime = 0;
      counter1 = 0;
      return;
    }
  }
}

void secondary_LW()
{
  if ((secondaryCutoff == HIGH) && (counter2 == 0))
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
      //sendSMS(urlHeaderArray, contactToArray2, contactFromArray1, LW2body);
      //sendSMS(urlHeaderArray, contactToArray3, contactFromArray1, LW2body);
      Serial.println(F("message sent or simulated"));
      delay(10);
      counter2 = 1;
      difference2 = 0;
      alarmSwitch2 = false;
      alarmTime2 = 0;
    }
    if (difference2 < debounceInterval)
    {
      Serial.println(difference2);
      return;
    }
  }
  else
  {

    if (secondaryCutoff == LOW)
    {
      alarmSwitch2 = false;
      difference2 = 0;
      alarmTime2 = 0;
      counter2 = 0;
      return;
    }
  }
}

void print_alarms()
{
    if (counter1 == 0 && counter2 == 0)
  {
    lcd.clear();
    lcd.print("No Alarm");
    delay(30);
  }
  else if (counter1 == 1 && counter2 == 1)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Both LWC Alarms");
    lcd.setCursor(0, 1);
    lcd.print("Are Alarming");
    lcd.setCursor(0, 2);
    lcd.print("Your boiler");
    lcd.setCursor(0, 3);
    lcd.print("is toast");
    delay(120);
  }
  else if (counter1 == 1)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("primary low water");
    delay(30);
  }
  else if (counter2 == 1)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("secondary low water");
    delay(30);
  }
}
void sendSMS(char pt1[], char pt2[], char pt3[], char pt4[])
{

  char finalURL[250] = "";

  strcpy(finalURL, pt1);
  strcat(finalURL, pt2);
  strcat(finalURL, pt3);
  strcat(finalURL, pt4);
  delay(500);
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

  getResponse();
}
void getResponse()
{
  //comment here
  if (Serial1.available())
  {
    while (Serial1.available())
    {
      data = Serial1.read();
      Serial.write(data);
    }
    data = 0;
  }
  delay(500);
}

void loadContacts()
{

  if (!SD.begin(10)) {
    Serial.println(F("initialization failed!"));
    while (1);
  }
  Serial.println(F("initialization done."));

  //---------------------------------------------//
  //------------------load "from" number.  This is the number alert messages will apear to be from-------------//

  myFile = SD.open("from1.txt");
  if (myFile) {
    Serial.println("phone number command 1");
    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      char c = myFile.read();  //gets one byte from serial buffer
      conFrom1 += c;
    }
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening from1.txt");
  }
  //convert the String into a character array
  conFrom1.toCharArray(contactFromArray1, 25);
  Serial.print(F("The first phone number FROM String is "));
  Serial.println(conFrom1);
  Serial.print(F("The first phone number FROM char array is "));
  Serial.println(contactFromArray1);

  //---------------------------------------------//
  //------------------load "from2" number.  This is the number alert messages will apear to be from-------------//

  myFile = SD.open("from2.txt");
  if (myFile) {
    Serial.println("loading second from number");
    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      char c = myFile.read();  //gets one byte from serial buffer
      conFrom2 += c;
    }
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening from2.txt");
  }
  //convert the String into a character array
  conFrom2.toCharArray(contactFromArray2, 25);
  Serial.print(F("The first phone number FROM String is "));
  Serial.println(conFrom2);
  Serial.print(F("The second phone number FROM char array is "));
  Serial.println(contactFromArray2);

  //---------------------------------------------//
  //------------------load first contact number-------------//

  myFile = SD.open("to1.txt");
  if (myFile) {
    Serial.println("phone number 1 command");
    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      char c = myFile.read();  //gets one byte from serial buffer
      conTo1 += c;
    }
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening toContact1.txt");
  }

  conTo1.toCharArray(contactToArray1, 25);
  Serial.print(F("The first phone number TO String is "));
  Serial.println(conTo1);
  Serial.print(F("The first phone number TO char array is "));
  Serial.println(contactToArray1);

  //---------------------------------------------//
  //------------------load second contact number-------------//

  myFile = SD.open("to2.txt");
  if (myFile) {
    Serial.println(F("phone number 2 command"));
    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      char c = myFile.read();  //gets one byte from serial buffer
      conTo2 += c;
    }
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println(F("error opening to2.txt"));
  }

  conTo1.toCharArray(contactToArray2, 25);
  Serial.print(F("The second phone number TO String is "));
  Serial.println(conTo2);
  Serial.print(F("The second phone number TO char array is "));
  Serial.println(contactToArray2);

  //---------------------------------------------//
  //------------------load third contact number-------------//

  myFile = SD.open("to3.txt");
  if (myFile) {
    Serial.println(F("loading third contact number"));
    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      char c = myFile.read();  //gets one byte from serial buffer
      conTo3 += c;
    }
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println(F("error opening to3.txt"));
  }

  conTo3.toCharArray(contactToArray3, 25);
  Serial.print(F("The third phone number TO String is "));
  Serial.println(conTo3);
  Serial.print(F("The third phone number TO char array is "));
  Serial.println(contactToArray3);

  //---------------------------------------------//
  //------------------load URL header-------------//

  myFile = SD.open("URL.txt");
  if (myFile) {
    Serial.println(F("loading URL header"));
    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      char c = myFile.read();  //gets one byte from serial buffer
      URLheader += c;
    }
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println(F("error opening URL.txt"));
  }

  URLheader.toCharArray(urlHeaderArray, 100);
  Serial.print(F("The URL header is "));
  Serial.println(URLheader);
  Serial.print(F("The URL header array is  "));
  Serial.println(urlHeaderArray);
}

void SIMboot()
{
  digitalWrite(SIMpin, HIGH);
  delay(3000);
  digitalWrite(SIMpin, LOW);
}
