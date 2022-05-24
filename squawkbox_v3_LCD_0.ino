#include <SD.h>
#include <Wire.h> // Library for I2C communication
#include <LiquidCrystal_I2C.h>

//The following const int pins are all pre-run in the PCB:
const int low1 = 5;  //to screw terminal
const int low2 = 6;   //to screw terminal
const int alarmPin = 4;   //to screw terminal
const int hplcIN {14};
const int hplcOUT {15};
const int PLWCOoutletPin {17};
const int SLWCOoutletPin {16};
//const int LCDFlickerDelay {3000};
const int debounceInterval = 3000;  //to prevent false alarms from electrical noise.  
//Setting this debounce too high will prevent the annunciation of instantaneous alarms like a bouncing LWC.

unsigned long currentMillis {};
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

bool PLWCOSent{};
bool SLWCOSent{};
bool HWAlarmSent{};
bool hlpcSent{};
bool alarm{};
bool primaryCutoff{};
bool secondaryCutoff{};
bool hlpcCOMMON{};
bool hlpcNC{};
bool PLWCOoutlet{};
bool SLWCOoutlet{};
bool alarmSwitch = false;
bool alarmSwitch2 = false;
bool alarmSwitch3 = false;
bool alarmSwitch4 = false;
bool ClearScreenSwitch {};

String PrimaryString {"Primary Low Water"};
String SecondaryString {"Secondary Low Water"};
String hlpcString {"High Limit"};
String AlarmString {"FSG Alarm"};

LiquidCrystal_I2C lcd(0x3F, 20, 4);
File myFile;

//=======================================================================

void setup() {

  Serial.begin(9600);
  Serial1.begin(19200);
  Serial.println(F("This is squawkbox V3.LCD.0 sketch."));
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
}

void print_alarms()
{
  if (!PLWCOSent && !SLWCOSent &&!HWAlarmSent &&!hlpcSent)
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
      // send message code
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
      // send message code
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
      // send message code
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
      // send message code
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
