#define DS1307_ADDRESS 0x68 
#define SCD_ADDRESS 0x62

int currentTime[7];


#include <Wire.h>

void setup()
{
  Wire.begin();
  Serial.begin(9600);
  while (!Serial); // wait for serial port to connect. Needed for native USB
  Serial.flush();
  
  // year (00-99) / month (1-12) / monthday (1-31) / weekday (1-7) / hour (0-23) /  minute (0-59) / second (always zero)
  setDateTime(23, 6, 11, 1, 16, 59, 0);

  // wait until sensors are ready, > 1000 ms according to datasheet
  delay(1000);

  // wait for first measurement to be finished
  delay(5000);
}

void loop()
{
  printDateTime(currentTime);

  Serial.print(currentTime[2]);
  Serial.print(" - ");
  Serial.print(currentTime[1]);
  Serial.print(" - ");
  Serial.print(currentTime[0]);

  Serial.print(" \t ");
  Serial.print(currentTime[4]);
  Serial.print(" / ");
  Serial.print(currentTime[5]);
  Serial.print(" / ");
  Serial.print(currentTime[6]);

  float co2;

  delay(6000);

  co2 = co2_concentration();
  Serial.print("\tCO2: ");
  Serial.println(co2);
}

// Sets date/time of the RTC module via the serial monitor
// year (00-99) / month (1-12) / monthday (1-31) / weekday (1-7) / hour (0-23) /  minute (0-59) / second (always zero)

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

// Prints the current date/time set in the RTC module to the serial monitor

void printDateTime(int nowTime[7])
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

// Converts a decimal (Base-10) integer to BCD (Binary-coded decimal)
int decToBcd(int value)
{
  return ((value/10*16) + (value%10));
}

// Converts a BCD (Binary-coded decimal) to decimal (Base-10) integer
int bcdToDec(int value)
{
  return ((value/16*10) + (value%16));
}

float co2_concentration(){
  float co2, temperature, humidity;
  uint8_t data[12], counter;

  // measure single shot delay 5000
  Wire.beginTransmission(SCD_ADDRESS);
  Wire.write(0x21);
  Wire.write(0x9d);
  Wire.endTransmission();

  delay(5000);

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
  co2 = (float)((uint16_t)data[0] << 8 | data[1]);


  // wait 2 s for next measurement
  return co2;
}
