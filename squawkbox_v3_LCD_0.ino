/*LiquidCrystal
 LCD RS pin to digital pin 8
 LCD Enable pin to digital pin 7
 LCD D4 pin to digital pin 42
 LCD D5 pin to digital pin 44
 LCD D6 pin to digital pin 46
 LCD D7 pin to digital pin 48
 LCD R/W pin to ground
 LCD VSS pin to ground
 LCD VCC pin to 5V
 10K resistor:pot
 ends to +5V and ground
 wiper to LCD VO pin (pin 3)
*/
#include <SD.h>
#include <LiquidCrystal.h>
//The following const int pins are all pre-run in the PCB:
const int low1 = 5;  //to screw terminal
const int low2 = 6;   //to screw terminal
const int alarmPin = 4;   //to screw terminal
const int hplcIN {14};
const int hplcOUT {15};
const int PLWCOoutletPin {17};
const int SLWCOoutletPin {16};

// LCD Mega pin layout for Squawk BreadBoard
const int rs = 8, en = 7, d4 = 42, d5 = 44, d6 = 46, d7 = 48;

const int debounceInterval = 3000;  //to prevent false alarms from electrical noise.  
//Setting this debounce too high will prevent the annunciation of instantaneous alarms like a bouncing LWC.

unsigned long currentMillis {};
unsigned long difference {};
unsigned long difference2 {};
unsigned long difference3 {};
unsigned long difference4 {};
unsigned long difference5 {};
unsigned long alarmTime {};
unsigned long alarmTime2 {};
unsigned long alarmTime3 {};
unsigned long alarmTime4 {};

bool PLWCOSent{};
bool SLWCOSent{};
bool HWAlarmSent{0};
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
bool hlpcDisplay{};

String PrimaryString {"Primary LowWater"};
String SecondaryString {"2nd Low Water"};
String hlpcString {"High Limit"};
String AlarmString {"FSG Alarm"};

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
File myFile;

//=======================================================================

void setup() {

  Serial.begin(9600);
  Serial1.begin(19200);
  Serial.println(F("This is squawkbox v3.LCD.0 sketch."));

  // set up the LCD's number of columns and rows:
  lcd.begin(20, 5);
  lcd.print("AB3D Squawk Box");
  delay(2000);
  pinMode(low1, INPUT);
  pinMode(low2, INPUT);
  pinMode(alarmPin, INPUT);
  pinMode(hplcIN, INPUT);
  pinMode(hplcOUT, INPUT);
  pinMode(PLWCOoutletPin, INPUT);
  delay(10);
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
  currentMillis = millis();

  print_alarms();
  primary_LW();
  secondary_LW();
  Honeywell_alarm();
  HPLC();
}

void print_alarms()
{
  if (!PLWCOSent && !SLWCOSent &&!HWAlarmSent &&!hlpcSent)
  {
    lcd.clear();
    lcd.print("No Alarm");
    delay(30);
  }
  else if (PLWCOSent && SLWCOSent && hlpcSent &&HWAlarmSent )
  {
     lcd.clear();
     lcd.print(PrimaryString);
     lcd.setCursor(0, 1);
     lcd.print(SecondaryString);
     delay(2000);
     lcd.clear();
     lcd.print(SecondaryString);
     lcd.setCursor(0, 1);
     lcd.print(hlpcString);
     delay(2000);
     lcd.clear();
     lcd.print(hlpcString);
     lcd.setCursor(0, 1);
     lcd.print(AlarmString);
     delay(2000); 
     lcd.clear();
     lcd.print(AlarmString);
     lcd.setCursor(0, 1);
     lcd.print(PrimaryString);
     delay(2000); 
  }
  else if (SLWCOSent && hlpcSent && HWAlarmSent)
  {
     lcd.clear();
     lcd.print(SecondaryString);
     lcd.setCursor(0, 1);
     lcd.print(hlpcString);
     delay(2000);
     lcd.clear();
     lcd.print(hlpcString);
     lcd.setCursor(0, 1);
     lcd.print(AlarmString);
     delay(2000);
     lcd.clear();
     lcd.print(AlarmString);
     lcd.setCursor(0, 1);
     lcd.print(SecondaryString);
     delay(2000);  
  }
  else if (PLWCOSent && SLWCOSent && hlpcSent)
  {
     lcd.clear();
     lcd.print(PrimaryString);
     lcd.setCursor(0, 1);
     lcd.print(SecondaryString);
     delay(2000);
     lcd.clear();
     lcd.print(SecondaryString);
     lcd.setCursor(0, 1);
     lcd.print(hlpcString);
     delay(2000);
     lcd.clear();
     lcd.print(hlpcString);
     lcd.setCursor(0, 1);
     lcd.print(PrimaryString);
     delay(2000);  
  }
  else if (PLWCOSent && SLWCOSent && HWAlarmSent)
  {
     lcd.clear();
     lcd.print(PrimaryString);
     lcd.setCursor(0, 1);
     lcd.print(SecondaryString);
     delay(2000);
     lcd.clear();
     lcd.print(SecondaryString);
     lcd.setCursor(0, 1);
     lcd.print(AlarmString);
     delay(2000);
     lcd.clear();
     lcd.print(AlarmString);
     lcd.setCursor(0, 1);
     lcd.print(PrimaryString);
     delay(2000);  
  }
  else if (PLWCOSent && hlpcSent && HWAlarmSent)
  {
     lcd.clear();
     lcd.print(PrimaryString);
     lcd.setCursor(0, 1);
     lcd.print(hlpcString);
     delay(2000);
     lcd.clear();
     lcd.print(hlpcString);
     lcd.setCursor(0, 1);
     lcd.print(AlarmString);
     delay(2000);
     lcd.clear();
     lcd.print(AlarmString);
     lcd.setCursor(0, 1);
     lcd.print(PrimaryString);
     delay(2000);  
  }
  else if (PLWCOSent && SLWCOSent)
  {
     lcd.clear();
     lcd.print(PrimaryString);
     lcd.setCursor(0, 1);
     lcd.print(SecondaryString);
     delay(30); 
  }
  else if (PLWCOSent && hlpcSent)
  {
     lcd.clear();
     lcd.print(PrimaryString);
     lcd.setCursor(0, 1);
     lcd.print(hlpcString);
     delay(30); 
  }
  else if (PLWCOSent && HWAlarmSent)
  {
     lcd.clear();
     lcd.print(PrimaryString);
     lcd.setCursor(0, 1);
     lcd.print(AlarmString);
     delay(30); 
  }
  else if (SLWCOSent && hlpcSent)
  {
     lcd.clear();
     lcd.print(SecondaryString);
     lcd.setCursor(0, 1);
     lcd.print(hlpcString);
     delay(30); 
  }
  else if (SLWCOSent && HWAlarmSent)
  {
     lcd.clear();
     lcd.print(SecondaryString);
     lcd.setCursor(0, 1);
     lcd.print(AlarmString);
     delay(30); 
  }
  else if (hlpcSent && HWAlarmSent)
  {
     lcd.clear();
     lcd.print(hlpcString);
     lcd.setCursor(0, 1);
     lcd.print(AlarmString);
     delay(30); 
  }
  else if (PLWCOSent)
  {
    lcd.clear();
    lcd.print(PrimaryString);
    delay(30);
  }
  else if (SLWCOSent)
  {
    lcd.clear();
    lcd.print(SecondaryString);
    delay(30);
  }
  else if (HWAlarmSent)
  {
    lcd.clear();
    lcd.print(AlarmString);
    delay(30);
  }
  else if (hlpcSent)
  {
    lcd.clear();
    lcd.print(hlpcString);
    delay(30);
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
      delay(10);
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
