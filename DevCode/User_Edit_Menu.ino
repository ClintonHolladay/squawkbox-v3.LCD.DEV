#include <SD.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <RTClib.h>
#include <LibPrintf.h>

LiquidCrystal_I2C lcd(0x3F, 20, 4);

#define ACTIVE 1
#define INACTIVE 0
//Rotary encoder definitons
#define CW 1
#define CCW -1
#define PUSH_BUTTON 0

static bool userInput{};
static bool userInput2{};
static bool userInput3{};
static bool userInput4{};
static bool faultHistory{};

static char contact1[11] {"1111111111"};
static char contact2[11] {"2222222222"};
static char contact3[11] {"3333333333"};
static char contact4[11] {"4444444444"};
static char contact5[11] {"5555555555"};
static char contact6[11] {"6666666666"};

static bool contact1Status {ACTIVE}; // will need to be stored in the EEPROM for when a loss of power happens 
static bool contact2Status {ACTIVE};
static bool contact3Status {ACTIVE};
static bool contact4Status {ACTIVE};
static bool contact5Status {ACTIVE};
static bool contact6Status {ACTIVE};

//LCD Menu Logic
const char* MainScreen[8] = {"Fault History","Contact 1","Contact 2","Contact 3","Contact 4","Contact 5","Contact 6","EXIT"};
const char* contactScreen[6] = {"CURRENT#","STATUS: ","EDIT#","EXIT","SAVE#","REDO#"};

const int resetPin {48};

const int pushButton {42};   //=====================//
const int encoderPinA {44};  // Rotary encoder pins //
const int encoderPinB {46};  //=====================//

const char PrimaryString[] {"Primary Low Water"};     // ====================================//
const char SecondaryString[] {"Secondary Low Water"}; // alarm text printed to the LCD screen//
const char hlpcString[] {"High Limit"};               // alarm text printed to the LCD screen//
const char AlarmString[] {"FSG Alarm"};               // ====================================//
const char DefaultString[] {"Open Fault Memory"};     // ====================================//

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
enum EEPROMAlarmCode 
{
  PLWCO = 1, 
  SLWCO, 
  FSGalarm, 
  HighLimit
};

RTC_PCF8523 rtc;
File myFile;

void setup() 
{
  //digitalWrite(resetPin,HIGH);
  pinMode(resetPin, INPUT_PULLUP);
  Serial.begin(9600);

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

  boot_SD();
  pinMode (encoderPinA, INPUT);
  pinMode (encoderPinB, INPUT);
  pinMode (pushButton, INPUT_PULLUP);
  EEPROM_Prefill();
  lcd.init(); // LCD initialization
  lcd.backlight();
  lcd.begin(20, 4); // initialize LCD screen (columns, rows)
  lcd.setCursor(2, 1);
  lcd.print("AB3D Squawk Box");
  delay(2000);
  //reboot_From_Contact_Change("to2.txt", contact1);
}


void loop() 
{
  static unsigned long debounce{};
  static int debounceLoop{2000};
  static bool TimerSwitch {false};
    if(!userInput && !userInput2 && TimerSwitch == false)
    {
      debounce = millis();
      TimerSwitch = true;
    }
    if (!userInput && !userInput2 && TimerSwitch && (millis() - debounce) > debounceLoop)
    {
      TimerSwitch = false;
      lcd.clear();
      lcd.setCursor(2, 1);
      lcd.print("AB3D SQUAWK BOX");
    }
    
    User_Input_Access_Menu();
}

void User_Input_Main_Screen(int CURSOR) 
{
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

void User_Input_Contact_Screen(const char* SCREEN[], int CURSOR, char CONTACT[], bool STATUS) 
{
    lcd.clear();
    lcd.setCursor(0,CURSOR);
    lcd.print("-");
    lcd.setCursor(1,CURSOR);
    lcd.print(">");
    lcd.setCursor(2,0);
    lcd.print(SCREEN[0]);
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

void Contact_Edit_Screen(const char* SCREEN[], int CURSOR, char CONTACT[], bool STATUS) 
{
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
    lcd.setCursor(10,1);
    lcd.blink();
}

void User_Input_Access_Menu()
{
 //==============================================================================================//
 //==============================================================================================//
 //==================================MAIN MENU FUNCTION==========================================//
 //==============================================================================================//
 //==============================================================================================//
 
  static int Cursor{};
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
    userInput = true;
    LCDTimerSwitch = false;
    if(userInput)//this line not needed?
    {
      Serial.println("User Input Recieved ON");
      Cursor = 0;//this line not needed?
      User_Input_Main_Screen(Cursor);//change to take no arguments and default cursor to 0 in the body of the function?
    }
    else Serial.println ("User Input Recieved OFF");//this line not needed?
  }
  /*  The userInput bool is used as a latch so that the LCD screen continues to display what the user wants until the pushButton 
   *  is activated again. When the above timer algorithm has been satisfied the squawk will begin tracking rotational rotary 
   *  encoder input from the user in order to traverse and LCD display the EEPROM fault data.*/
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
          //return;
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
          }
          lcd.setCursor(0,Cursor + 1);
          lcd.print(" ");
          lcd.setCursor(1,Cursor + 1);
          lcd.print(" ");
          lcd.setCursor(0,Cursor);
          lcd.print("-");
          lcd.setCursor(1,Cursor);
          lcd.print(">");
        }
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
          //return;
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
  /*  Once userInput has been recieved and the debounce time has passed we ! the userInput bool and turn 
   *  off the LCDTimerSwitch to stop running through the timer code until a new userinput is recieved.*/
  if (LCDTimerSwitch && (millis() - LCDdebounce) > debounceDelay && userInput && !userInput2)
  { 
    userInput2 = true;
    LCDTimerSwitch = false;
    if(userInput2 && Cursor == 0)
    {
      faultHistory = true;
      EEPROMalarmPrint(Cursor, 0);
    }
    else if(userInput2 && Cursor == 1)
    {
      User_Input_Contact_Screen(contactScreen, 1, contact1, contact1Status);
      whichContactToEdit = 1;
      Cursor = 0;
    }
    else if(userInput2 && Cursor == 2)
    {
      User_Input_Contact_Screen(contactScreen, 1, contact2, contact2Status);
      whichContactToEdit = 2;
      Cursor = 0;
    }
    else if(userInput2 && Cursor == 3)
    {
      User_Input_Contact_Screen(contactScreen, 1, contact3, contact3Status);
      whichContactToEdit = 3;
      Cursor = 0;
    }
    else if(userInput2 && Cursor == 4)
    {
      User_Input_Contact_Screen(contactScreen, 1, contact4, contact4Status);
      whichContactToEdit = 4;
      Cursor = 0;
    }
    else if(userInput2 && Cursor == 5)
    {
      User_Input_Contact_Screen(contactScreen, 1, contact5, contact5Status);
      whichContactToEdit = 5;
      Cursor = 0;
    }
    else if(userInput2 && Cursor == 6)
    {
      User_Input_Contact_Screen(contactScreen, 1, contact6, contact6Status);
      whichContactToEdit = 6;
      Cursor = 0;
    }
    else if(userInput2 && Cursor == 7)
    {
      userInput2 = false;
      userInput = false;
    }
  }

 //==============================================================================================//
 //=================================EEPROM MENU FUNCTION=========================================//
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

  if(digitalRead(pushButton) == LOW) //(&& LCDTimerSwitch == false) took this out to speed up the pushbutton recognition
  {
    LCDdebounce = millis();
    LCDTimerSwitch = true;
  }
  /*  Once userInput has been recieved and the debounce time has passed we ! the userInput bool and turn 
   *  off the LCDTimerSwitch to stop running through the timer code until a new userinput is recieved.*/
  if (LCDTimerSwitch && (millis() - LCDdebounce) > debounceDelay)
  { 
    userInput3 = true;
    LCDTimerSwitch = false;
    if(userInput3)
    {
      userInput3 = false;
      userInput2 = false;
      faultHistory = false;
      Cursor = 0;
      User_Input_Main_Screen(Cursor);
    }
  }
 }

 //==============================================================================================//
 //=====================================SUB MENU 2 CODE==========================================//
 //==============================================================================================//

  switch(whichContactToEdit)
  {
    case 0:break;
    case 1: Contact_Edit_Menu(contact1Status, contact1);
    break;
    case 2: Contact_Edit_Menu(contact2Status, contact2);
    break;
    case 3: Contact_Edit_Menu(contact3Status, contact3);
    break;
    case 4: Contact_Edit_Menu(contact4Status, contact4);
    break;
    case 5: Contact_Edit_Menu(contact5Status, contact5);
    break;
    case 6: Contact_Edit_Menu(contact6Status, contact6);
    break;
    default: break;
  } 
}

void Contact_Edit_Menu(bool& CONTACTSTATUS, char CONTACT[])
{
  static unsigned long LCDdebounce2{};
  static bool LCDTimerSwitch2 {false};
  static int debounceDelay2{350};
  static int Cursor2{1};
  static int encoderPinALast2 {LOW}; 
  static int n2 {LOW};
  
  if(userInput && userInput2 && !faultHistory)
  { 
    n2 = digitalRead(encoderPinA);
    if ((encoderPinALast2 == LOW) && (n2 == HIGH)) 
    {
      if (digitalRead(encoderPinB) == LOW) 
      {
        //Counter Clockwise turn
        Cursor2--;
        delay(20);//UI tuning. Makes the display user's interaction lees choppy.
        if(Cursor2 < 1)
        {
          Cursor2 = 1;
          delay(10);//UI tuning. Makes the display user's interaction lees choppy.
         // return;//might prevent LCD flicker  
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
        delay(20);//UI tuning. Makes the display user's interaction lees choppy.
        if(Cursor2 > 3)
        {
          Cursor2 = 3;
          delay(10);//UI tuning. Makes the display user's interaction lees choppy.
          //return;//might prevent LCD flicker
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
    /*  Once userInput has been recieved and the debounce time has passed we ! the userInput bool and turn 
     *  off the LCDTimerSwitch to stop running through the timer code until a new userinput is recieved.*/
    if (LCDTimerSwitch2 && (millis() - LCDdebounce2) > debounceDelay2)
    { 
      userInput3 = true;
      LCDTimerSwitch2 = false;
      if(userInput3 && Cursor2 == 1)// Turn CONTACT ON/OFF
      {
        //code for turning on/off this contact and saving it to the EEPROM
        CONTACTSTATUS = !CONTACTSTATUS;
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
          userInput3 = false;
        }
      else if(userInput3 && Cursor2 == 2)//EDIT CONTACT
      {
        lcd.setCursor(0,2);
        lcd.print("ENTER NEW#");
        lcd.setCursor(10,2);
        lcd.blink();
        Cursor2 = 10;
        Contact_Edit_Screen(contactScreen, 1, CONTACT, CONTACTSTATUS);
        userInput3 = true;
      }
      else if(userInput3 && Cursor2 == 3)//EXIT CONTACT
      {
        userInput3 = false;
        userInput2 = false;
        Cursor2 = 1;
        User_Input_Main_Screen(0);
      }
    }
  }
  
  //==============================================================================================//
  //==============================CONTACT NUMBER EDITING CODE=====================================//
  //==============================================================================================//
 
  while(userInput3)
  {
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
        if(newDigit < 48)
        {
          newDigit =57;
          return;//prevents LCD flicker
        }
        lcd.setCursor(Cursor2,1);
        lcd.print(newDigit);
        lcd.setCursor(Cursor2,1);
      } 
      else //Clockwise turn
      {
        newDigit++;
        if(newDigit == 58)
        {
          newDigit = 48;
          return;//prevents LCD flicker
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
    /*  Once userInput has been recieved and the debounce time has passed we ! the userInput bool and turn 
     *  off the LCDTimerSwitch to stop running through the timer code until a new userinput is recieved.*/
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
        static int i {};
        newContact[i] = newDigit;
        i++;
        printf("i = %i\n",i);
        Serial.print("newContact# in now: ");
        Serial.println(newContact);
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
          while(userInput4)
          {
            static int selector{};
            n2 = digitalRead(encoderPinA);
            if ((encoderPinALast2 == LOW) && (n2 == HIGH)) 
            {
              if (digitalRead(encoderPinB) == LOW) 
              {
                //Counter Clockwise turn
                selector--;
                if(selector < 0)
                {
                  selector = 2;
                }
              } 
              else //Clockwise turn
              {
                selector++;
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
            
            if(digitalRead(pushButton) == LOW && LCDTimerSwitch2 == false)
            {
              LCDdebounce2 = millis();
              LCDTimerSwitch2 = true;
            }
            /*  Once userInput has been recieved and the debounce time has passed we ! the userInput bool and turn 
             *  off the LCDTimerSwitch to stop running through the timer code until a new userinput is recieved.*/
            if (LCDTimerSwitch2 && (millis() - LCDdebounce2) > debounceDelay2)
            { 
              LCDTimerSwitch2 = false;
                if(selector == 0)// SAVE NEW CONTACT 
                {
                  strcpy(CONTACT,newContact);
                  Save_New_Contact(CONTACT);
                  userInput4 = false;
                  userInput3 = false;
                  User_Input_Contact_Screen(contactScreen, 1, CONTACT, CONTACTSTATUS);
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
                }
                else if(selector == 2)//EXIT
                {
                  userInput4 = false;
                  userInput3 = false;
                  Cursor2 = 1;
                  User_Input_Contact_Screen(contactScreen, 1, CONTACT, CONTACTSTATUS);
                }
             }
          }
        }
     }
  }
}

void Save_New_Contact(char NEWCONTACT[])
{
  printf("Save_New_Contact().\n");
  myFile = SD.open("to2.txt", O_WRITE);
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

void reboot_From_Contact_Change(char file_name[], char CONTACT[])
{
  myFile = SD.open(file_name);
  if (myFile) 
  {
    // read from the file until there's nothing else in it:
    int i{};
    //char Temp[11];
    while (myFile.available())
    {
      char c = myFile.read();  //gets one byte from serial buffer
      //Temp[i] = c;
      CONTACT[i] = c;
      i++;
    }
    myFile.close();
    //const char* holder = Temp;
    //return Temp;
  }
  else
  {
    // if the file didn't open, print an error:
    Serial.print("***ERROR opening reboot_From_Contact_Change file ");
    Serial.println(file_name);
  }
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
