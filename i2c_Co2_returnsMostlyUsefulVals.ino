#define DS1307_ADDRESS 0x68 
#define SCD_ADDRESS 0x62


#include <Wire.h>

void setup()
{
  Wire.begin();
  Serial.begin(9600);
  while (!Serial); // wait for serial port to connect. Needed for native USB
  Serial.flush();
  
  // year (00-99) / month (1-12) / monthday (1-31) / weekday (1-7) / hour (0-23) /  minute (0-59) / second (always zero)
  setDateTime(23, 5, 24, 4, 20, 17, 0);
}

void loop()
{
  delay(1000);
  printDateTime();

  delay(2000);

  float co2 = CO2Concentration();
  Serial.print("\t co2: ");
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
  Wire.write(decToBcd(byte(hour)));
  Wire.write(decToBcd(byte(month)));
  Wire.write(decToBcd(byte(year)));
  Wire.write(byte(0));
  Wire.endTransmission();
}
/*
byte readByte()
{
  while (!Serial.available()) {
    delay(10);
  }
  byte reading = 0;
  byte incomingByte = Serial.read();
  while (incomingByte != '\n') {
    if (incomingByte >= '0' && incomingByte <= '9') {
      reading = reading * 10 + (incomingByte - '0');
  }
    incomingByte = Serial.read();
  }
  //Serial.println(reading, BIN);
  Serial.flush();
  return reading;
}
*/
// Prints the current date/time set in the RTC module to the serial monitor

void printDateTime()
{
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(0x00);
  Wire.endTransmission();
  Wire.requestFrom(DS1307_ADDRESS, 7);

  byte nowSeconds = bcdToDec(Wire.read());
  byte nowMinute = bcdToDec(Wire.read());
  byte nowHour = bcdToDec(Wire.read() & 0b111111);
  byte nowWeekDay = bcdToDec(Wire.read());
  byte nowMonthDay = bcdToDec(Wire.read());
  byte nowMonth = bcdToDec(Wire.read());
  byte nowYear = bcdToDec(Wire.read());

  char data[20] = "";
  sprintf(data, "20%02d-%02d-%02d %02d:%02d:%02d", nowYear, nowMonth, nowMonthDay, nowHour, nowMinute, nowSeconds);
  Serial.print("Current datetime: ");
  Serial.print(data);
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

float CO2Concentration(){
  // get_data_ready
  Wire.beginTransmission(SCD_ADDRESS);
  Wire.write(0xe4b8);
  Wire.endTransmission();

  delay(1000);

  Wire.requestFrom(SCD_ADDRESS, 3);

  
  if(Wire.read() & 0x8000){
    Serial.print("du vogel daten noch nicht fertig");
    return 0.0;
  }

  Wire.beginTransmission(SCD_ADDRESS);
  Wire.write(0xec05);
  Wire.endTransmission();

  delay(1000);

  Wire.requestFrom(SCD_ADDRESS, 12);

  uint8_t data[12];
  uint8_t counter = 0;

  while (Wire.available()) {
    data[counter++] = Wire.read();
  }
  
  return (float)((uint16_t)data[0] << 8 | data[1]); // CO2 value
}
