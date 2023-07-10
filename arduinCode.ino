#include <AirQuality.h>
#include <time.h>
#include <TimeAlarms.h>

AirQuality aq(12);
AlarmId id;

void setup() { 
  Serial.begin(9600);
  //aq.set_RTC_time(23,7,9,1,18,41,0);

  delay(1000);

  aq.retrieve_RTC_time(currentTime);
  setTime(currentTime[2], currentTime[1], currentTime[0], currentTime[4], currentTime[5], currentTime[6]);

  //id = Alarm.timerRepeat(10, dataLogging);
  Serial.println("Setup done");
}

void loop() {
  dataLogging();
}

void dataLogging()
{
  aq.retrieve_RTC_time(currentTime);

  /*
  sensorVals.co = aq.co_concentration();
  sensorVals.co2 = aq.co2_concentration();
  sensorVals.temperature = aq.room_temperature();
  sensorVals.humidity = aq.room_humidity();
  */

  sensorVals.co = 1;
  sensorVals.co2 = (int) aq.co2_concentration();
  sensorVals.temperature = 1;
  sensorVals.humidity = 1;
  sensorVals.batLevel = aq.bat_Voltage();

  char buffer[200];

  // hour min sec day month year temperature humidity co2 co batVoltage
  char batVoltage_string[9];
  dtostrf(sensorVals.batLevel, 4, 3, batVoltage_string);
  
  char co_conc_temp[12];
  dtostrf(aq.co_concentration(), 4, 4, co_conc_temp);
  
  sprintf(buffer, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%s\t%s\n", currentTime[2], currentTime[1], currentTime[0], currentTime[4], currentTime[5], currentTime[6], sensorVals.temperature, sensorVals.humidity, sensorVals.co2, co_conc_temp, batVoltage_string);
  
  aq.add_values_to_hourly_buf(sensorVals);
  aq.addValuesToSDCard(buffer);
  Serial.println(buffer);

  delay(2000); 
}
