#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x3F, 20, 4);

#define ACTIVE 1
#define INACTIVE 0

static bool userInput{};
static bool userInput2{};
static bool userInput3{};
static bool faultHistory{};

const char* contact1 {"6158122833"};
static bool contact1Status {ACTIVE}; // will need to be stored in the EEPROM for when a loss of power happens 

//LCD Menu Logic
const char* MainScreen[8] = {"Fault History","Contact 1","Contact 2","Contact 3","Contact 4","Contact 5","Contact 6","EXIT"};
const char* contactScreen[4] = {"CURRENT#","STATUS: ","EDIT","EXIT"};

const int pushButton {42};   //=====================//
const int encoderPinA {44};  // Rotary encoder pins //
const int encoderPinB {46};  //=====================//

void setup() 
{
  Serial.begin(9600);
  pinMode (encoderPinA, INPUT);
  pinMode (encoderPinB, INPUT);
  pinMode (pushButton, INPUT_PULLUP);

  lcd.init(); // LCD initialization
  lcd.backlight();
  lcd.begin(20, 4); // initialize LCD screen (columns, rows)
  lcd.setCursor(2, 1);
  lcd.print("AB3D Squawk Box");
  delay(2000);
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
      lcd.print("AB3D Squawk Box");
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

void User_Input_Contact_Screen(const char* SCREEN[], int CURSOR, const char* CONTACT, bool STATUS) 
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
 
  static unsigned long LCDdebounce2{};//this line not needed? use LCDdebounce instead
  static bool LCDTimerSwitch2 {false};//this line not needed?
  
  if(digitalRead(pushButton) == LOW && LCDTimerSwitch2 == false && userInput && !userInput2)
  {
    LCDdebounce2 = millis();
    LCDTimerSwitch2 = true;
  }
  /*  Once userInput has been recieved and the debounce time has passed we ! the userInput bool and turn 
   *  off the LCDTimerSwitch to stop running through the timer code until a new userinput is recieved.*/
  if (LCDTimerSwitch2 && (millis() - LCDdebounce2) > debounceDelay && userInput && !userInput2)
  { 
    userInput2 = true;
    LCDTimerSwitch2 = false;
    if(userInput2 && Cursor == 0)
    {
      lcd.clear();
      lcd.setCursor(2,0);
      faultHistory = true;
      lcd.print("SUB MENU");// put EEPROM FAULTS here
    }
    else if(userInput2 && Cursor == 1)
    {
      User_Input_Contact_Screen(contactScreen, 1, contact1, contact1Status);
    }
    else if(userInput2 && Cursor == 2)
    {
      lcd.clear();
      lcd.setCursor(2,0);
      lcd.print("SUB MENU 2");
      //User_Input_Contact_Screen(const char* SCREEN[], int CURSOR, char CONTACT[10], bool STATUS);
    }
    else if(userInput2 && Cursor == 3)
    {
      lcd.clear();
      lcd.setCursor(2,0);
      lcd.print("SUB MENU 3");
      //User_Input_Contact_Screen(const char* SCREEN[], int CURSOR, char CONTACT[10], bool STATUS);
    }
    else if(userInput2 && Cursor == 4)
    {
      lcd.clear();
      lcd.setCursor(2,0);
      lcd.print("SUB MENU 4");
      //User_Input_Contact_Screen(const char* SCREEN[], int CURSOR, char CONTACT[10], bool STATUS);
    }
    else if(userInput2 && Cursor == 5)
    {
      lcd.clear();
      lcd.setCursor(2,0);
      lcd.print("SUB MENU 5");
      //User_Input_Contact_Screen(const char* SCREEN[], int CURSOR, char CONTACT[10], bool STATUS);
    }
    else if(userInput2 && Cursor == 6)
    {
      lcd.clear();
      lcd.setCursor(2,0);
      lcd.print("SUB MENU 6");
      //User_Input_Contact_Screen(const char* SCREEN[], int CURSOR, char CONTACT[10], bool STATUS);
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
        Cursor--;
        if(Cursor < 1)
        {
          Cursor = 1;
          return;//prevents LCD flicker
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
        //Clockwise turn
        Cursor++;
        if(Cursor > 3)
        {
          Cursor = 3;
          return;//prevents LCD flicker
        }
        lcd.setCursor(0,Cursor - 1);
        lcd.print(" ");
        lcd.setCursor(1,Cursor - 1);
        lcd.print(" ");
        lcd.setCursor(0,Cursor);
        lcd.print("-");
        lcd.setCursor(1,Cursor);
        lcd.print(">");
      }
    }
    encoderPinALast = n;

  if(digitalRead(pushButton) == LOW && LCDTimerSwitch2 == false && userInput && userInput2 && !userInput3 && faultHistory)
  {
    LCDdebounce2 = millis();
    LCDTimerSwitch2 = true;
  }
  /*  Once userInput has been recieved and the debounce time has passed we ! the userInput bool and turn 
   *  off the LCDTimerSwitch to stop running through the timer code until a new userinput is recieved.*/
  if (LCDTimerSwitch2 && (millis() - LCDdebounce2) > debounceDelay && userInput && userInput2 && !userInput3)
  { 
    userInput3 = true;
    LCDTimerSwitch2 = false;
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
 
  if(userInput && userInput2 && !faultHistory)
  { 
    n = digitalRead(encoderPinA);
    if ((encoderPinALast == LOW) && (n == HIGH)) 
    {
      if (digitalRead(encoderPinB) == LOW) 
      {
        //Counter Clockwise turn
        Cursor--;
        if(Cursor < 1)
        {
          Cursor = 1;
          return;//prevents LCD flicker
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
        //Clockwise turn
        Cursor++;
        if(Cursor > 3)
        {
          Cursor = 3;
          return;//prevents LCD flicker
        }
        lcd.setCursor(0,Cursor - 1);
        lcd.print(" ");
        lcd.setCursor(1,Cursor - 1);
        lcd.print(" ");
        lcd.setCursor(0,Cursor);
        lcd.print("-");
        lcd.setCursor(1,Cursor);
        lcd.print(">");
      }
    }
    encoderPinALast = n;
  }

  if(digitalRead(pushButton) == LOW && LCDTimerSwitch2 == false && userInput && userInput2 && !userInput3)
  {
    LCDdebounce2 = millis();
    LCDTimerSwitch2 = true;
  }
  /*  Once userInput has been recieved and the debounce time has passed we ! the userInput bool and turn 
   *  off the LCDTimerSwitch to stop running through the timer code until a new userinput is recieved.*/
  if (LCDTimerSwitch2 && (millis() - LCDdebounce2) > debounceDelay && userInput && userInput2 && !userInput3)
  { 
    userInput3 = true;
    LCDTimerSwitch2 = false;
    if(userInput3 && Cursor == 1)
    {
      contact1Status = !contact1Status;
      lcd.setCursor(10,1);
      lcd.print("        ");
      lcd.setCursor(10,1);
      if(contact1Status)
      {
        lcd.print("ACTIVE");
      }
      else
      {
        lcd.print("INACTIVE");
      }
        userInput3 = false;
      }
    else if(userInput3 && Cursor == 2)
    {
      lcd.clear();
      lcd.setCursor(2,0);
      lcd.print("EDITING");
    }
    else if(userInput3 && Cursor == 3)
    {
      userInput3 = false;
      userInput2 = false;
      Cursor = 0;
      User_Input_Main_Screen(Cursor);
    }
  }

}
