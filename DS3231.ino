#include <Wire.h>
#include "LowPower.h"

#define RTC_USE_SERIAL true
#define RTC_INIT_SETUP true

#define RTC_I2C_ADDRESS      0x68
#define RTC_SECONDS_REGISTER 0x00
#define RTC_MINUTES_REGISTER 0x01
#define RTC_HOURS_REGISTER   0x02
#define RTC_DAY_REGISTER     0x04
#define RTC_MONTH_REGISTER   0x05
#define RTC_YEAR_REGISTER    0x06
#define RTC_A2M2_REGISTER    0x0B
#define RTC_A2M3_REGISTER    0x0C
#define RTC_A2M4_REGISTER    0x0D
#define RTC_CONTROL_REGISTER 0x0E
#define RTC_STATUS_REGISTER  0x0F

#define PIN_RTC_SQW 2

uint16_t curr_year;
uint8_t curr_month, curr_day, curr_hour_24, curr_min;

void getRTCtime() {
	// set position to first register
	Wire.beginTransmission(RTC_I2C_ADDRESS);
	Wire.write(RTC_MINUTES_REGISTER);
	Wire.endTransmission(false);
	
	// request time/date bytes from registers: seconds, minutes, hours, weekday, (weekday), day, month, year
	Wire.requestFrom(RTC_I2C_ADDRESS, 6);
	
	// read the data
	curr_min = bcdToDec(Wire.read());
	curr_hour_24 = bcdToDec(Wire.read());
	curr_day = Wire.read();
	curr_day = bcdToDec(Wire.read());
	curr_month = bcdToDec(Wire.read());
	curr_year = 2000 + bcdToDec(Wire.read());
}
void setRTCtime(uint8_t time_unit, uint8_t address) {
	// sets the seconds, minutes, hours, day, month, or year in the RTC
	// use the register addresses preprocessor
	Wire.beginTransmission(RTC_I2C_ADDRESS);
	Wire.write(address);
	Wire.write(decToBcd(time_unit));
	Wire.endTransmission();
}
void isr() {
	// dummy ISR function called on wakeup
}
byte bcdToDec(byte val) { return( (val/16*10) + (val%16) ); }
byte decToBcd(byte val) { return( (val/10*16) + (val%10) ); }

void setup() {
	// initialize the Wire library for I2C
	Wire.begin();
	
	// set up the RS2321 SQW to the apprpriate PIN
	pinMode(PIN_RTC_SQW, INPUT);
	digitalWrite(PIN_RTC_SQW, HIGH);
	
	#if RTC_USE_SERIAL
		// set up the serial data rate
		Serial.begin(9600);
	#endif
	
	#if RTC_INIT_SETUP
		// these need to be set up once per chip
		// Refer to datasheet
		Wire.beginTransmission(RTC_I2C_ADDRESS);
		Wire.write(RTC_A2M2_REGISTER);
		Wire.write(0b10000000);   // A2M2
		Wire.write(0b10000000);   // A2M3
		Wire.write(0b10000000);   // A2M4
		Wire.write(0b00000110);   // Control
		Wire.write(0b00000000);   // Status
		Wire.endTransmission();
		
		// set the RTC clock to the current time
		setRTCfromDateTime();
	#endif
	
	#if RTC_USE_SERIAL
		// print out state of registers
		Wire.beginTransmission(RTC_I2C_ADDRESS);
		Wire.write(RTC_A2M2_REGISTER);
		Wire.endTransmission(false);
		Wire.requestFrom(RTC_I2C_ADDRESS, 5);
		byte val = Wire.read();
		Serial.print("A2M2 - ");
		for (int i = 7; i >= 0; i--) { bool b = bitRead(val, i); Serial.print(b); }
		Serial.println();
		val = Wire.read();
		Serial.print("A2M3 - ");
		for (int i = 7; i >= 0; i--) { bool b = bitRead(val, i); Serial.print(b); }
		Serial.println();
		val = Wire.read();
		Serial.print("A2M4 - ");
		for (int i = 7; i >= 0; i--) { bool b = bitRead(val, i); Serial.print(b); }
		Serial.println();
		val = Wire.read();
		Serial.print("Control - ");
		for (int i = 7; i >= 0; i--) { bool b = bitRead(val, i); Serial.print(b); }
		Serial.println();
		val = Wire.read();
		Serial.print("Status - ");
		for (int i = 7; i >= 0; i--) { bool b = bitRead(val, i); Serial.print(b); }
		Serial.println();
	#endif
	
	// set the seconds to 50
	// helps while debugging so you only have to wait 10 seconds for the alarm to trigger
	setRTCtime(50, RTC_SECONDS_REGISTER);
}

static uint8_t conv2d(const char *p) {
	uint8_t v = 0;
	if ('0' <= *p && *p <= '9')
	v = *p - '0';
	return 10 * v + *++p - '0';
}
void setRTCfromDateTime() {
	curr_year = 2000 + conv2d(__DATE__ + 9);
	switch (__DATE__[0]) {
		case 'J':
			curr_month = (__DATE__[1] == 'a') ? 1 : ((__DATE__[2] == 'n') ? 6 : 7);
			break;
		case 'F':
			curr_month = 2;
			break;
		case 'A':
			curr_month = __DATE__[2] == 'r' ? 4 : 8;
			break;
		case 'M':
			curr_month = __DATE__[2] == 'r' ? 3 : 5;
			break;
		case 'S':
			curr_month = 9;
			break;
		case 'O':
			curr_month = 10;
			break;
		case 'N':
			curr_month = 11;
			break;
		case 'D':
			curr_month = 12;
			break;
	}
	curr_day = conv2d(__DATE__ + 4);
	curr_hour_24 = conv2d(__TIME__);
	curr_min = conv2d(__TIME__ + 3);
	
	setRTCtime(curr_year, RTC_YEAR_REGISTER);
	setRTCtime(curr_month, RTC_MONTH_REGISTER);
	setRTCtime(curr_day, RTC_DAY_REGISTER);
	setRTCtime(curr_hour_24, RTC_HOURS_REGISTER);
	setRTCtime(curr_min, RTC_MINUTES_REGISTER);
	setRTCtime(conv2d(__TIME__ + 6), RTC_SECONDS_REGISTER);
}


void loop() {
	// attach the square wave output from RS3231 as an interrupt
	attachInterrupt(digitalPinToInterrupt(PIN_RTC_SQW), isr, FALLING);
	
	// get the current time from the RTC
	// stores the day/time in global variables
	getRTCtime();
	
	#if RTC_USE_SERIAL
		// serial output
		Serial.print(curr_hour_24); Serial.print(":"); Serial.print(curr_min);
		Serial.print(" ("); Serial.print(curr_month); Serial.print("-"); Serial.print(curr_day); Serial.print("-"); Serial.print(curr_year); Serial.print(")");
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
	#endif
	
	// reset the alarm 2 warning
	Wire.beginTransmission(RTC_I2C_ADDRESS);
	Wire.write(RTC_STATUS_REGISTER);
	Wire.write(0b00000000);
	Wire.endTransmission();
	
	#if RTC_USE_SERIAL
		// wait for serial output to finish before powering down
		Serial.flush();
	#endif
	
	// power down the arduino
	LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
	
	// detach the interrupt pin
	detachInterrupt(digitalPinToInterrupt(PIN_RTC_SQW));
}