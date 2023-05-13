#include <SPI.h>
#include <SD.h>
#include <XMLWriter.h>
// SPI       PINS
// MOSI       11
// MISO       12
// CLOCK      13
// CS         10
#define CS    7     // adjust this ChipSelect line if needed !

char buffer[24];

void initSDcard(){
  // initialize the SD card
  if (!SD.begin(CS))
  {
    Serial.println("init1 Error: SD card failure"); // Fails here
  }
/*
// remove file for proper timing
SD.remove("data.xml");
delay(1000);
*/

  File logfile = SD.open("data.xml", FILE_WRITE);
  if (!logfile)
  {
    Serial.println("init2Error: SD card failure");
  }

  XMLWriter XML(&logfile);
  XML.header();
  XML.comment("File just to get in touch with XML Writer", true);
  XML.comment("A0 --- A1 -- A2");
  XML.flush();

  logfile.close();

  Serial.println("Setup done done...");
}

void addValuesToSDCard(){
  if (!SD.begin(CS))
  {
    Serial.println("addVal Error: SD card failure");
  }


  File logfile = SD.open("data.xml", FILE_WRITE);
  if (!logfile)
  {
    Serial.println("addVal2 Error: SD card failure");
  }

  XMLWriter XML(&logfile);
  int data[3];

  data[0] = analogRead(A0);
  data[1] = analogRead(A1);
  data[2] = analogRead(A2);

  XML.tagStart("SensorData");
  XML.tagField("overwrite", "yes");
  XML.tagField("A0", (uint16_t) data[0]);
  XML.tagField("A1", (uint16_t) data[1]);
  XML.tagField("A2", (uint16_t) data[2]);
  XML.tagEnd(); 
  XML.flush();

  logfile.close();
}

void setup()
{
  Serial.begin(9600);

  initSDcard();
}
void loop()
{
  addValuesToSDCard();
  delay(1000);
}