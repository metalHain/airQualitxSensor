#include "DHT.h"
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Nextion.h>

#define DS1307_ADDRESS 0x68 	// rtc address
#define SCD_ADDRESS 0x62		// co2 address
#define CS    7
#define MAX_AVG_BUF_SIZE 4

#define BAT_VOLTAGE_PIN A1
#define CO_PIN A0

DHT dht(2, DHT22);

File sdFile;

byte track_Hours; // current hour for circular buffer

float co_avg_tmp, co2_avg_tmp, temp_avg_tmp, humi_avg_tmp;
float temp_hum_val[2] = {0};    
static unsigned long int program_ticks = millis();
float batLev;
float ref_BatLev = 0;

// function prototypes
void set_RTC_time(byte year, byte month, byte monthday, byte weekday, byte hour, byte minute, byte second);
void retrieve_RTC_time(int nowTime[7]);
void initSDcard();
void addValuesToSDCard(char sdbuffer[40]);
void initCircBuffer_avg();
void add_values_to_AVG_buf(int currentHour, struct sensedData values);
float co2_concentration();
float co_concentration();
void room_temperature_humidity();
int bat_Level();
int decToBcd(int value);
int bcdToDec(int value);

// basic data structure
struct sensedData{
  byte hour;
  byte minute;
  byte second;
  byte day;
  byte month;
  byte year;
  int co;				      // ppm
  int co2;				    // ppm
  float temperature;	// degrees celcius
  int humidity;		    // percent
};

struct sensedDataMini{
  byte hour;
  int co;				      // ppm
  int co2;				    // ppm
  float temperature;	// degrees celcius
  int humidity;		    // percent
};

static int currentTime[7];

typedef struct{
  struct sensedDataMini average[MAX_AVG_BUF_SIZE];
  struct sensedDataMini min;
  struct sensedDataMini max;
  byte write_idx_avg_buf;
  int trackedElements;
} CircularBuffer_avg;

static CircularBuffer_avg circBuf_avg;

static struct sensedData sensorVals;

// Display stuff

NexProgressBar batteryBar = NexProgressBar(1,5,"a");

NexText clock = NexText(1,6,"b");

NexNumber t_mv_cv_CO = NexNumber(2,7,"c");
NexNumber t_mv_cv_CO2 = NexNumber(2,9,"d");
NexNumber t_mv_cv_hum = NexNumber(2,8,"e");
NexNumber t_mv_cv_temp = NexNumber(2,10,"f");


NexNumber t_mv_av_CO = NexNumber(1,7,"g");
NexNumber t_mv_av_CO2 =NexNumber(1,9,"h");
NexNumber t_mv_av_hum =NexNumber(1,8,"i");
NexNumber t_mv_av_temp =NexNumber(1,10,"j");

/*
NexNumber t_mv_mm_CO_min  = NexNumber(0,7,"k");
NexNumber t_mv_mm_CO2_min =NexNumber(0,9,"l");
NexNumber t_mv_mm_hum_min =NexNumber(0,8,"m");
NexNumber t_mv_mm_temp_min =NexNumber(0,10,"n");

NexNumber t_mv_mm_CO_max  = NexNumber(0,7,"o");
NexNumber t_mv_mm_CO2_max =NexNumber(0,9,"p");
NexNumber t_mv_mm_hum_max =NexNumber(0,8,"q");
NexNumber t_mv_mm_temp_max =NexNumber(0,10,"r");
*/
NexWaveform graph_CO = NexWaveform(8,4,"s");
NexWaveform graph_CO2 = NexWaveform(9,10,"t");
NexWaveform graph_TEMP = NexWaveform(10,9,"u");
NexWaveform graph_HUM = NexWaveform(11,11,"v");


float h = 31;  
float t = 31;


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

  Wire.endTransmission();
}

void retrieve_RTC_time(int nowTime[7])
{
  Wire.clearWireTimeoutFlag();

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

// --------------------------------------------------------------------------------- +++++++++++++++++++++++++++++++++++++++++++++++ ------------------------------------------------------------------------
// SD

void initSDcard(){
  SD.begin(7);
}


void addValuesToSDCard(char sdbuffer[40]){ // write line of data to sd card file data.txt
  sdFile = SD.open("datLog.txt", FILE_WRITE);

  // if the file opened okay, write to it:
  if (sdFile) {
    sdFile.println(sdbuffer);
    sdFile.close();
  }
}

// --------------------------------------------------------------------------------- +++++++++++++++++++++++++++++++++++++++++++++++ ------------------------------------------------------------------------
// circular buffer

// --------------------------------------------------------------------------------- +++++++++++++++++++++++++++++++++++++++++++++++ ------------------------------------------------------------------------
// init

void initCircBuffer_avg(){
  circBuf_avg.write_idx_avg_buf = 0;
  
  for(int i=0; i<7; i++){
    circBuf_avg.min.co = -1;
    circBuf_avg.min.co2 = -1;
    circBuf_avg.min.temperature = -1;
    circBuf_avg.min.humidity = -1;
  }
}

// --------------------------------------------------------------------------------- +++++++++++++++++++++++++++++++++++++++++++++++ ------------------------------------------------------------------------
// fill with vals

void add_values_to_AVG_buf(int currentHour, struct sensedData values){

  if(circBuf_avg.trackedElements == 0) // new hour has started -> init buffer values
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

    // update min values
    if(circBuf_avg.min.co == -1){circBuf_avg.min.co = values.co;}
    if(circBuf_avg.min.co2 == -1){circBuf_avg.min.co2 = values.co2;}
    if(circBuf_avg.min.temperature == -1){circBuf_avg.min.temperature = values.temperature;}
    if(circBuf_avg.min.humidity == -1){circBuf_avg.min.humidity = values.humidity;}

    if(circBuf_avg.min.co > values.co ){circBuf_avg.min.co = values.co;}
    if(circBuf_avg.min.co2 > values.co2 ){circBuf_avg.min.co2 = values.co2;}
    if(circBuf_avg.min.temperature > values.temperature ){circBuf_avg.min.temperature = values.temperature;}
    if(circBuf_avg.min.humidity > values.humidity ){circBuf_avg.min.humidity = values.humidity;}

    // update max values
    if(circBuf_avg.max.co < values.co){circBuf_avg.max.co = values.co;}
    if(circBuf_avg.max.co2 < values.co2){circBuf_avg.max.co2 = values.co2;}
    if(circBuf_avg.max.temperature < values.temperature){circBuf_avg.max.temperature = values.temperature;}
    if(circBuf_avg.max.humidity < values.humidity){circBuf_avg.max.humidity = values.humidity;}

    circBuf_avg.trackedElements++;
  }else // new hour -> calculate average
  {
    circBuf_avg.average[circBuf_avg.write_idx_avg_buf].co = co_avg_tmp / circBuf_avg.trackedElements;
    circBuf_avg.average[circBuf_avg.write_idx_avg_buf].co2 = co2_avg_tmp / circBuf_avg.trackedElements;
    circBuf_avg.average[circBuf_avg.write_idx_avg_buf].temperature = temp_avg_tmp / circBuf_avg.trackedElements;
    circBuf_avg.average[circBuf_avg.write_idx_avg_buf].humidity = humi_avg_tmp / circBuf_avg.trackedElements;

    circBuf_avg.trackedElements = 0;
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

  delay(5000);
  
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
	
	analogReference(INTERNAL);
	
	for(int co_cnt = 1; co_cnt <= 50; co_cnt++){
		co_conc += analogRead(CO_PIN);
		delay(1);
	}
	
	co_conc = ( co_conc * 1.1 / 1023 ) / 50;
	analogReference(DEFAULT);
	
  // calculate co concentration in ppm - the offset of 0.252 V was measured over an hour with no co in room air
  co_conc = (co_conc - 0.252) * (1/0.00261); 
	
  if(co_conc < 0)
    return 0;
  else if (co_conc >= 1000)
    return 1000;
  else
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
    // error reading temperature / humidity sensor
  }
}

// --------------------------------------------------------------------------------- +++++++++++++++++++++++++++++++++++++++++++++++ ------------------------------------------------------------------------
// battery voltage

int bat_Level(){
	batLev = 0;
	
	for(int bat_cnt = 1; bat_cnt <= 50; bat_cnt++){
		batLev += analogRead(BAT_VOLTAGE_PIN);
		delay(1);
	}
	
	batLev = (batLev * 5.0 / 1023) / 50;

  if(abs(batLev - 4.2) < abs(batLev - 3.95)){
    return 100;
  }
  if(abs(batLev - 3.95) < abs(batLev - 3.7)){
    return 75;
  }
  if(abs(batLev - 3.7) < abs(batLev - 3.2)){
    return 25;
  }
  else
  {
    return 0;
  }
}

char* floatToChar(float value) {
  static char buffer[15]; // Buffer to hold the converted string
  dtostrf(value, 5, 2, buffer); // Convert the float to a string with 5 total characters and 2 decimal places
  return buffer; // Return the converted string
}

char* intToChar(int value) {
  static char buffer[10];
  // Convert the integer to a char array (string)
  // using itoa() function from the C standard library.
  itoa(value, buffer, 10); // Assuming a maximum buffer size of 10 characters.
  return buffer;
}


void set_battery_indicator(){
  int batteryLevel = bat_Level();
  // Limit the battery level within the range of 0 to maxBatteryLevel
  
  batteryBar.setValue(batteryLevel);
  // Change the color based on battery level thresholds
  if (batteryLevel > 75) {
    batteryBar.Set_font_color_pco(1024); // Green color 
  } else if (batteryLevel >= 35 && batteryLevel <= 75) {
    batteryBar.Set_font_color_pco(65504); // Yellow color 
  } else {
    batteryBar.Set_font_color_pco(63488); // Red color 
  }
}


void set_mv_cv(){
  int mv_cv_CO = sensorVals.co;
  int mv_cv_CO2 = sensorVals.co2;
  int mv_cv_hum = sensorVals.humidity;
  float mv_cv_temp = sensorVals.temperature;

  byte critical_CO = 130;
  byte optimal_CO = 0;
  int critical_CO2 = 1500;
  int optimal_CO2 = 800;
  byte critical_hum = 8;
  byte optimal_hum = 65;
  byte critical_temp = 35;
  byte optimal_temp = 20;

  if (mv_cv_CO <= optimal_CO){t_mv_cv_CO.Set_background_color_bco(1024);}
  else if (mv_cv_CO>optimal_CO && mv_cv_CO<critical_CO){t_mv_cv_CO.Set_background_color_bco(50720);}
  else {t_mv_cv_CO.Set_background_color_bco(63488);}

  if (mv_cv_CO2 <= optimal_CO2){t_mv_cv_CO2.Set_background_color_bco(1024);}
  else if (mv_cv_CO2>optimal_CO2 && mv_cv_CO2<critical_CO2){t_mv_cv_CO2.Set_background_color_bco(50720);}
  else {t_mv_cv_CO2.Set_background_color_bco(63488);}

  if (mv_cv_hum <= optimal_hum){t_mv_cv_hum.Set_background_color_bco(1024);}
  else if (mv_cv_hum>optimal_hum && mv_cv_hum<critical_hum){t_mv_cv_hum.Set_background_color_bco(50720);}
  else {t_mv_cv_hum.Set_background_color_bco(63488);}

  if (mv_cv_temp <= optimal_temp){t_mv_cv_temp.Set_background_color_bco(1024);}
  else if (mv_cv_temp>optimal_temp && mv_cv_temp<critical_temp){t_mv_cv_temp.Set_background_color_bco(50720);}
  else {t_mv_cv_temp.Set_background_color_bco(63488);}


  t_mv_cv_CO.setValue(mv_cv_CO);
  t_mv_cv_CO2.setValue(mv_cv_CO2);
  t_mv_cv_hum.setValue(mv_cv_hum);
  t_mv_cv_temp.setValue(mv_cv_temp);
}

void set_mv_av(){
  // update lese index braucht es hier immer eins weniger lesen als beschrieben wird!
  if(circBuf_avg.trackedElements > 0)
  {
    int mv_av_CO = round(co_avg_tmp / circBuf_avg.trackedElements);
    int mv_av_CO2 = round(co2_avg_tmp / circBuf_avg.trackedElements);
    int mv_av_hum = round(humi_avg_tmp / circBuf_avg.trackedElements);
    float mv_av_temp = round(temp_avg_tmp / circBuf_avg.trackedElements);

    byte critical_CO = 130;
    byte optimal_CO = 0;
    int critical_CO2 = 1500;
    int optimal_CO2 = 800;
    byte critical_hum = 8;
    byte optimal_hum = 65;
    byte critical_temp = 35;
    byte optimal_temp = 20;

    if (mv_av_CO <= optimal_CO){t_mv_av_CO.Set_background_color_bco(1024);}
    else if (mv_av_CO>optimal_CO && mv_av_CO<critical_CO){t_mv_av_CO.Set_background_color_bco(50720);}
    else {t_mv_av_CO.Set_background_color_bco(63488);}

    if (mv_av_CO2 <= optimal_CO2){t_mv_av_CO2.Set_background_color_bco(1024);}
    else if (mv_av_CO2>optimal_CO2 && mv_av_CO2<critical_CO2){t_mv_av_CO2.Set_background_color_bco(50720);}
    else {t_mv_av_CO2.Set_background_color_bco(63488);}

    if (mv_av_hum <= optimal_hum){t_mv_av_hum.Set_background_color_bco(1024);}
    else if (mv_av_hum>optimal_hum && mv_av_hum<critical_hum){t_mv_av_hum.Set_background_color_bco(50720);}
    else {t_mv_av_hum.Set_background_color_bco(63488);}
    
    if (mv_av_temp <= optimal_temp){t_mv_av_temp.Set_background_color_bco(1024);}
    else if (mv_av_temp>optimal_temp && mv_av_temp<critical_temp){t_mv_av_temp.Set_background_color_bco(50720);}
    else {t_mv_av_temp.Set_background_color_bco(63488);}

    t_mv_av_CO.setValue(mv_av_CO);
    t_mv_av_CO2.setValue(mv_av_CO2);
    t_mv_av_hum.setValue(mv_av_hum);
    t_mv_av_temp.setValue(mv_av_temp);
  }
}

/*
void set_mv_mm(){
  float mv_mm_CO_min = 5.1;
  float mv_mm_CO2_min = 6.2;
  float mv_mm_hum_min = h;
  float mv_mm_temp_min = t;
  
  byte min_critical_CO = 8;
  byte min_optimal_CO = 4;
  byte min_critical_CO2 = 8;
  byte min_optimal_CO2 = 4;
  byte min_critical_hum = 8;
  byte min_optimal_hum = 4;
  byte min_critical_temp = 8;
  byte min_optimal_temp = 4;

  if (mv_mm_CO_min <= min_optimal_CO){t_mv_mm_CO_min.Set_background_color_bco(1024);}
  else if (mv_mm_CO_min>min_optimal_CO && mv_mm_CO_min<min_critical_CO){t_mv_mm_CO_min.Set_background_color_bco(50720);}
  else {t_mv_mm_CO_min.Set_background_color_bco(63488);}

  if (mv_mm_CO2_min <= min_optimal_CO2){t_mv_mm_CO2_min.Set_background_color_bco(1024);}
  else if (mv_mm_CO2_min>min_optimal_CO2 && mv_mm_CO2_min<min_critical_CO2){t_mv_mm_CO2_min.Set_background_color_bco(50720);}
  else {t_mv_mm_CO2_min.Set_background_color_bco(63488);}

  if (mv_mm_hum_min <= min_optimal_hum){t_mv_mm_hum_min.Set_background_color_bco(1024);}
  else if (mv_mm_hum_min>min_optimal_hum && mv_mm_hum_min<min_critical_hum){t_mv_mm_hum_min.Set_background_color_bco(50720);}
  else {t_mv_mm_hum_min.Set_background_color_bco(63488);}
  
  if (mv_mm_temp_min <= min_optimal_temp){t_mv_mm_temp_min.Set_background_color_bco(1024);}
  else if (mv_mm_temp_min>min_optimal_temp && mv_mm_temp_min<min_critical_temp){t_mv_mm_temp_min.Set_background_color_bco(50720);}
  else {t_mv_mm_temp_min.Set_background_color_bco(63488);}

  float mv_mm_CO_max  =7.1;
  float mv_mm_CO2_max =8.2;
  float mv_mm_hum_max =h;
  float mv_mm_temp_max =t;

  byte max_critical_CO = 8;
  byte max_optimal_CO = 4;
  byte max_critical_CO2 = 8;
  byte max_optimal_CO2 = 4;
  byte max_critical_hum = 8;
  byte max_optimal_hum = 4;
  byte max_critical_temp = 8;
  byte max_optimal_temp = 4;

  if (mv_mm_CO_max <= max_optimal_CO){t_mv_mm_CO_max.Set_background_color_bco(1024);}
  else if (mv_mm_CO_max>max_optimal_CO && mv_mm_CO_max<max_critical_CO){t_mv_mm_CO_max.Set_background_color_bco(50720);}
  else {t_mv_mm_CO_max.Set_background_color_bco(63488);}

  if (mv_mm_CO2_max <= max_optimal_CO2){t_mv_mm_CO2_max.Set_background_color_bco(1024);}
  else if (mv_mm_CO2_max>max_optimal_CO2 && mv_mm_CO2_max<max_critical_CO2){t_mv_mm_CO2_max.Set_background_color_bco(50720);}
  else {t_mv_mm_CO2_max.Set_background_color_bco(63488);}

  if (mv_mm_hum_max <= max_optimal_hum){t_mv_mm_hum_max.Set_background_color_bco(1024);}
  else if (mv_mm_hum_max>max_optimal_hum && mv_mm_hum_max<max_critical_hum){t_mv_mm_hum_max.Set_background_color_bco(50720);}
  else {t_mv_mm_hum_max.Set_background_color_bco(63488);}
  
  if (mv_mm_temp_max <= max_optimal_temp){t_mv_mm_temp_max.Set_background_color_bco(1024);}
  else if (mv_mm_temp_max>max_optimal_temp && mv_mm_temp_max<max_critical_temp){t_mv_mm_temp_max.Set_background_color_bco(50720);}
  else {t_mv_mm_temp_max.Set_background_color_bco(63488);}

  
  t_mv_mm_CO_max.setText(floatToChar(mv_mm_CO_max));
  t_mv_mm_CO2_max.setText(floatToChar(mv_mm_CO2_max));
  t_mv_mm_hum_max.setText(floatToChar(mv_mm_hum_max));
  t_mv_mm_temp_max.setText(floatToChar(mv_mm_temp_max));

  t_mv_mm_CO_min.setText(floatToChar(mv_mm_CO_min));
  t_mv_mm_CO2_min.setText(floatToChar(mv_mm_CO2_min));
  t_mv_mm_hum_min.setText(floatToChar(mv_mm_hum_min));
  t_mv_mm_temp_min.setText(floatToChar(mv_mm_temp_min));
}
*/

int step(int sensorvalue, float stepvalue){
  return sensorvalue*stepvalue;
}

void set_plot_CO(int sensorwert){
  graph_CO.addValue(0,step(sensorwert,0.53333333));
}
void set_plot_CO2(int sensorwert){
  graph_CO2.addValue(1,step(sensorwert,0.064));
}
void set_plot_TEMP(int sensorwert){
  graph_TEMP.addValue(2, step(sensorwert,4));
}
void set_plot_HUM(int sensorwert){
  graph_HUM.addValue(3, step(sensorwert,1.6));
}

void set_time(){
  char buffer[6];

  sprintf(buffer, "%02i:%02i", currentTime[2], currentTime[1]);

  clock.setText(buffer);
}

// --------------------------------------------------------------------------------- +++++++++++++++++++++++++++++++++++++++++++++++ ------------------------------------------------------------------------
// private


int decToBcd(int value)
{
  return ((value/10*16) + (value%10));
}

int bcdToDec(int value)
{
  return ((value/16*10) + (value%16));
}
