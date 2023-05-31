#include <Wire.h>

// SCD4x
const int16_t SCD_ADDRESS = 0x62;

void setup() {
  // check in your settings that the right speed is selected
  Serial.begin(9600);
  // wait for serial connection from PC
  // comment the following line if you'd like the output
  // without waiting for the interface being ready
  while(!Serial);

  // output format
  Serial.println("CO2(ppm)\tTemperature(degC)\tRelativeHumidity(percent)");
  
  // init I2C
  Wire.begin();

  // wait until sensors are ready, > 1000 ms according to datasheet
  delay(1000);
  
  // start scd measurement in periodic mode, will update every 5 s
  Wire.beginTransmission(SCD_ADDRESS);
  Wire.write(0x21ac);
  Wire.endTransmission();

  // wait for first measurement to be finished
  delay(5000);
}

void loop() {
  float co2 =  co2_concentration();
  Serial.println(co2);
  
  delay(1000);
}

float co2_concentration(){
  uint8_t data[12], counter;

  // send read data command
  Wire.beginTransmission(SCD_ADDRESS);
  Wire.write(0xec05);
  Wire.endTransmission();
  
  delay(10);

  Wire.requestFrom(SCD_ADDRESS, 12);
  counter = 0;
  
  while (Wire.available()) {
    data[counter++] = Wire.read();
  }
  
  // floating point conversion according to datasheet
  return (float)((uint16_t)data[0] << 8 | data[1]);
}
