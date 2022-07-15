

void loadContacts()
{

  String URLheader = "";
  String conFrom1 = "";
  String conFrom2 = "";
  String conTo1 = "";
  String conTo2 = "";
  String conTo3 = "";
  String conToTotal = "To=";

  //---------------------------------------------//
  //load "from" number.  This is the number alert messages will apear to be from-------------//

conFrom1 = fill_from_SD("from1.txt");
conFrom1.toCharArray(contactFromArray1, 25);
conFrom2 = fill_from_SD("from2.txt");
conFrom2.toCharArray(contactFromArray2, 25);
Serial.print("From nums: ");
Serial.print(contactFromArray1);
Serial.println(contactFromArray2);
conTo1 = fill_from_SD("To1.txt");
if (conTo1[0] > 0) {
  conToTotal += conTo1;
}
conTo2 = fill_from_SD("To2.txt");
if (conTo2[0] > 0) {
  conToTotal += "," + conTo2;
}
conTo3 = fill_from_SD("To3.txt");
if (conTo3[0] > 0) {
  conToTotal += "," + conTo3;
}

conToTotal += "&";    //format the "to" list of numbers for being in the URL by ending it with '&' so that next parameter can come after

Serial.print(F("The total contact list is: "));
Serial.println(conToTotal);
Serial.print("fourth position character: ");
Serial.println(conToTotal[3]);

if (conToTotal[3] == ',')
{
  conToTotal.remove(3, 1);
  Serial.print(F("New contact list: "));
  Serial.println(conToTotal);
}

conToTotal.toCharArray(conToTotalArray, 60);

URLheader = fill_from_SD("URL.txt");
URLheader.toCharArray(urlHeaderArray, 100);
Serial.print("URL header is: ");
Serial.println(urlHeaderArray);

}


String fill_from_SD(String file_name)
{
  String temporary_string = "";
  String info_from_SD = "";
  myFile = SD.open(file_name);
  if (myFile) {

    // read from the file until there's nothing else in it:
    while (myFile.available())
    {
      char c = myFile.read();  //gets one byte from serial buffer
      info_from_SD += c;
    }

    myFile.close();
    return info_from_SD;

  }
  else
  {
    // if the file didn't open, print an error:
    Serial.println("error opening file");
  }
  
}
