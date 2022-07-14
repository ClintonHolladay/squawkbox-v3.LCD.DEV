#include <SD.h>   
#include <RTClib.h>
#include <LibPrintf.h>

void setup() 
{
  Serial.begin(9600);
  boot_SD();  
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
    dataFile.print(now.unixtime());
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
    printf("Data Log failed.\n");
  }
}

void boot_SD() //see if the card is present and can be initialized
{
  if (!SD.begin(10)) 
  {
    printf("SD Card failed or not present.\n");
  }
  else
  {
    File dataFile = SD.open("DataLogger.txt", FILE_WRITE);
    if (dataFile) 
    {
      if(!dataFile.position())
      {
        dataFile.println("Fault,Year,Month,Day,Hour,Minute,Second");
        dataFile.close();
        printf("SD Prior initialization recognition complete.\n");
      }
      else
      {
        DateTime now = rtc.now();
        dataFile.print("SystemRESET");
        dataFile.print(",");
        dataFile.print(now.unixtime());
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
        printf("SD SystemRESET Data Log Complete.\n");
      }
    }
    else
    {
      printf("SD Prior initialization recognition OR SystemRESET Data Log Failed.\n");
    }
  }
}
