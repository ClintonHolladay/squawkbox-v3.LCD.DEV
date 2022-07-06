int val;
int encoder0PinA = 4;
int encoder0PinB = 3;
int encoder0Pos = 0;
int encoder0PinALast = LOW;
int n = LOW;
int pushButton = 5;
unsigned long LCDdebounce{};
int debounceDelay{250};
bool LCDswitch = false;


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
  static bool userInput = false;
  
  if(digitalRead(pushButton) == LOW && LCDswitch == false)
  {
    LCDdebounce = millis();
    LCDswitch = true;
    Serial.println(millis());
  }
  if (digitalRead(pushButton) == LOW && (millis() - LCDdebounce) > debounceDelay)
  {
    Serial.println ("User Input Recieved");
    userInput = !userInput;
    LCDswitch = false;
  }
  if(userInput)
  {
    n = digitalRead(encoder0PinA);
  if ((encoder0PinALast == LOW) && (n == HIGH)) 
  {
    if (digitalRead(encoder0PinB) == LOW) 
    {
      encoder0Pos--; //LCD.print statments from EEPROM here
    } 
    else 
    {
      encoder0Pos++; //LCD.print statments from EEPROM here
    }
    Serial.println (encoder0Pos);
  }
  encoder0PinALast = n;
  }
}
