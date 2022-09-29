/*


   ###Connection with SD card module###
  Vcc->5V
  Gnd->Gnd
  MISO->pin 12
  MOSI->pin 11
  SCK-> pin 13
  CS-> pin 10 for adafruit shield

   ###Connection with DS3231###
  Vcc->5V
  Gns->Gnd
  SCL->pin A5
  SDA-> pin A4

   ###Connection with DT11###
  Vcc->5V
  Gnd->Gnd
  Out-> pin 7


*/


#include "RTClib.h" //Library for RTC module (Download from Link in article)

#include <SD.h> //Library for SD card (Pre-Loaded into Arduino)
#include <dht.h> //Library for dht11 Temperature and Humidity sensor (Download from Link in article)


#define DHT11_PIN 7 //Sensor output pin is connected to pin 7
#define DHT11_PIN2 9 // second sensor is on pin 8
dht DHT; //Sensor object named as DHT
dht DHT2; //Second sensor is named DHT2

const int chipSelect = 10;


// Init the DS3231 using the hardware interface

RTC_PCF8523 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};


void setup()
{
  // Setup Serial connection
  Serial.begin(9600);
  Initialize_SDcard();

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  Serial.println("RTC setting");
  // following line sets the RTC to the date & time this sketch was compiled
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  //rtc.adjust(DateTime(2020, 4, 21, 9, 53, 0));
  Serial.println("this is test");
  DateTime now = rtc.now();
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.println("");
}


void loop()
{

  Write_SDcard();

  delay(5000); //Wait for 5 seconds before writing the next data
}


void Write_SDcard()
{
  int chk = DHT.read11(DHT11_PIN);
  int chk2 = DHT2.read11(DHT11_PIN2);
  int temp = DHT.temperature;
  int temp2 = DHT2.temperature;
  int tempF2 = ((temp2 * (1.8)) + 32);
  int tempF = ((temp * (1.8)) + 32);
  DateTime now = rtc.now();
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("LoggerCD.txt", FILE_WRITE);


  // if the file is available, write to it:
  if (dataFile) {
    dataFile.print(now.hour()); //Store date on SD card
    dataFile.print(':');
    dataFile.print(now.minute());
    dataFile.print(","); //Move to next column using a ","

    dataFile.print(tempF); //Store date on SD card
    dataFile.print(","); //Move to next column using a ","

    dataFile.print(tempF2);
    dataFile.print(",");

    dataFile.print(DHT.humidity); //Store date on SD card
    dataFile.print(","); //Move to next column using a ","

    dataFile.print(DHT2.humidity);
    dataFile.print(",");


    dataFile.println(); //End of Row move to next row
    dataFile.close(); //Close the file
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.println("");
    Serial.print("temp 1 = ");
    Serial.println(tempF);
    Serial.print("temp 2 = ");
    Serial.println(tempF2);
    Serial.print("hum 1 = ");
    Serial.println(DHT.humidity);
    Serial.print("hum 2 = ");
    Serial.println(DHT2.humidity);
  }
  else
    Serial.println("OOPS!! SD card writing failed");
}


void Initialize_SDcard()
{
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("LoggerCD.txt", FILE_WRITE);
  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println("Time,Temperature,Temperature2,Humidity,Humidity2"); //Write the first row of the excel file
    dataFile.close();
  }
}
