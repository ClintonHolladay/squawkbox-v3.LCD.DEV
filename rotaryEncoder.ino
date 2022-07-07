
const int pushButton {5};
const int encoderPinA {4};
const int encoderPinB {3};
static bool userInput = false; // when true display EEPROM faults
                              // when false display current faults

void setup() 
{
  pinMode (encoderPinA, INPUT);
  pinMode (encoderPinB, INPUT);
  pinMode (pushButton, INPUT_PULLUP);
  Serial.begin (9600);
}

void loop() 
{ 
  LCDDisplayEEPROM();
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
    LCDTimerSwitch = false;
    if(userInput)Serial.println ("User Input Recieved ON");
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
        LCDscreenPage--; //LCD.print statments from EEPROM here
      } 
      else 
      {
        LCDscreenPage++; //LCD.print statments from EEPROM here
      }
      Serial.println (LCDscreenPage);
    }
    encoderPinALast = n;
  } 
}
