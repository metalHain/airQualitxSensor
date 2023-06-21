// includes =================================================================================

#include <Wire.h>
#include <SPI.h>
#include <SD.h>

// defines ==================================================================================

#define DS1307_ADDRESS 0x68 	// rtc address
#define SCD_ADDRESS 0x62	// co2 address
// SPI       PINS
// MOSI       11
// MISO       12
// CLOCK      13
// CS         10
#define CS    7     // adjust this ChipSelect line if needed !

// global variables =========================================================================

int retreivedTime[7];
int currentTime[7];
int i = 0;
double bat_ADC;
double batLevel;

struct sensedData{
  int sync_MayRead;
  byte hour;
  byte minute;
  byte second;
  byte day;
  byte month;
  byte year;
  byte co;
  byte co2;
  byte temperature;
  byte humidity;
  double batLevel;
}

// function prototypes ======================================================================

// RTC Clock
void set_RTC_time(byte year, byte month, byte monthday, byte weekday, byte hour, byte minute, byte second);
void retreive_RTC_time(int nowTime[7]);
int decToBcd(int value);
int bcdToDec(int value);

// CO2 Sensor
float co2_concentration();

// SD Card
void initSDcard();
void addValuesToSDCard(sensedData values);
void deleteFileOnSDCard(char * fileName);

// CO Sensor
// Temperature + humidity sensor
// Display
// error handling

// functions definitions ====================================================================

// RTC ===============================================================================================================
void setDateTime(byte year, byte month, byte monthday, byte weekday, byte hour, byte minute, byte second)
{
  second = 0;

  // The following codes transmits the data to the RTC
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(byte(0));
  Wire.write(decToBcd(byte(second)));
  Wire.write(decToBcd(byte(minute)));
  Wire.write(decToBcd(byte(hour)));
  Wire.write(decToBcd(byte(hour)));
  Wire.write(decToBcd(byte(monthday)));
  Wire.write(decToBcd(byte(month)));
  Wire.write(decToBcd(byte(year)));
  Wire.write(byte(0));
  Wire.endTransmission();
}

void retreive_RTC_time(int nowTime[7])
{
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(0x00);
  Wire.endTransmission();
  Wire.requestFrom(DS1307_ADDRESS, 7);

  while(Wire.available()){
    nowTime[0] = bcdToDec(Wire.read());              // seconds
    nowTime[1] = bcdToDec(Wire.read());              // minute
    nowTime[2] = bcdToDec(Wire.read() & 0b111111);   // hour
    nowTime[3] = bcdToDec(Wire.read());              // weekday
    nowTime[4] = bcdToDec(Wire.read());              // monthday
    nowTime[5] = bcdToDec(Wire.read());              // month
    nowTime[6] = bcdToDec(Wire.read());              // year
  }
}

int decToBcd(int value)
{
  return ((value/10*16) + (value%10));
}

int bcdToDec(int value)
{
  return ((value/16*10) + (value%16));
}

// CO2 ===============================================================================================================
float co2_concentration(){
  float co2;
  uint8_t data[12], counter = 0;

  Wire.beginTransmission(SCD_ADDRESS);
  Wire.write(0x21); // measure single shot max delay 5000 ms
  Wire.write(0x9d);
  Wire.endTransmission();

  delay(5000);

  Wire.beginTransmission(SCD_ADDRESS);
  Wire.write(0xec); // send read data command
  Wire.write(0x05);
  Wire.endTransmission();
 
  Wire.requestFrom(SCD_ADDRESS, 12);

  while (Wire.available()) {
    data[counter++] = Wire.read();
  }
  
  // floating point conversion according to datasheet
  co2 = (float)((uint16_t)data[0] << 8 | data[1]);

  return co2;
}

// SD ===============================================================================================================
void initSDcard(sensedData values){
  if (!SD.begin(CS))
  {
    Serial.println("init1 Error: SD card failure");
  }

  File logfile = SD.open("data.txt", FILE_WRITE);
  if (!logfile)
  {
    Serial.println("init2Error: SD card failure");
  }

  logfile.println("hour minute  day month year  temp  humi  co  co2 batLevel");

  logfile.close();

  Serial.println("Setup done done...");
}

void addValuesToSDCard(){
  if (!SD.begin(CS))
  {
    Serial.println("addVal Error: SD card failure");
  }

  File logfile = SD.open("data.txt", FILE_WRITE);
  if (!logfile)
  {
    Serial.println("addVal2 Error: SD card failure");
  }

  printDateTime(currentTime);

  char buffer[200];
  // hour minute  day month year  temp  humi  co  co2 batLevel

  bat_ADC = 0;
  batLevel = 0;

  for(int z = 1; z <= 200; z++){
    bat_ADC += analogRead(A0);
  }

  bat_ADC = bat_ADC / 200;
  batLevel = bat_ADC * 5.0 / 1023;
  
  sprintf(buffer, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t", i, currentTime[2], currentTime[1], currentTime[4], currentTime[5], currentTime[6], bat_ADC, bat_ADC, bat_ADC, bat_ADC);

  logfile.print(buffer);

  logfile.println(batLevel);

  Serial.println( batLevel);

  logfile.close();
  i++;
}
