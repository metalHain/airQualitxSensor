#include "airQuality_Lib.h"

void setup() {
  Serial.begin(9600);
  initSDcard();
  initCircBuffer_avg();

  dht.begin();  
  Wire.begin();
  Wire.setWireTimeout(3000 /* us */, true /* reset_on_timeout */);

  retrieve_RTC_time(currentTime);

  track_Hours = currentTime[2];
  circBuf_avg.trackedElements = 0;

  pinMode(13, OUTPUT); // Set pin 13 as an output
  nexInit(); // Initialize Nextion communication

  delay(1000);   // wait until sensors are ready, > 1000 ms according to datasheet
  
  // start scd measurement in periodic mode, will update every 5s
  Wire.beginTransmission(SCD_ADDRESS);
  Wire.write(0x21);
  Wire.write(0xac);
  Wire.endTransmission();
}

void loop() {
  // put your main code here, to run repeatedly:
  
  if(millis() > program_ticks + 32000){
    datalogging();
    program_ticks = millis();

    for(int cnt = 1; cnt <= 4; cnt++)
    {
      set_plot_CO(sensorVals.co);
      set_plot_CO2(sensorVals.co2);
      set_plot_HUM(sensorVals.humidity);
      set_plot_TEMP(round(sensorVals.temperature));
    }
  }
  
  updateDisplay();
}

void datalogging()
{
  char log[40] = {};

  retrieve_RTC_time(currentTime);

  // Get sensor values
  sensorVals.co = co_concentration();
  sensorVals.co2 = (int) round(co2_concentration());
  room_temperature_humidity();
  sensorVals.temperature = temp_hum_val[1];
  sensorVals.humidity = (int) round(temp_hum_val[0]);
  batLev = bat_Level();

  sprintf(log, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d.%d\t%d\t%d.%d\0", currentTime[2], currentTime[1], currentTime[0], currentTime[4], currentTime[5], currentTime[6], sensorVals.co, sensorVals.co2, (int) sensorVals.temperature, (int)(100 * (sensorVals.temperature - (int)(sensorVals.temperature))), sensorVals.humidity, (int) batLev, (int) (100 * (batLev - (int) batLev)));

  addValuesToSDCard(log);
  add_values_to_AVG_buf(currentTime[2], sensorVals);
}

void updateDisplay()
{
  set_battery_indicator();
  set_time();
  set_mv_cv();
  set_mv_av();
  //set_mv_mm();
}
