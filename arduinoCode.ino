
#include <Wire.h>
#include <AirQuality.h>

AirQuality aq(12);

void setup() {
  // check in your settings that the right speed is selected
  Serial.begin(115200);
  // wait for serial connection from PC
  // comment the following line if you'd like the output
  // without waiting for the interface being ready
  //while(!Serial);

  // wait until sensors are ready, > 1000 ms according to datasheet
  delay(1000);
  
  // start scd measurement in periodic mode, will update every 30 s
  Wire.beginTransmission(SCD_ADDRESS);
  Wire.write(0x21);
  Wire.write(0xac);
  Wire.endTransmission();

  // wait for first measurement to be finished
  delay(5000);

  Serial.println("setup done");
}

void loop() {
  aq.retrieve_RTC_time(currentTime);

  sensorVals.co = aq.co_concentration();
  sensorVals.co2 = (int) aq.co2_concentration();
  sensorVals.temperature = aq.room_temperature();
  sensorVals.humidity = aq.room_humidity();
  sensorVals.batLevel = aq.bat_Voltage();

  // convert double values to string so serial.print can handle it
  char buffer[100];

  char co_conc[9];
  dtostrf(sensorVals.co, 4, 4, co_conc);
  
  char batVoltage_string[9];
  dtostrf(sensorVals.batLevel, 4, 2, batVoltage_string);

  char temperature_string[8];
  dtostrf(sensorVals.temperature, 4, 2, temperature_string);

  char humidity_string[8];
  dtostrf(sensorVals.humidity, 4, 2, humidity_string);
  

  sprintf(buffer, "%d\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\t%s\t%s\n", currentTime[2], currentTime[1], currentTime[0], currentTime[4], currentTime[5], currentTime[6], temperature_string, humidity_string, sensorVals.co2, co_conc, batVoltage_string);
  
  //aq.add_values_to_hourly_buf(sensorVals);
  aq.addValuesToSDCard(buffer);
  Serial.println(buffer);

  delay(30000); 
}
