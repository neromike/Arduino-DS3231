#include <Wire.h>
#include "LowPower.h"

#define SET_REGISTERS true

#define RTC_I2C_ADDRESS 0x68
#define RTC_A2M2_REGISTER 0x0B
#define RTC_A2M3_REGISTER 0x0C
#define RTC_A2M4_REGISTER 0x0D
#define RTC_CONTROL_REGISTER 0x0E
#define RTC_STATUS_REGISTER 0x0F

#define RTC_SQW_PIN 2

uint8_t alarm_state;

void isr() {}

void setup()
{
  Wire.begin();

  //pinMode(RTC_SQW_PIN, INPUT_PULLUP);
  pinMode(RTC_SQW_PIN, INPUT);
  digitalWrite(RTC_SQW_PIN, HIGH);

  Serial.begin(9600);

  #if SET_REGISTERS
    Wire.beginTransmission(RTC_I2C_ADDRESS);
    Wire.write(RTC_A2M2_REGISTER);
    Wire.write(0b10000000);   // A2M2
    Wire.write(0b10000000);   // A2M3
    Wire.write(0b10000000);   // A2M4
    Wire.write(0b00000110);   // Control
    Wire.write(0b00000000);   // Status
    Wire.endTransmission();
  #endif
  
  // print out state of registers
  Wire.beginTransmission(RTC_I2C_ADDRESS);
  Wire.write(RTC_A2M2_REGISTER);
  Wire.endTransmission(false);
  Wire.requestFrom(RTC_I2C_ADDRESS, 1);
  Serial.print("A2M2 - ");
  byte val = Wire.read();
  for (int i = 7; i >= 0; i--) {
      bool b = bitRead(val, i);
      Serial.print(b);
  }
  Serial.println();

  Wire.beginTransmission(RTC_I2C_ADDRESS);
  Wire.write(RTC_A2M3_REGISTER);
  Wire.endTransmission(false);
  Wire.requestFrom(RTC_I2C_ADDRESS, 1);
  Serial.print("A2M3 - ");
  val = Wire.read();
  for (int i = 7; i >= 0; i--) {
      bool b = bitRead(val, i);
      Serial.print(b);
  }
  Serial.println();

  Wire.beginTransmission(RTC_I2C_ADDRESS);
  Wire.write(RTC_A2M4_REGISTER);
  Wire.endTransmission(false);
  Wire.requestFrom(RTC_I2C_ADDRESS, 1);
  Serial.print("A2M4 - ");
  val = Wire.read();
  for (int i = 7; i >= 0; i--) {
      bool b = bitRead(val, i);
      Serial.print(b);
  }
  Serial.println();

  Wire.beginTransmission(RTC_I2C_ADDRESS);
  Wire.write(RTC_CONTROL_REGISTER);
  Wire.endTransmission(false);
  Wire.requestFrom(RTC_I2C_ADDRESS, 1);
  Serial.print("Control - ");
  val = Wire.read();
  for (int i = 7; i >= 0; i--) {
      bool b = bitRead(val, i);
      Serial.print(b);
  }
  Serial.println();

  Wire.beginTransmission(RTC_I2C_ADDRESS);
  Wire.write(RTC_STATUS_REGISTER);
  Wire.endTransmission(false);
  Wire.requestFrom(RTC_I2C_ADDRESS, 1);
  Serial.print("Status - ");
  val = Wire.read();
  for (int i = 7; i >= 0; i--) {
      bool b = bitRead(val, i);
      Serial.print(b);
  }
  Serial.println();

  Wire.beginTransmission(RTC_I2C_ADDRESS);
  Wire.write(0x00);
  Wire.write(decToBcd(50));
  Serial.print("SET SECONDS TO 50 - ");
  Serial.println( Wire.endTransmission() );

  Serial.println();
  
}

byte bcdToDec(byte val) { return( (val/16*10) + (val%16) ); }
byte decToBcd(byte val) { return( (val/10*16) + (val%10) ); }
void setRTC_minute(byte minute) { Wire.beginTransmission(RTC_I2C_ADDRESS); Wire.write(0x01); Wire.write(decToBcd(minute)); Wire.endTransmission(); }
void setRTC_hour(byte hour) { Wire.beginTransmission(RTC_I2C_ADDRESS); Wire.write(0x02); Wire.write(decToBcd(hour)); Wire.endTransmission(); }
void setRTC_day(byte day) { Wire.beginTransmission(RTC_I2C_ADDRESS); Wire.write(0x04); Wire.write(decToBcd(day)); Wire.endTransmission(); }
void setRTC_month(byte month) { Wire.beginTransmission(RTC_I2C_ADDRESS); Wire.write(0x05); Wire.write(decToBcd(month)); Wire.endTransmission(); }
void setRTC_year(byte year) { Wire.beginTransmission(RTC_I2C_ADDRESS); Wire.write(0x06); Wire.write(decToBcd(year)); Wire.endTransmission(); }

void loop() {
  
  // attach the square wave output from RS3231 as an interrupt
  attachInterrupt(digitalPinToInterrupt(RTC_SQW_PIN), isr, FALLING);
  
  // set position to first register
  Wire.beginTransmission(RTC_I2C_ADDRESS);
  Wire.write(0x00);
  Wire.endTransmission(false);
  
  // request time/date bytes from registers: seconds, minutes, hours, weekday, (weekday), day, month, year
  Wire.requestFrom(RTC_I2C_ADDRESS, 7);
  
  // read the data
  byte seconds = bcdToDec(Wire.read());
  byte minutes = bcdToDec(Wire.read());
  byte hours = bcdToDec(Wire.read());
  byte day = Wire.read();
  day = bcdToDec(Wire.read());
  byte month = bcdToDec(Wire.read());
  byte year = bcdToDec(Wire.read());
  
  // serial output
  Serial.print(hours); Serial.print(":"); Serial.print(minutes); Serial.print(":"); Serial.print(seconds);
  Serial.print(" ("); Serial.print(month); Serial.print("-"); Serial.print(day); Serial.print("-"); Serial.print(year); Serial.print(")");
  Serial.print(" - ");
  Wire.beginTransmission(RTC_I2C_ADDRESS);
  Wire.write(RTC_STATUS_REGISTER);
  Wire.endTransmission(false);
  Wire.requestFrom(RTC_I2C_ADDRESS, 1);
  Serial.print("Status - ");
  byte val = Wire.read();
  for (int i = 7; i >= 0; i--) {
      bool b = bitRead(val, i);
      Serial.print(b);
  }
  Serial.println();

  // reset the alarm 2 warning
  Wire.beginTransmission(RTC_I2C_ADDRESS);
  Wire.write(RTC_STATUS_REGISTER);
  Wire.write(0b00000000);   // Status
  Wire.endTransmission();

  Serial.flush();
  
  // power down the arduino
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); 

  // detach the interrupt
  detachInterrupt(digitalPinToInterrupt(RTC_SQW_PIN));
  
}
