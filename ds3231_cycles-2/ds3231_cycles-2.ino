/* Example implementation of an alarm using DS3231

   VCC and GND of rtc_ds3231 should be connected to some power source
   SDA, SCL of rtc_ds3231 should be connected to SDA, SCL of arduinoid
   SQW should be connected to CLOCK_INTERRUPT_PIN on arduinoid
   CLOCK_INTERRUPT_PIN needs to work with interrupts
*/

#include <RTClib.h>
#include <Wire.h>
#include "ArduinoLowPower.h"
//#include <Adafruit_Sleepydog.h>
#include <SPI.h>
#include <SD.h>
#include "stdlib.h"
#include <Adafruit_ADS1X15.h>

RTC_DS3231 rtc_ds3231;
Adafruit_ADS1115 ads1;
Adafruit_ADS1115 ads2;
File myFile;

bool ledToggle = true;
float vin;
float tempC1, tempC2, tempC3, tempC4, tempC5, tempC6, tempC7, tempC8;

#define SD_CHIP_SELECT 4
#define CLOCK_INTERRUPT_PIN 5  // the pin that is connected to SQW
#define HEATER_PIN 6

// ......|______|----|_____________|...................... |____|----|_____________|
//         PREH   H       POSTH
//.......|............................T....................|....................

/// T is the period between measurement events on alarm 2.
/// Can't match secs. Only to minutes.
#define T_HRS 0
#define T_MINS 4
#define T_SECS 0
/// PREH is the measurement period before heat on
/// known a baseline in J Beslity
#define PREH_HRS 0
#define PREH_MINS 1
#define PREH_SECS 0
/// H is the period to apply heat
#define H_HRS 0
#define H_MINS 0
#define H_SECS 30
/// POTSTH is the measurement period after heat
#define POSTH_HRS 0
#define POSTH_MINS 1
#define POSTH_SECS 0
/// TS is the period between measuements. The shortest cycle
#define TS_HRS 0
#define TS_MINS 0
#define TS_SECS 2
/// SLEEP time to stop for the day
#define SLEEP_HRS 11  // 6PM
#define SLEEP_MINS 5
/// WAKE time to start the day
#define WAKE_HRS 11  // 7AM
#define WAKE_MINS 15

/// NOTE: T > PREH + H + POSTH > TS

// states (periods) in cycle
#define PREH 0
#define H 1
#define POSTH 2

#define R2 10000.00
#define R1 100000.00
#define MAX_ADC 19999
#define MAX_ADCV 2.048
#define SERIESRESISTOR 10000.000

int heatingState = PREH;
DateTime sleepTime, wakeTime;
TimeSpan ts;

void setup() {
  Serial.begin(9600);
  delay(5000);
  Serial.println(__FILE__);
  Wire.begin();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  //The RTC
  if (!rtc_ds3231.begin()) {
    Serial.println("Couldn't find RTC!");
    while (1)
      ;
  }
  //Initialize the SD Card
  if (!SD.begin(SD_CHIP_SELECT)) {
    Serial.println("Card failed or not present!");
    while (1)
      ;
  }
  Serial.println("Card Initialized");

  ads1.setGain(GAIN_TWO);
  ads2.setGain(GAIN_TWO);

  if (!ads1.begin(0x48)) {
    Serial.println("Failed to initialized ADS1.");
    while (1)
      ;
  }

  if (!ads2.begin(0x48)) {
    Serial.println("Failed to initialized ADS1.");
    while (1)
      ;
  }

  printDateTime(rtc_ds3231.now());
  Serial.println("is the current RTC date/time");

  //RTC adjust
  if (rtc_ds3231.lostPower()) {
    //if (1) {
    DateTime dt = inputDateTime();
    while (!dt.isValid()) {
      Serial.println(" RTC lost power. Enter valid date time");
      dt = inputDateTime();
    }
    rtc_ds3231.adjust(dt);
  }
  //// end rtc adjust ////

  //we don't need the 32Khz Pin, so disable it
  rtc_ds3231.disable32K();

  pinMode(CLOCK_INTERRUPT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CLOCK_INTERRUPT_PIN), heatEvent, FALLING);

  rtc_ds3231.writeSqwPinMode(DS3231_OFF);
  rtc_ds3231.disableAlarm(1);
  rtc_ds3231.clearAlarm(1);
  rtc_ds3231.disableAlarm(2);
  rtc_ds3231.clearAlarm(2);

  // set the daily night time alarm clock
  TimeSpan ts = DateTime(0, 0, 0, WAKE_HRS, WAKE_MINS, 0) - DateTime(0, 0, 0, SLEEP_HRS, SLEEP_MINS, 0);
  if (ts.totalseconds() < 0)
    ts = ts + TimeSpan(1, 0, 0, 0);

  sleepTime = DateTime(rtc_ds3231.now().year(), rtc_ds3231.now().month(), rtc_ds3231.now().day(), SLEEP_HRS, SLEEP_MINS, 0);
  if (rtc_ds3231.now() >= sleepTime)
    sleepTime = sleepTime + TimeSpan(1, 0, 0, 0);
  wakeTime = sleepTime + ts;

  Serial.print("timespan is: ");
  printTimeSpan(ts);
  Serial.println();
  Serial.print("sleeptime is: ");
  printDateTime(sleepTime);
  Serial.println();
  Serial.print("waketime is: ");
  printDateTime(wakeTime);
  Serial.println('\n');

//  LowPower.attachInterruptWakeup(CLOCK_INTERRUPT_PIN, heatEvent, CHANGE);

  // set next measurement event at T - PREH + H + POSTH
  setNextT();
}

void loop() {

  if (rtc_ds3231.alarmFired(2))
    heatEvent();
  
  toggleLed();
  delay(200);

  // for (int i = 0; i < 6; i++) {
  //   toggleLed();
  //   delay(1000);
  // }
  // LowPower.sleep();
}

void heatEvent() {

  digitalWrite(LED_BUILTIN, LOW);

  printTime(rtc_ds3231.now());
  Serial.print("entering measurements: PREH starting");
  Serial.println();

  setPREH();  // set the Pre Heat period on alarm 2

  while (true) {

    // toggleLed();
    // delay(250);

    readThermistor();
    vin = measureVoltage();
    writeSD(heatingState);

    if (rtc_ds3231.alarmFired(1)) {
      switch (heatingState) {
        case PREH:
          {  // turn heater ON
            printTime(rtc_ds3231.now());
            Serial.print("turning heater on");
            Serial.println();
            heaterOn();
            // change status to H heating
            heatingState = H;
            // set alarm go off at end of Heating time period
            setH();
            break;
          }
        case H:
          {  // turn heater OFF
            printTime(rtc_ds3231.now());
            Serial.print("turning heater off");
            Serial.println();
            heaterOFF();
            heatingState = POSTH;
            setPOSTH();
            break;
          }
        case POSTH:
          {  // done cycle so set alarm 1 to T cycle and get out
            printTime(rtc_ds3231.now());
            Serial.print("leaving measurements and going to standby");
            Serial.println();
            heatingState = PREH;
            rtc_ds3231.disableAlarm(1);
            rtc_ds3231.clearAlarm(1);
            setNextT();
            break;
          }
      }  // end switch
      if (heatingState == PREH)
        break;  // get out of the while(true)
    }           // end measurement routine

    delay(TimeSpan(0, TS_HRS, TS_MINS, TS_SECS).totalseconds() * 1000);

  }  // end while(true)
}