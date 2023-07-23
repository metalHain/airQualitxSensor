#include "Arduino.h"
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include "DHT.h"

#define DS1307_ADDRESS 0x68 	// rtc address
#define SCD_ADDRESS 0x62		// co2 address
#define CS    7
#define MAX_AVG_BUF_SIZE 2

#define BAT_VOLTAGE_PIN A1
#define CO_PIN A0

DHT dht(2, DHT22);

byte error;
byte len;
File sdFile;

int track_Hours;
int trackedElements;
float co_avg_tmp, co2_avg_tmp, temp_avg_tmp, humi_avg_tmp;
float temp_hum_val[2] = {0};    

// basic data structure
struct sensedData{
  byte hour;
  byte minute;
  byte second;
  byte day;
  byte month;
  byte year;
  float co;				// ppm
  float co2;				// ppm
  float temperature;		// degrees celcius
  float humidity;		// percent
};

struct sensedDataMini{
  byte hour;
  float co;				// ppm
  int co2;				// ppm
  float temperature;		// degrees celcius
  float humidity;		// percent
};

static int currentTime[7];

typedef struct{
  struct sensedDataMini average[MAX_AVG_BUF_SIZE];
  struct sensedDataMini min[MAX_AVG_BUF_SIZE];
  struct sensedDataMini max[MAX_AVG_BUF_SIZE];
  int write_idx_avg_buf;
} CircularBuffer_avg;

static CircularBuffer_avg circBuf_avg;

static struct sensedData sensorVals;

void setup() {
  Serial.begin(9600);

  initSDcard();

  dht.begin();  

  Wire.begin();
  Wire.setWireTimeout(3000 /* us */, true /* reset_on_timeout */);

  initCircBuffer_avg();
  retrieve_RTC_time(currentTime);

  track_Hours = currentTime[2];
  trackedElements = 0;

  // wait until sensors are ready, > 1000 ms according to datasheet
  delay(1000);
  
  // start scd measurement in periodic mode, will update every 30 s
  Wire.beginTransmission(SCD_ADDRESS);
  Wire.write(0x21);
  Wire.write(0xac);
  Wire.endTransmission();

  // wait for first measurement to be finished
  delay(5000);

  Serial.println(F("setup done"));
}

void loop() {
  // put your main code here, to run repeatedly:
  datalogging();
}

void datalogging()
{
  char log[60] = {};

  retrieve_RTC_time(currentTime);

  // Get sensor values
  sensorVals.co = co_concentration();
  sensorVals.co2 = co2_concentration();
  room_temperature_humidity();
  sensorVals.temperature = temp_hum_val[1];
  sensorVals.humidity = temp_hum_val[0];
  float batVoltage = bat_Voltage();

  char co_string[9];
  dtostrf(sensorVals.co, 4, 4, co_string);

  char co2_string[8];
  dtostrf(sensorVals.co2, 4, 0, co2_string);

  char temperature_string[8];
  dtostrf(sensorVals.temperature, 4, 2, temperature_string);

  char humidity_string[8];
  dtostrf(sensorVals.humidity, 4, 2, humidity_string);

  sprintf(log, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%s\t%s\0", currentTime[2], currentTime[1], currentTime[0], currentTime[4], currentTime[5], currentTime[6], co_string, co2_string, temperature_string, humidity_string);

  Serial.println(log);

  addValuesToSDCard(log);
  add_values_to_AVG_buf(currentTime[2], sensorVals);

  delay(30000); 
}

// --------------------------------------------------------------------------------- +++++++++++++++++++++++++++++++++++++++++++++++ ------------------------------------------------------------------------
// RTC

void set_RTC_time(byte year, byte month, byte monthday, byte weekday, byte hour, byte minute, byte second)
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
  error = Wire.endTransmission();

  if (error) {
    printError("I2C error occured when writing");
    if (error == 5)
      printError("I2C error occured - It was a timeout");
  }
}

void retrieve_RTC_time(int nowTime[7])
{
  Wire.clearWireTimeoutFlag();

  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(0x00);
  Wire.endTransmission();
  len = Wire.requestFrom(DS1307_ADDRESS, 7);

  if (len == 0) {
    printError("I2C error occured when reading");

    if (Wire.getWireTimeoutFlag())
      printError("I2C error occured - It was a timeout");
  }

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

// --------------------------------------------------------------------------------- +++++++++++++++++++++++++++++++++++++++++++++++ ------------------------------------------------------------------------
// SD

void initSDcard(){
  if (!SD.begin(7)) {
    Serial.println(F("initialization failed!"));
  }

  Serial.println(F("initialization done."));
}


void addValuesToSDCard(char * sdbuffer){ // write line of data to sd card file data.txt
  sdFile = SD.open("datLog.txt", FILE_WRITE);

  // if the file opened okay, write to it:
  if (sdFile) {
    Serial.print(F("Writing to datLog.txt..."));
    sdFile.println(sdbuffer);
    // close the file:
    sdFile.close();
    Serial.println(F("done."));
  } else {
    // if the file didn't open, print an error:
    Serial.println(F("error opening datLog.txt"));
  }
}

// --------------------------------------------------------------------------------- +++++++++++++++++++++++++++++++++++++++++++++++ ------------------------------------------------------------------------
// circular buffer

// --------------------------------------------------------------------------------- +++++++++++++++++++++++++++++++++++++++++++++++ ------------------------------------------------------------------------
// init

void initCircBuffer_avg(){
  circBuf_avg.write_idx_avg_buf = 0;
  
  for(int i=0; i<7; i++){
    circBuf_avg.min[i].co = -1;
    circBuf_avg.min[i].co2 = -1;
    circBuf_avg.min[i].temperature = -1;
    circBuf_avg.min[i].humidity = -1;
  }
}

// --------------------------------------------------------------------------------- +++++++++++++++++++++++++++++++++++++++++++++++ ------------------------------------------------------------------------
// fill with vals

void add_values_to_AVG_buf(int currentHour, struct sensedData values){
  Serial.println(F("now add_values_to_AVG_buf"));

  if(trackedElements == 0) // new hour has started -> init buffer values
  {
    co2_avg_tmp  = 0;
    co2_avg_tmp  = 0;
    temp_avg_tmp = 0;
    humi_avg_tmp = 0;
  } 

  if(track_Hours == currentHour) // add sensor Values to avg
  {
    if(circBuf_avg.write_idx_avg_buf == MAX_AVG_BUF_SIZE) // buffer full -> reset write_idx
    {
      circBuf_avg.write_idx_avg_buf = 0;
    }

    // sum up sensor values over one hour
    co_avg_tmp += values.co;
    co2_avg_tmp += values.co2;
    temp_avg_tmp += values.temperature;
    humi_avg_tmp += values.humidity;

    // max values
    if(values.co > circBuf_avg.max[circBuf_avg.write_idx_avg_buf].co){ circBuf_avg.max[circBuf_avg.write_idx_avg_buf].co = values.co; }
    if((values.co2 > circBuf_avg.max[circBuf_avg.write_idx_avg_buf].co2) && (values.co2 <= 5000)){ circBuf_avg.max[circBuf_avg.write_idx_avg_buf].co2 = values.co2; }
    if(values.temperature > circBuf_avg.max[circBuf_avg.write_idx_avg_buf].temperature){ circBuf_avg.max[circBuf_avg.write_idx_avg_buf].temperature = values.temperature; }
    if(values.humidity > circBuf_avg.max[circBuf_avg.write_idx_avg_buf].humidity){ circBuf_avg.max[circBuf_avg.write_idx_avg_buf].humidity = values.humidity; }

    // min values
    if(circBuf_avg.min[circBuf_avg.write_idx_avg_buf].co == -1) { circBuf_avg.min[circBuf_avg.write_idx_avg_buf].co = values.co; }
    else if(circBuf_avg.min[circBuf_avg.write_idx_avg_buf].co > values.co) { circBuf_avg.min[circBuf_avg.write_idx_avg_buf].co = values.co; }

    if(circBuf_avg.min[circBuf_avg.write_idx_avg_buf].co2 == -1) { circBuf_avg.min[circBuf_avg.write_idx_avg_buf].co2 = values.co2; }
    else if(circBuf_avg.min[circBuf_avg.write_idx_avg_buf].co2 > values.co2) { circBuf_avg.min[circBuf_avg.write_idx_avg_buf].co2 = values.co2; }

    if(circBuf_avg.min[circBuf_avg.write_idx_avg_buf].temperature == -1) { circBuf_avg.min[circBuf_avg.write_idx_avg_buf].temperature = values.temperature; }
    else if(circBuf_avg.min[circBuf_avg.write_idx_avg_buf].temperature > values.temperature) { circBuf_avg.min[circBuf_avg.write_idx_avg_buf].temperature = values.temperature; }

    if(circBuf_avg.min[circBuf_avg.write_idx_avg_buf].humidity == -1) { circBuf_avg.min[circBuf_avg.write_idx_avg_buf].humidity = values.humidity; }
    else if(circBuf_avg.min[circBuf_avg.write_idx_avg_buf].humidity > values.humidity) { circBuf_avg.min[circBuf_avg.write_idx_avg_buf].humidity = values.humidity; }

    trackedElements++;

    Serial.print("Min CO2: ");
    Serial.print(circBuf_avg.min[circBuf_avg.write_idx_avg_buf].co2);
    Serial.print(" Max: CO2: ");
    Serial.println(circBuf_avg.max[circBuf_avg.write_idx_avg_buf].co2);

    Serial.print("WriteIdx: ");
    Serial.print(circBuf_avg.write_idx_avg_buf);
    Serial.print(" TrackedItems: ");
    Serial.print(trackedElements);
    Serial.print(" cont Avg: ");
    Serial.println( co2_avg_tmp / trackedElements );

  }else // new hour -> calculate average
  {
    circBuf_avg.average[circBuf_avg.write_idx_avg_buf].hour = track_Hours;
    circBuf_avg.min[circBuf_avg.write_idx_avg_buf].hour = track_Hours;
    circBuf_avg.max[circBuf_avg.write_idx_avg_buf].hour = track_Hours;

    circBuf_avg.average[circBuf_avg.write_idx_avg_buf].co = co_avg_tmp / trackedElements;
    circBuf_avg.average[circBuf_avg.write_idx_avg_buf].co2 = co2_avg_tmp / trackedElements;
    circBuf_avg.average[circBuf_avg.write_idx_avg_buf].temperature =  temp_avg_tmp / trackedElements;
    circBuf_avg.average[circBuf_avg.write_idx_avg_buf].humidity =  humi_avg_tmp / trackedElements;

    trackedElements = 0;
    circBuf_avg.write_idx_avg_buf++;
    track_Hours = currentHour;
  }
}

// --------------------------------------------------------------------------------- +++++++++++++++++++++++++++++++++++++++++++++++ ------------------------------------------------------------------------
// CO2

float co2_concentration(){
  uint8_t data[12], counter;

  // send read data command
  Wire.beginTransmission(SCD_ADDRESS);
  Wire.write(0xec);
  Wire.write(0x05);
  Wire.endTransmission();
  
  // read measurement data: 2 bytes co2, 1 byte CRC,
  // 2 bytes T, 1 byte CRC, 2 bytes RH, 1 byte CRC,
  // 2 bytes sensor status, 1 byte CRC
  // stop reading after 12 bytes (not used)
  // other data like  ASC not included
  Wire.requestFrom(SCD_ADDRESS, 12);
  counter = 0;
  while (Wire.available()) {
    data[counter++] = Wire.read();
  }
  
  // floating point conversion according to datasheet
  return (float)((uint16_t)data[0] << 8 | data[1]);
}

// --------------------------------------------------------------------------------- +++++++++++++++++++++++++++++++++++++++++++++++ ------------------------------------------------------------------------
// CO Sensor
float co_concentration()
{
	float co_conc = 0;
	int offset = 0.15; //150 mV constant offset
	
	analogReference(INTERNAL);
	
	for(int co_cnt = 1; co_cnt <= 50; co_cnt++){
		co_conc += analogRead(CO_PIN);
		delay(1);
	}
	
	co_conc = ( co_conc * 1.1 / 1023 ) / 50;
	
	analogReference(DEFAULT);
	
	return co_conc;
}

// --------------------------------------------------------------------------------- +++++++++++++++++++++++++++++++++++++++++++++++ ------------------------------------------------------------------------
// Temperature + humidity sensor

void room_temperature_humidity()
{    
  if(!dht.readTempAndHumidity(temp_hum_val)){
    return;
  }
  else{
    printError("Failed to get temperature / humidity value.");
  }
}

// --------------------------------------------------------------------------------- +++++++++++++++++++++++++++++++++++++++++++++++ ------------------------------------------------------------------------
// battery voltage

float bat_Voltage(){
	float batLev = 0;
	
	for(int bat_cnt = 1; bat_cnt <= 50; bat_cnt++){
		batLev += analogRead(BAT_VOLTAGE_PIN);
		delay(1);
	}
	
	batLev = (batLev * 5.0 / 1023) / 50;

  if(batLev < 3.2){
    // charge the device -> red
  }
  else if(batLev >= 3.2 && batLev < 3.4){
    // charge status okay -> signal yellow
  }else if(batLev >= 3.4 && batLev < 4){
    // everything allright have fun -> green
  }else if(batLev > 4){
    // charge status more than perfect -> blink green
  }
	
	return batLev;
}

// --------------------------------------------------------------------------------- +++++++++++++++++++++++++++++++++++++++++++++++ ------------------------------------------------------------------------
// Display


// --------------------------------------------------------------------------------- +++++++++++++++++++++++++++++++++++++++++++++++ ------------------------------------------------------------------------
// private

// --------------------------------------------------------------------------------- +++++++++++++++++++++++++++++++++++++++++++++++ ------------------------------------------------------------------------
// error handling

void printError(char * error_msg){
	
	// raise error and print on display
  //Serial.println(F("errorOccurred"));
}

int decToBcd(int value)
{
  return ((value/10*16) + (value%10));
}

int bcdToDec(int value)
{
  return ((value/16*10) + (value%16));
}
