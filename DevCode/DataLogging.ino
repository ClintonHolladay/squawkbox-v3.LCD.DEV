#include <SD.h>   

void setup() 
{
  Serial.begin(9600);
  boot_SDcard();  
}

void loop() 
{
  Data_Logger();
}

void Data_Logger(const char& FAULT[]) 
{
  DateTime now = rtc.now();
  File dataFile = SD.open("DataLogger.txt", FILE_WRITE);
  if (dataFile) 
  {
    dataFile.print(FAULT);
    dataFile.print(",");
    dataFile.print(now.year());  
    dataFile.print(",");      
    dataFile.print(now.month());  
    dataFile.print(",");
    dataFile.print(now.day());  
    dataFile.print(",");
    dataFile.print(now.hour());  
    dataFile.print(",");
    dataFile.print(now.minute());  
    dataFile.print(",");
    dataFile.print(now.second());  
    dataFile.print(",");
    dataFile.println();  //End of Row move to next row
    dataFile.close();    //Close the file
    printf("Data Log Complete.\n");
  } 
  else
  {
    printf("Data Log failed");
  }
}

void boot_SDcard() {
  // see if the card is present and can be initialized:
  if (!SD.begin(10)) {
    printf("Card failed or not present");
    return;
  }
  File dataFile = SD.open("DataLogger.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.println("Fault,Year,Month,Day,Hour,Minute,Second");
    dataFile.close();
  }
}
