#include <Wire.h>

// I2C address for CO2-Sensor
const int16_t SCD_ADDRESS = 0x62; 

void setup(){
  Serial.begin(9600);
  Wire.begin();
}

void loop()
{ 
    float co2 = CO2Concentration();
    Serial.print("co2: ");
    Serial.println(co2);
}

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
  
  return (float)((uint16_t)data[0] << 8 | data[1]); // CO2 value
}
