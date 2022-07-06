
const int pushButton = 5;
const int encoder0PinA = 4;
const int encoder0PinB = 3;
static bool userInput = false; // when true display EEPROM faults
                              // when false display current faults

void setup() 
{
  pinMode (encoder0PinA, INPUT);
  pinMode (encoder0PinB, INPUT);
  pinMode (pushButton, INPUT_PULLUP);
  //digitalWrite (pushButton, HIGH);
  Serial.begin (9600);
}

void loop() 
{ 
  LCDDisplayEEPROM();
}

void LCDDisplayEEPROM()
{
  static int LCDscreenPage = 0;
  static int encoder0PinALast = LOW;
  static int n = LOW;
  static unsigned long LCDdebounce{};
  static int debounceDelay{350};
  static bool LCDTimerSwitch = false;
  
  if(digitalRead(pushButton) == LOW && LCDTimerSwitch == false)
  {
    LCDdebounce = millis();
    LCDTimerSwitch = true;
  }
  if (digitalRead(pushButton) == LOW && (millis() - LCDdebounce) > debounceDelay)
  {
    if(userInput)
    {
      Serial.println ("User Input Recieved OFF");
    }
    else Serial.println ("User Input Recieved ON");
    userInput = !userInput;
    LCDTimerSwitch = false;
  }
  if(userInput)
  {
    n = digitalRead(encoder0PinA);
  if ((encoder0PinALast == LOW) && (n == HIGH)) 
  {
    if (digitalRead(encoder0PinB) == LOW) 
    {
      LCDscreenPage--; //LCD.print statments from EEPROM here
    } 
    else 
    {
      LCDscreenPage++; //LCD.print statments from EEPROM here
    }
    Serial.println (LCDscreenPage);
  }
  encoder0PinALast = n;
  }
  
}
