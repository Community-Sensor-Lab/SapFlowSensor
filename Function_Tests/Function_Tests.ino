#include <Adafruit_ADS1X15.h>
#include <RTClib.h>
#include <Wire.h>
#include "SPI.h"
#include <SD.h>
#include "stdlib.h"


#define R2 10000.00
#define R1 100000.00
#define MAX_ADC 19999
#define MAX_ADCV 2.048
#define SERIESRESISTOR 10000.000
#define SD_CHIP_SELECT 4

Adafruit_ADS1115 ads1, ads2;
RTC_DS3231 rtc_3231;

float vin;
float tempC1, tempC2, tempC3, tempC4, tempC5, tempC6, tempC7, tempC8;

void setup() {
  Serial.begin(9600);
  delay(7000);
  Serial.println(__FILE__);
  Wire.begin();

  if (!rtc_3231.begin()) {
    Serial.println("Couldn't find RTC!");
    while (1);
  }

  char datetime[32] = "YYYY/MM/DD hh:mm:ss";
  rtc_3231.now().toString(datetime);
  Serial.print("current RTC date-time: ");
  Serial.println(datetime);

  if (rtc_3231.lostPower()) {
    DateTime dt = inputDateTime();
    while (!dt.isValid()) {
      Serial.println(" RTC lost power. Enter valid date time");
      dt = inputDateTime();
    }
    rtc_3231.adjust(dt);
  }

  if (!SD.begin(SD_CHIP_SELECT)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1);
  }
  Serial.println("card initialized.");

  ads1.setGain(GAIN_TWO);  // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  ads2.setGain(GAIN_TWO);  // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV

  if (!ads1.begin(0x48)) {
    Serial.println("Failed to initialize ADS1.");
    while (1)
      ;
  }

  if (!ads2.begin(0x49)) {
    Serial.println("Failed to initialize ADS2.");
    while (1)
      ;
  }

  Serial.println("DONE");
}

void loop() {
  readThermistor();
  vin = measureVoltage();

  // Serial.printf("t1: %s, t2: %s, t3: %s, t4: %s \n",tempC1,tempC2,tempC3,tempC4);
  // Serial.printf("t5: %s, t6: %s, t7: %s, t8: %s \n",tempC5,tempC6,tempC7,tempC8);
  // Serial.printf("vin: %s, %c\n\n",vin,'b');
  writeSD();

  delay(1000);
}
