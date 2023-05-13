#include "Nextion.h"
//#include "DHT.h"  
#include <Wire.h>  
#include <RTClib.h> // RTC library
//#include <SPI.h>
//#include <SD.h>
#include <XMLWriter.h>

#define CS    7     // adjust this ChipSelect line if needed !

RTC_DS1307 rtc;

// I2C address for CO2-Sensor
const int16_t SCD_ADDRESS = 0x62; 
// const int16_t RTC_ADDRESS = 0x68;

/*            
#define DHTPIN 2          
#define DHTTYPE DHT22                           
DHT dht(DHTPIN, DHTTYPE);
*/


//NexButton example = NexButton(0, 2, "b0");  page number,comp ID, comp name;

NexText t_mv_cv_CO = NexText(2,7,"mv1");
NexText t_mv_cv_CO2 = NexText(2,9,"mv2");
NexText t_mv_cv_hum = NexText(2,8,"mv3");
NexText t_mv_cv_temp =NexText(2,10,"mv4");

NexText t_mv_av_CO = NexText(1,7,"mv5");
NexText t_mv_av_CO2 =NexText(1,9,"mv6");
NexText t_mv_av_hum =NexText(1,8,"mv7");
NexText t_mv_av_temp =NexText(1,10,"mv8");

NexText t_mv_mm_CO_min  = NexText(0,7,"mv9");
NexText t_mv_mm_CO2_min =NexText(0,9,"mv10");
NexText t_mv_mm_hum_min =NexText(0,8,"mv11");
NexText t_mv_mm_temp_min =NexText(0,10,"mv12");

NexText t_mv_mm_CO_max  = NexText(0,7,"mv13");
NexText t_mv_mm_CO2_max =NexText(0,9,"mv14");
NexText t_mv_mm_hum_max =NexText(0,8,"mv15");
NexText t_mv_mm_temp_max =NexText(0,10,"mv16");

char* floatToChar(float value) {
  static char buffer[15]; // Buffer to hold the converted string
  dtostrf(value, 5, 2, buffer); // Convert the float to a string with 5 total characters and 2 decimal places
  return buffer; // Return the converted string
}

// ON________Functions
/*
void example__PushCallback(void *ptr) {
  digitalWrite(13, HIGH); // Turn on LED on pin 13
  t0.setText(floatToChar(f)); // Set text in t0 to "On"
}*/

// OFF________Functions
/*
void example__PopCallback(void *ptr) {
  digitalWrite(13, LOW); // Turn off LED on pin 13
  t0.setText("Off"); // Set text in t0 to "Off"
}
*/

NexTouch *nex_listen_list[] = {
  NULL
};

float CO2Concentration(){
  Wire.beginTransmission(SCD_ADDRESS);
  Wire.write(0xec);
  Wire.write(0x05);
  Wire.endTransmission();
  

  Wire.requestFrom(SCD_ADDRESS, 12);
  uint8_t data[12];
  uint8_t counter = 0;
  while (Wire.available()) {
    data[counter++] = Wire.read();
  }
  Serial.println((float)((uint16_t)data[0] << 8 | data[1]));
  return (float)((uint16_t)data[0] << 8 | data[1]); // CO2 value
}

void set_values(){
  DateTime now = rtc.now();

  delay(1000);
  
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();

  //float h = dht.readHumidity();   
  //float t = dht.readTemperature(); 
  float h = 0.0;
  float t = 0.0;
  float co2 = CO2Concentration();
  float co = 0.0;

  float mv_cv_CO = co;
  float mv_cv_CO2 = co2;
  float mv_cv_hum = h;
  float mv_cv_temp = t;

  float mv_av_CO = now.hour();
  float mv_av_CO2 = now.minute();
  float mv_av_hum = now.second();
  float mv_av_temp = t;

  float mv_mm_CO_min = co;
  float mv_mm_CO2_min = co2;
  float mv_mm_hum_min = h;
  float mv_mm_temp_min = t;

  float mv_mm_CO_max  =co;
  float mv_mm_CO2_max =co2;
  float mv_mm_hum_max =h;
  float mv_mm_temp_max =t;


  t_mv_cv_CO.setText(floatToChar(mv_cv_CO));
  t_mv_cv_CO2.setText(floatToChar(mv_cv_CO2));
  t_mv_cv_hum.setText(floatToChar(mv_cv_hum));
  t_mv_cv_temp.setText(floatToChar(mv_cv_temp));
  
  t_mv_av_CO.setText(floatToChar(mv_av_CO));
  t_mv_av_CO2.setText(floatToChar(mv_av_CO2));
  t_mv_av_hum.setText(floatToChar(mv_av_hum));
  t_mv_av_temp.setText(floatToChar(mv_av_temp));
  
  t_mv_mm_CO_min.setText(floatToChar(mv_mm_CO_min));
  t_mv_mm_CO2_min.setText(floatToChar(mv_mm_CO2_min));
  t_mv_mm_hum_min.setText(floatToChar(mv_mm_hum_min));
  t_mv_mm_temp_min.setText(floatToChar(mv_mm_temp_min));
  
  t_mv_mm_CO_max.setText(floatToChar(mv_mm_CO_max));
  t_mv_mm_CO2_max.setText(floatToChar(mv_mm_CO2_max));
  t_mv_mm_hum_max.setText(floatToChar(mv_mm_hum_max));
  t_mv_mm_temp_max.setText(floatToChar(mv_mm_temp_max));
}



void setup() {
  Serial.begin(9600);
  pinMode(13, OUTPUT); // Set pin 13 as an output
  //example.attachPush(example__PushCallback); // Attach b0PushCallback to button press event
  //example.attachPop(example__PopCallback); // Attach b0PopCallback to button release event
  nexInit(); // Initialize Nextion communication
  //dht.begin();

  rtc.begin(); // Initialize the RTC module
  // Set the date and time of the RTC module
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  delay(1000);

  Wire.beginTransmission(SCD_ADDRESS);
  Wire.write(0x21);
  Wire.write(0xb1);
  Wire.endTransmission();
}

void loop() {
  delay(2000);
  set_values();
  nexLoop(nex_listen_list); // Check for incoming Nextion commands
}