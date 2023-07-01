// get sensor vals and print in serial monitor

#include <Wire.h>
#include <DHT.h>
#include <SPI.h>
#include <SD.h>

#define DS1307_ADDRESS 0x68 	// rtc address
#define SCD_ADDRESS 0x62		// co2 address
#define DHTPIN 2
#define DHTTYPE DHT22
#define CS    7     // adjust this ChipSelect line if needed !

int currentTime[7];

DHT dht(DHTPIN, DHTTYPE);

void setup()
{
  Serial.begin(9600);
  Wire.begin();
  delay(1000);

  //setDateTime(23, 7, 1, 7, 19, 45, 0);
}

void loop()
{
  float co2, co, temp, humi;

  co2 = co2_concentration();
  co = co_concentration();
  temp = room_temperature();
  humi = room_humidity();

  addValuesToSDCard(co2, co, temp, humi);
}

// CO2 ===============================================================================================================
float co2_concentration(){
  float co2;
  uint8_t data[12], counter = 0;

  Wire.beginTransmission(SCD_ADDRESS);
  Wire.write(0x36); // wake up
  Wire.write(0xf6);
  Wire.endTransmission();

  delay(30);

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

// CO Sensor ===============================================================================================================
float co_concentration()
{
	float co_conc = 0;
	int offset = 0.15; //150 mV constant offset
	
	analogReference(INTERNAL);
	
	for(int co_cnt = 1; co_cnt <= 50; co_cnt++){
		co_conc += analogRead(A0);
		delay(1);
	}
	
	co_conc = ( co_conc * 1.1 / 1023 ) / 50;
	
	analogReference(DEFAULT);
	
	return co_conc;
}

// Temperature + humidity sensor ============================================================================================

float room_temperature()
{
	float room_tmp = dht.readTemperature();
	
	if(isnan(room_tmp)){
			Serial.println("Error reading DHT22_Temp");
      return -1;
	}else{
		return room_tmp;
	}
}

float room_humidity()
{
	float room_humidi = dht.readHumidity();
	
	if(isnan(room_humidi)){
			Serial.println("Error reading DHT22_Temp");
      return -1;
	}else{
		return room_humidi;
	}
}

void addValuesToSDCard(float co2, float co, float temp, float humi){ // make to function write buffer to sd card
  if (!SD.begin(CS))
  {
    Serial.println("addVal Error: SD card failure");
  }

  File logfile = SD.open("data.txt", FILE_WRITE);
  if (!logfile)
  {
    Serial.println("addVal2 Error: SD card failure");
  }

  retreive_RTC_time(currentTime);

  char buffer[200];
  // index hour minute  sec day month year  temp  humi  co  co2 batLevel
  
  sprintf(buffer, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d", currentTime[2], currentTime[1], currentTime[0], currentTime[4], currentTime[5], currentTime[6], (int) co2, (int) co, (int) temp, (int) humi);

  logfile.println(buffer);

  logfile.close();
}

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
