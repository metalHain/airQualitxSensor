// get sensor vals and print in serial monitor

#include <Wire.h>
#include <DHT.h>

#define SCD_ADDRESS 0x62		// co2 address
#define DHTPIN 2
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

void setup()
{
  Serial.begin(9600);
  Wire.begin();
  delay(1000);
}

void loop()
{
  float co2, co, temp, humi;

  co2 = co2_concentration();
  co = co_concentration();
  //temp = room_temperature();
  //humi = room_humidity();

  char ausgabe[300]; 

  sprintf(ausgabe, "%d\t%d", (int) co2, (int) co);

  Serial.println(ausgabe);
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
