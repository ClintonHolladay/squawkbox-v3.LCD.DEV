/*This code will be for A1 cleaners.  It will incorporate
  the use of the SS library, AltSoftSerial.  Doing so means
  pins 8 and 9 must be used for software serial.  */

#include <AltSoftSerial.h>
#include <SPI.h>
#include <ModbusMaster.h>

String pnumber1;
String pnumber2;
const int low1 = 4;
const int low2 = 6;
const int alarmPin = A5;
const int gsmRebootPin = 7;
const int debounceInterval = 3000;
int primaryCutoff;
int counter1;
int secondaryCutoff;
int counter2;
int alarm;
int counter3;
int hplcIN = A2;
int hplcOUT = A1;
int hlpcCOMMON;
int hlpcNC;
int counter4;
String message2;
String message3;
char incomingChar;

unsigned long currentMillis = 0;
unsigned long difference = 0;
unsigned long difference2 = 0;
unsigned long difference3 = 0;
unsigned long difference4 = 0;

unsigned long alarmTime = 0;
unsigned long alarmTime2 = 0;
unsigned long alarmTime3 = 0;
unsigned long alarmTime4 = 0;

bool alarmSwitch = false;
bool alarmSwitch2 = false;
bool alarmSwitch3 = false;
bool alarmSwitch4 = false;

unsigned long cycleslong = 0;
unsigned long runHours = 0;

//Set up the modbus module.  It is declared as "node"

//the following lines set up the MAX485 board that is used for ttl to rs485 conversion
#define MAX485_DE 3
#define MAX485_RE_NEG 2
ModbusMaster node;
void preTransmission() // not sure what the purpose of this is
{
  digitalWrite(MAX485_RE_NEG, 1);
  digitalWrite(MAX485_DE, 1);
}
void postTransmission() // also not sure what this does
{
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
}

// Configure software serial port for using the SIM900
AltSoftSerial SIM900;

void setup() {
  pinMode(gsmRebootPin, OUTPUT);
  digitalWrite(gsmRebootPin, LOW);
  GSMstartup();
  Serial.begin(9600);
  SIM900.begin(19200);
  // Give time to your GSM shield log on to network
  delay(10000);
  pinMode(low1, INPUT);
  pinMode(low2, INPUT);
  pinMode(alarmPin, INPUT);
  pinMode(hplcIN, INPUT);
  pinMode(hplcOUT, INPUT);
  pinMode(MAX485_RE_NEG, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);

  //putting these pins to zero sets Modbus module to receive mode.
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
  
  node.begin(1, Serial);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  sendSMS2(F("Systems booted successfully.  Contacts loaded successfully."));
  sendSMS3(F("Systems booted successfully.  Contacts loaded successfully."));
  Serial.println(F("Setup complete.  Entering main loop"));
}
//=================================================================//
//=================================================================//

void loop() 
{
  primaryCutoff = digitalRead(low1);
  secondaryCutoff = digitalRead(low2);
  alarm = digitalRead(alarmPin);
  hlpcCOMMON = digitalRead(hplcIN);
  hlpcNC = digitalRead(hplcOUT);
  currentMillis = millis();

  resetCounters();
  primary_LW();
  secondary_LW();
  HPLC();
  Honeywell_alarm();
  SMSRequest();
}

//-------------------------------------------------------------------------//

void resetCounters()
{
  if (primaryCutoff == LOW)
  {
    alarmSwitch = false;
    difference = 0;
    alarmTime = 0;

    if (counter1 == 1)
     { 
      sendSMS2(F("Primary low water has been reset"));
      counter1 = 0;
     }  
     //if the counter was not 1, it had to be 0.  so setting it to 0 outside
     //of this if statement would be redundant.        
    
  }
  
  if (secondaryCutoff == LOW)
  {
    alarmSwitch2 = false;
    difference2 = 0;
    alarmTime2 = 0;

    if (counter2 == 1)
    {
      sendSMS2(F("Secondary LWC has been reset"));
      counter2 = 0;
    }
    
  }
  if (alarm == LOW)
  {
    alarmSwitch3 = false;
    
    difference3 = 0;
    alarmTime3 = 0;
    if (counter3 == 1)
    {
      sendSMS2(F("Honeywell reset"));
      counter3 = 0;
    }
  }
  if ((hlpcCOMMON == HIGH) && (hlpcNC == HIGH))
  {
    if (counter4 == 1)
    {
      sendSMS2(F("HLPC reset"));
      counter4 = 0;
    }
    
  }
  //this next line may not be necessary, but I think it will help prevent against false alarms on HPLC
  if (hlpcCOMMON == LOW)
  {
    counter4 = 1;
  }
}

void primary_LW()
{
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
      sendSMS2("primary low water alarm");
      sendSMS3("primary low water alarm");
      
      Serial.println("message sent or simulated");
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

    if ((primaryCutoff == LOW) || (counter1 == 1))
    {
      alarmSwitch = false;
      difference = 0;
      alarmTime = 0;
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
      Serial.println("alarmSwitch2 is true");
    }
    difference2 = currentMillis - alarmTime2;

    if ( difference2 >= debounceInterval)
    {
      Serial.println(F("Secondary low water.  Sending message."));
      sendSMS2("Secondary low water alarm");
      sendSMS3("Secondary low water alarm");
      Serial.println("message sent or simulated");
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

    if ((secondaryCutoff == LOW) || (counter2 == 1))
    {
      alarmSwitch2 = false;
      difference2 = 0;
      alarmTime2 = 0;
      return;
    }
  }
}

void HPLC()
{

  if ((hlpcCOMMON == HIGH) && (hlpcNC == LOW) && (counter4 == 0))
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
      sendSMS2(F("High pressure alarm"));
      sendSMS3(F("High pressure alarm"));
      delay(100);
      Serial.println("message sent or simulated");
      counter4 = 1;
      difference4 = 0;
      alarmSwitch4 = false;
      alarmTime4 = 0;
    }
    if (difference4 < debounceInterval)
    {
      Serial.println(difference4);
      return;
    }
  }
  else
  {
    if ((hlpcCOMMON == LOW) || (counter4 == 1) || ((hlpcCOMMON == HIGH) && hlpcNC == (HIGH)))
    {
      alarmSwitch4 = false;
      difference4 = 0;
      alarmTime4 = 0;
      return;
    }
  }
}

void Honeywell_alarm()
{
  if ((alarm == HIGH) && (counter3 == 0))
  {
    if (alarmSwitch3 == false)
    {
      alarmTime3 = currentMillis;
      alarmSwitch3 = true;
      Serial.println("alarmSwitch is true");
    }
    difference3 = currentMillis - alarmTime3;

    if ( difference3 >= debounceInterval)
    {
      Serial.println("sending alarm message");
      sendSMS2(F("Boiler down on alarm."));
      sendSMS3(F("Boiler down on alarm."));
      delay(100);
      readModbus();
      Serial.println("message sent or simulated");
      counter3 = 1;
      difference3 = 0;
      alarmSwitch3 = false;
      alarmTime3 = 0;
    }
    if (difference3 < debounceInterval)
    {
      Serial.println(difference3);
      return;
    }
  }
  else
  {

    if ((alarm == LOW) || (counter3 == 1))
    {
      alarmSwitch3 = false;
      difference3 = 0;
      alarmTime3 = 0;
      return;
    }
  }
}

void sendSMS2(String message2)
{
  // AT command to set SIM900 to SMS mode
  SIM900.print("AT+CMGF=1\r");
  delay(100);

  // REPLACE THE X's WITH THE RECIPIENT'S MOBILE NUMBER
  // USE INTERNATIONAL FORMAT CODE FOR MOBILE NUMBERS
  SIM900.println("AT+CMGS=\"+17065755866\"");
  delay(100);

  // REPLACE WITH YOUR OWN SMS MESSAGE CONTENT
  SIM900.println(message2);
  delay(100);

  // End AT command with a ^Z, ASCII code 26
  SIM900.println((char)26);
  delay(100);
  SIM900.println();
  // Give module time to send SMS
  delay(5000);
}



void sendSMS3(String message3)
{
  // AT command to set SIM900 to SMS mode
  SIM900.print("AT+CMGF=1\r");
  delay(100);

  // REPLACE THE X's WITH THE RECIPIENT'S MOBILE NUMBER
  // USE INTERNATIONAL FORMAT CODE FOR MOBILE NUMBERS
  SIM900.println("AT+CMGS=\"+16158122833\"");
  delay(100);

  // REPLACE WITH YOUR OWN SMS MESSAGE CONTENT
  SIM900.println(message3);
  delay(100);

  // End AT command with a ^Z, ASCII code 26
  SIM900.println((char)26);
  delay(100);
  SIM900.println();
  // Give module time to send SMS
  delay(5000);
}

void SMSRequest()
{
  if (SIM900.available() > 0)
  {
    incomingChar = SIM900.read();
    switch (incomingChar)
    {
      case 'C':
        delay(10);
        Serial.print(incomingChar);
        incomingChar = SIM900.read();
        if (incomingChar == 'H') {
          delay(10);
          Serial.print(incomingChar);
          incomingChar = SIM900.read();
          if (incomingChar == 'E') {
            delay(10);
            Serial.print(incomingChar);
            incomingChar = SIM900.read();
            if (incomingChar == 'C') {
              delay(10);
              Serial.print(incomingChar);
              incomingChar = SIM900.read();
              if (incomingChar == 'K') {
                delay(10);
                Serial.print(incomingChar);
                incomingChar = "";
                Serial.println(F("GOOD CHECK. SMS SYSTEMS ONLINE"));
                Serial.println(F("SENDING CHECK VERIFICATION MESSAGE")) ;
                sendSMS2(F("SMS systems are online"));
                sendSMS3(F("SMS systems are online"));
                //Serial.println("verification message sent");
                break;
              }
            }
          }
        }


      case 'B':
        delay(10);
        Serial.print(incomingChar);
        incomingChar = SIM900.read();
        if (incomingChar == 'E') {
          delay(10);
          Serial.print(incomingChar);
          incomingChar = SIM900.read();
          if (incomingChar == 'A') {
            delay(10);
            Serial.print(incomingChar);
            incomingChar = SIM900.read();
            if (incomingChar == 'N') {
              delay(10);
              Serial.print(incomingChar);
              incomingChar = SIM900.read();
              if (incomingChar == 'S') {
                delay(10);
                Serial.print(incomingChar);
                incomingChar = "";
                sendSMS2("this is a beans message");
                break;
              }
            }
          }
        }

      case 'S':
        delay(10);
        Serial.print(incomingChar);
        incomingChar = SIM900.read();
        if (incomingChar == 'T') {
          delay(10);
          Serial.print(incomingChar);
          incomingChar = SIM900.read();
          if (incomingChar == 'A') {
            delay(10);
            Serial.print(incomingChar);
            incomingChar = SIM900.read();
            if (incomingChar == 'T') {
              delay(10);
              Serial.print(incomingChar);
              incomingChar = SIM900.read();
              if (incomingChar == 'E') {
                delay(10);
                Serial.print(incomingChar);
                incomingChar = "";
                statusReport();
                break;
              }
            }
          }
        }

        case 'R':
        delay(10);
        Serial.print(incomingChar);
        incomingChar = SIM900.read();
        if (incomingChar == 'E') {
          delay(10);
          Serial.print(incomingChar);
          incomingChar = SIM900.read();
          if (incomingChar == 'P') {
            delay(10);
            Serial.print(incomingChar);
            incomingChar = SIM900.read();
            if (incomingChar == 'R') {
              delay(10);
              Serial.print(incomingChar);
              incomingChar = SIM900.read();
              if (incomingChar == 'T') {
                delay(10);
                Serial.print(incomingChar);
                incomingChar = "";
                CyclesandHours();
                break;
              }
            }
          }
        }

      case 'F':
        delay(10);
        Serial.print(incomingChar);
        incomingChar = SIM900.read();
        if (incomingChar == 'L') {
          delay(10);
          Serial.print(incomingChar);
          incomingChar = SIM900.read();
          if (incomingChar == 'A') {
            delay(10);
            Serial.print(incomingChar);
            incomingChar = SIM900.read();
            if (incomingChar == 'M') {
              delay(10);
              Serial.print(incomingChar);
              incomingChar = SIM900.read();
              if (incomingChar == 'E') {
                delay(10);
                Serial.print(incomingChar);
                incomingChar = "";
                sendSMSreport();
                break;
              }
            }
          }
        }

      case 'A':
        delay(10);
        Serial.print(incomingChar);
        incomingChar = SIM900.read();
        if (incomingChar == 'B') {
          delay(10);
          Serial.print(incomingChar);
          incomingChar = SIM900.read();
          if (incomingChar == 'O') {
            delay(10);
            Serial.print(incomingChar);
            incomingChar = SIM900.read();
            if (incomingChar == 'R') {
              delay(10);
              Serial.print(incomingChar);
              incomingChar = SIM900.read();
              if (incomingChar == 'T') {
                delay(10);
                Serial.print(incomingChar);
                incomingChar = "";
                sendSMS2(F("Shutting down SMS systems now.  Manual reset of SIM module required."));
                abortSMS();
                return;

              }
            }
          }
        }
    }
  }
}

void readModbus()
{
  Serial.println("In the readModbus() function now");
  delay(500);
  uint16_t result;

  result = node.readHoldingRegisters (0x0000, 1);
  if (result == node.ku8MBSuccess)
  {

    int alarmRegister = node.getResponseBuffer(result);


    switch (alarmRegister)
    {

      case 1:
        sendSMS2(F("Fault Code 1: No Purge Card"));
        sendSMS3(F("Fault Code 1: No Purge Card"));
        break;
      case 14:
        sendSMS2(F("Fault code 14\r\nHigh Fire Interlock Switch failure to close during Prepurge"));
        break;
      case 15:
        sendSMS2(F("Fault code 15\r\nFlame present when no flame expected"));
        break;
      case 17:
        sendSMS2(F("Fault code 17\r\nMain flame failure\r\nFLame failure during RUN"));
        sendSMS3(F("Fault code 17\r\nMain flame failure\r\nFLame failure during RUN"));
        break;
      case 19:
        sendSMS2(F("Fault code 19\r\nMain Flame Failure"));
        sendSMS3(F("Fault code 19\r\nMain Flame Failure"));
        break;
      case 20:
        sendSMS2(F("Fault code 20\r\nLow Fire Interlock switch failure"));
        sendSMS3(F("Fault code 20\r\nLow Fire Interlock switch failure"));
        break;
      case 28:
        sendSMS2(F("Fault code 28\r\nPilot Flame Failure"));
        sendSMS3(F("Fault code 28\r\nPilot Flame Failure"));
        break;
      case 29:
        sendSMS2(F("Fault code 29\r\nLockout Interlock\r\nCheck blower rotation.\r\nCheck air switch."));
        sendSMS3(F("Fault code 29\r\nLockout Interlock\r\nCheck blower rotation.\r\nCheck air switch."));

        break;
      case 46:
        sendSMS2(F("Fault code 46\r\nFlame Amplifier\r\nEnsure flame amplifier is installed properly"));
        break;
      case 47:
        sendSMS2(F("Fault code 47\r\nJumpers Changed\r\nJumper configuration has been changed since startup"));
        break;
      default:
        sendSMS2(F("boiler in alarm state but no Modbus number available"));
        sendSMS3(F("boiler in alarm state but no Modbus number available"));
        break;
        Serial.println("End of readModbus() function");
    }
  }
  else
  {
    sendSMS2(F("Boiler in alarm state but communication with boiler controller has failed"));
  }
}

void sendSMSreport()
{
  delay(400);
  uint16_t result;
  delay(400);
  result = node.readHoldingRegisters (0x000A , 1);
  if (result == node.ku8MBSuccess)
  {
    float fSignal = ((node.getResponseBuffer(result)) * .048);

    delay(1000);
    // REPLACE THE X's WITH THE RECIPIENT'S MOBILE NUMBER
    // USE INTERNATIONAL FORMAT CODE FOR MOBILE NUMBERS
    SIM900.println("AT+CMGS=\"+17065755866\"");
    delay(100);

    // Send the SMS text message
    SIM900.print(fSignal);
    delay(100);
    // End AT command with a ^Z, ASCII code 26
    SIM900.println((char)26);
    delay(100);
    SIM900.println();
    // Give module time to send SMS
    delay(5000);
  }
  else
  {
    sendSMS2(F("request received but no data"));
  }
}


void statusReport()
{
  delay(300);
  uint16_t result2;
  delay(300);
  uint16_t checkAlarm;
  
  checkAlarm = node.readHoldingRegisters (0x0000, 1);
  if (checkAlarm == node.ku8MBSuccess)
  {
    int alarmNum = node.getResponseBuffer(checkAlarm);

    if (alarmNum != 0)
    {
      Serial.print("The alarmNum value is: ");
      Serial.println(alarmNum);
      Serial.println("STATE request received but boiler is in alarm state");
      Serial.println("running readModbus() function");
      delay(300);
      readModbus();
      Serial.println("readModbus() function complete.  Text should have been sent");
    }

    else if (alarmNum == 0)
    {
      delay(400);
      result2 = node.readHoldingRegisters (0x0002 , 1);
      if (result2 == node.ku8MBSuccess)
      {
        int Sreport = ((node.getResponseBuffer(result2)));
        
        switch (Sreport)
        {

          case 6:
            sendSMS2(F("Standby"));
            sendSMS3(F("Standby"));
            break;
          case 8:
            sendSMS2(F("Standby Hold: Start Switch"));
            break;
          case 9:
            sendSMS2(F("Standby Hold: Flame Detected"));
            break;
          case 12:
            sendSMS2(F("Standby Hold: T7 Running Interlock "));
            break;
          case 13:
            sendSMS2(F("Standby Hold: Airflow Switch"));
            break;
          case 14:
            sendSMS2(F("Purge Hold: T19 High Fire Switch"));
            break;
          case 15:
            sendSMS2(F("Purge Delay: T19 High Fire Jumpered"));
            break;
          case 16:
            sendSMS2(F("Purge Hold: Test Switch"));
            break;
          case 17:
            sendSMS2(F("Purge Delay: Low Fire Jumpered"));
            break;
          case 18:
            sendSMS2(F("Purge Hold: Flame Detected"));
            break;
          case 19:
            sendSMS2(F("Purge"));
            sendSMS3(F("Purge"));
            break;
          case 20:
            sendSMS2(F("Low Fire Switch"));
            break;
          case 22:
            sendSMS2(F("Purge Hold: Lockout Interlock"));
            break;
          case 25:
            sendSMS2(F("Pilot Ignition"));
            break;
          case 28:
            sendSMS2(F("Main Ignition"));
            break;
          case 30:
            sendSMS2(F("Run"));
            sendSMS3(F("Run"));
            break;
          case 35:
            sendSMS2(F("Postpurge"));
            sendSMS3(F("Postpurge"));
            break;
          case 45:
            sendSMS2(F("Pre-ignition"));
            break;
          default:
            sendSMS2(F("Request received, but no data available"));
            break;

        }
      }
    }
  }
  else
  {
    sendSMS2(F("request received but no data"));
  }
}

void rebootGSM()
{
  digitalWrite (gsmRebootPin, HIGH);
  delay(3000);
  digitalWrite (gsmRebootPin, LOW);
  delay(3000);
  digitalWrite (gsmRebootPin, HIGH);
  delay(3000);
  digitalWrite (gsmRebootPin, LOW);
}

void GSMstartup()
{
  digitalWrite (gsmRebootPin, HIGH);
  delay(3000);
  digitalWrite (gsmRebootPin, LOW);
}

void abortSMS()
{
  digitalWrite (gsmRebootPin, HIGH);
  delay(3000);
  digitalWrite (gsmRebootPin, LOW);
}


void CyclesandHours()
{
    uint32_t cycleCheck; //for the honeywell 7800 series, two registers must be read for U32 format
    uint32_t runHoursCheck;
    delay(200);
    cycleCheck = node.readHoldingRegisters (0x0006, 2);
    delay(200);
    if (cycleCheck == node.ku8MBSuccess)
    {    
      cycleslong = node.getResponseBuffer(cycleCheck);
      Serial.println(cycleslong);
      //sendSMS2(F(cycleslong));


     SIM900.print("AT+CMGF=1\r");
  delay(100);

  // REPLACE THE X's WITH THE RECIPIENT'S MOBILE NUMBER
  // USE INTERNATIONAL FORMAT CODE FOR MOBILE NUMBERS
  SIM900.println("AT+CMGS=\"+17065755866\"");
  delay(100);

  // REPLACE WITH YOUR OWN SMS MESSAGE CONTENT
  SIM900.println(cycleslong);
  delay(100);

  // End AT command with a ^Z, ASCII code 26
  SIM900.println((char)26);
  delay(100);
  SIM900.println();
  // Give module time to send SMS
  delay(5000);
}
      
    
    delay(200);
    runHoursCheck = node.readHoldingRegisters (0x0008, 2);
    delay(200);
    if (runHoursCheck == node.ku8MBSuccess)
    {
      runHours = node.getResponseBuffer(runHoursCheck);
      Serial.println(runHours);
      //sendSMS2(F(runHours));
 SIM900.print("AT+CMGF=1\r");
  delay(100);

  // REPLACE THE X's WITH THE RECIPIENT'S MOBILE NUMBER
  // USE INTERNATIONAL FORMAT CODE FOR MOBILE NUMBERS
  SIM900.println("AT+CMGS=\"+17065755866\"");
  delay(100);

  // REPLACE WITH YOUR OWN SMS MESSAGE CONTENT
  SIM900.println(runHours);
  delay(100);

  // End AT command with a ^Z, ASCII code 26
  SIM900.println((char)26);
  delay(100);
  SIM900.println();
  // Give module time to send SMS
  delay(5000);
}




      
    

    
    
}
