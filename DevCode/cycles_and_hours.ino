#include <SPI.h>
#include <ModbusMaster.h>

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


void setup() {
  
  pinMode(MAX485_RE_NEG, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);

  //putting these pins to zero sets Modbus module to receive mode.
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
  
  node.begin(1, Serial);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
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
                //sendSMSreport();
                get_flame_signal();
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

float get_flame_signal()
{
  uint16_t result = node.readHoldingRegisters (0x000A , 1);//As per the S7800A1146 Display manual 
  if (result == node.ku8MBSuccess)
  {
    float flameSignal = node.getResponseBuffer(result);
    flameSignal = map(flameSignal,0,255,0,25.5)//As per the S7800A1146 Display manual 
    return flameSignal;
  }
  else
  {
    printf("***ERROR Flame signal retrival failed.***\n");
    return -1.0;
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
