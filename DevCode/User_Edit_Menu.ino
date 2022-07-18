#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x3F, 20, 4);

bool userInput{};
bool userInput2{};

//LCD Menu Logic
const int numOfScreens = 8;
int currentScreen = 0;
const char* MainScreen[numOfScreens] = {"Fault History","Contact 1","Contact 2","Contact 3","Contact 4","Contact 5","Contact 6","EXIT"};
int parameters[numOfScreens];

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

void User_Input_Print_Screen(const char* SCREEN[], int NumofLines, int CURSOR) 
{
  if(CURSOR < 4)
  {
    lcd.clear();
    lcd.setCursor(0,CURSOR);
    lcd.print(">");
    lcd.setCursor(2,0);
    for(int i = 0; i < NumofLines; i++)
    {
      lcd.setCursor(2,i);
      lcd.print(SCREEN[i]);
    }
  }
  else
  {
    lcd.clear();
    lcd.setCursor(0,(CURSOR - 4));
    lcd.print(">");
    lcd.setCursor(2,0);
    for(int i = 0; i < 4; i++)
    {
      lcd.setCursor(2,i);
      lcd.print(SCREEN[i + 4]);
    }
  }
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
  if(digitalRead(pushButton) == LOW && LCDTimerSwitch == false && !userInput2 && !userInput)
  {
    LCDdebounce = millis();
    LCDTimerSwitch = true;
  }
  /*  Once userInput has been recieved and the debounce time has passed we ! the userInput bool and turn 
   *  off the LCDTimerSwitch to stop running through the timer code until a new userinput is recieved.*/
  if (LCDTimerSwitch && (millis() - LCDdebounce) > debounceDelay && !userInput2 && !userInput)
  {
    userInput = !userInput;
    LCDTimerSwitch = false;
    if(userInput)
    {
      Serial.println("User Input Recieved ON");
      Cursor = 0;
      User_Input_Print_Screen(MainScreen,4,Cursor);
    }
    else Serial.println ("User Input Recieved OFF");
  }
  /*  The userInput bool is used as a latch so that the LCD screen continues to display what the user wants until the pushButton 
   *  is activated again. When the above timer algorithm has been satisfied the squawk will begin tracking rotational rotary 
   *  encoder input from the user in order to traverse and LCD display the EEPROM fault data.*/
  if(userInput && !userInput2)
  {
    n = digitalRead(encoderPinA);
    if ((encoderPinALast == LOW) && (n == HIGH)) 
    {
      if (digitalRead(encoderPinB) == LOW) 
      {
        //Counter Clockwise turn
        Cursor--;
        if(Cursor < 0)
        {
          Cursor = 7;
        }
        User_Input_Print_Screen(MainScreen,4,Cursor);
      } 
      else 
      {
        //Clockwise turn
        Cursor++;
        if(Cursor > 7)
        {
          Cursor = 0;
        }
        User_Input_Print_Screen(MainScreen,4,Cursor);
      }
    }
    encoderPinALast = n;
  } 
  
 //==============================================================================================//
 //==============================================================================================//
 //==================================SUB MENU FUNCTION===========================================//
 //==============================================================================================//
 //==============================================================================================//
 
  static unsigned long LCDdebounce2{};
  static int debounceDelay2{350};
  static bool LCDTimerSwitch2 {false};
  
  if(digitalRead(pushButton) == LOW && LCDTimerSwitch2 == false && userInput && !userInput2)
  {
    LCDdebounce2 = millis();
    LCDTimerSwitch2 = true;
  }
  /*  Once userInput has been recieved and the debounce time has passed we ! the userInput bool and turn 
   *  off the LCDTimerSwitch to stop running through the timer code until a new userinput is recieved.*/
  if (LCDTimerSwitch2 && (millis() - LCDdebounce2) > debounceDelay2 && userInput && !userInput2)
  {
    userInput2 = !userInput2;
    LCDTimerSwitch2 = false;
    if(userInput2 && Cursor == 0)
    {
      Serial.println("User Input2 ON");
      lcd.clear();
      lcd.setCursor(2,0);
      lcd.print("SUB MENU");// put EEPROM FAULTS here
    }
    else if(userInput2 && Cursor == 7)
    {
      userInput2 = !userInput2;
      userInput = !userInput;
      LCDTimerSwitch2 = false;
    }
  }

 //==============================================================================================//
 //==================================EXIT MENU FUNCTION==========================================//
 //=====================================FOR EEPROM===============================================//
  
  if(userInput && userInput2)
  {
    static unsigned long LCDdebounce3{};
    static int debounceDelay3{350};
    static bool LCDTimerSwitch3 {false};
    
    if(digitalRead(pushButton) == LOW && LCDTimerSwitch3 == false)
    {
      LCDdebounce3 = millis();
      LCDTimerSwitch3 = true;
    }
    /*  Once userInput has been recieved and the debounce time has passed we ! the userInput bool and turn 
     *  off the LCDTimerSwitch to stop running through the timer code until a new userinput is recieved.*/
    if (LCDTimerSwitch3 && (millis() - LCDdebounce3) > debounceDelay3)
    {
      userInput2 = !userInput2;
      LCDTimerSwitch3 = false;
      User_Input_Print_Screen(MainScreen,4,Cursor);
    }
  }

}
