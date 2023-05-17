#include <RTClib.h>
#include <Wire.h>


// Define Ports, Makros ================================================================== //

// SPI       PINS
// MOSI       11
// MISO       12
// CLOCK      13
// CS         10

RTC_DS1307 rtc;

// I2C address for CO2-Sensor
const int16_t SCD_ADDRESS = 0x62;
const int16_t RTC_ADDRESS = 0x68;

byte decToBcd(byte val){
  return ((val/16*10) + (val % 10));
}

byte bcdToDec(byte val){
  return( (val/16*10) + (val%16));
}
/*
void initRTC(){
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC, check pin layout");
    Serial.flush();
    while (1) delay(10);
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    // When time needs to be set on a new device, or after an empty battery, the
    // following line sets the RTC to the date & time this sketch was compiled
    //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  // When time needs to be re-set on a previously configured device, the
  // following line sets the RTC to the date & time this sketch was compiled
  Serial.print("Setup of RTC successful");
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}
*/
void setup(){
  Serial.begin(9600);
  Wire.begin();

  //seconds, minutes, hours, day, date, month, year
  setDS1307time(0, 59, 23, 7, 31, 12, 99);
  Serial.println("Setup routine done");
}

void loop()
{ 
  receiveTime();

  delay(1000);

  
  Wire.beginTransmission(SCD_ADDRESS);
  Wire.write(0x21);
  Wire.write(0xb1);
  Wire.endTransmission();

  Wire.requestFrom(SCD_ADDRESS, 12);
  uint8_t data[12];
  uint8_t counter = 0;
  while (Wire.available()) {
    data[counter++] = Wire.read();
  }

  float CO2 = (float)((uint16_t)data[0] << 8 | data[1]); // CO2 value

  Serial.print("\t CO2: ");
  Serial.println(CO2);
}


void setDS1307time(byte second, byte minute, byte hour, byte day, byte date, byte month, byte year){
  Wire.beginTransmission(RTC_ADDRESS);
  Wire.write(byte(0));
  Wire.write(decToBcd(second));
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour));
  Wire.write(decToBcd(day));
  Wire.write(decToBcd(date));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));
  Wire.write(byte(0));
  Wire.endTransmission();
}

void receiveTime(){
  Wire.beginTransmission(RTC_ADDRESS);
  Wire.write(0x00);
  Wire.endTransmission();
  Wire.requestFrom(RTC_ADDRESS, 7);

  byte nowSeconds = bcdToDec(Wire.read());
  byte nowMinute = bcdToDec(Wire.read());
  byte nowHour = bcdToDec(Wire.read() & 0b111111);
  byte nowWeekDay = bcdToDec(Wire.read());
  byte nowMonthDay = bcdToDec(Wire.read());
  byte nowMonth = bcdToDec(Wire.read());
  byte nowYear = bcdToDec(Wire.read());

  char data[20] = "";
  sprintf(data, "%d-%d-%d %d:%d:%d", nowYear, nowMonth, nowMonthDay, nowHour, nowMinute, nowSeconds);
  Serial.print("Current datetime: ");
  Serial.println(data);
}

/*int bcdtable(int dec){
    if(dec == 0)
        return 0000;
    else if(dec == 1)
        return 0001;
    else if(dec == 2)
        return 0010;
    else if(dec == 3)
        return 0011;
    else if(dec == 4)
        return 0100;
    else if(dec == 5)
        return 0101;
    else if(dec == 6)
        return 0110;
    else if(dec == 7)
        return 0111;
    else if(dec == 8)
        return 1000;
    else
        return 1001;
        
}
int bcdToDecimal(int num)
{
    int tens;
    int ones;
    int bcd[8];
    
    // tens
    if(num >= 10){
        tens = num / 10;
    }
    
    // ones
    ones = num % 10;
    
    printf("tens: %d\t", bcdtable(tens));
    printf("ones: %d", bcdtable(ones));
}

int main()
{
    bcdToDecimal(90);

    return 0;
}*/
