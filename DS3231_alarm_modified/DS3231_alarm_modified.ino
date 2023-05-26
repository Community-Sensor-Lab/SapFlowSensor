/* Example implementation of an alarm using DS3231

   VCC and GND of rtc_ds3231 should be connected to some power source
   SDA, SCL of rtc_ds3231 should be connected to SDA, SCL of arduinoid
   SQW should be connected to CLOCK_INTERRUPT_PIN on arduinoid
   CLOCK_INTERRUPT_PIN needs to work with interrupts
*/

#include <RTClib.h>
#include <Wire.h>
#include "ArduinoLowPower.h"

RTC_DS3231 rtc_ds3231;

// the pin that is connected to SQW
#define CLOCK_INTERRUPT_PIN 5

/// ALARM1 will drive the overall reading cycle
#define ALARM1_EVERY_N_HRS 0
#define ALARM1_EVERY_N_MINS 0
#define ALARM1_EVERY_N_SECS 10
/// ALARM2 will drive the heat on - off cycle
#define ALARM2_EVERY_N_HRS 0
#define ALARM2_EVERY_N_MINS 0
#define ALARM2_EVERY_N_SECS 5

bool alarm = false;

void setup() {
  Serial.begin(9600);
  delay(5000);
  Serial.println(__FILE__);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  ///// the RTC /////
  if (!rtc_ds3231.begin()) {
    Serial.println("Couldn't find RTC!");
    while (1)
      ;
  }

  char datetime[32] = "YYYY/MM/DD hh:mm:ss";
  rtc_ds3231.now().toString(datetime);
  Serial.print("current RTC date-time: ");
  Serial.println(datetime);

  if (rtc_ds3231.lostPower()) {
    DateTime dt = inputDateTime();
    while (!dt.isValid()) {
      Serial.println(" RTC lost power. Enter valid date time");
      dt = inputDateTime();
    }
    rtc_ds3231.adjust(dt);
  }

  //we don't need the 32Khz Pin, so disable it
  rtc_ds3231.disable32K();

  pinMode(CLOCK_INTERRUPT_PIN, INPUT_PULLUP);
  // //// Interrupt Function setup ////
  // attachInterrupt(digitalPinToInterrupt(CLOCK_INTERRUPT_PIN), onAlarm, FALLING);
  // LowPower.attachInterruptWakeup(CLOCK_INTERRUPT_PIN, onAlarm, FALLING);

  //rtc_ds3231.clearAlarm(1);
  rtc_ds3231.clearAlarm(2);
  rtc_ds3231.writeSqwPinMode(DS3231_OFF);
  rtc_ds3231.disableAlarm(2);

  // schedule an alarm in x time
  // mode triggers the alarm when the seconds match. See Doxygen for other options
  // Only works when the values are added here, does not get the values from the definitions
  // found in lines 17 - 19

  for (int i = 0; i < 10; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
  }

  if (!rtc_ds3231.setAlarm1(rtc_ds3231.now() + TimeSpan(0, ALARM1_EVERY_N_HRS, ALARM1_EVERY_N_MINS, ALARM1_EVERY_N_SECS), DS3231_A1_Minute))
    Serial.println("Error, alarm wasn't set!");

  // the clearAlarm will send the SQW pin to HIGH
  rtc_ds3231.clearAlarm(1);
}

void loop() {
  if (rtc_ds3231.alarmFired(1)) {

    // if (alarm) {
    alarm = false;
    doYourThing();
  }

  delay(1000);
  Serial.print('.');
}

void onAlarm() {
  alarm = true;
}

void doYourThing() {

  // if (!rtc_ds3231.setAlarm2(rtc_ds3231.now() + TimeSpan(0, 0, ALARM2_EVERY_N_MINS, ALARM2_EVERY_N_SECS), DS3231_A2_Minute))
  //   digitalWrite(LED_BUILTIN, HIGH);
  uint32_t del = micros();

  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);

  if (!rtc_ds3231.setAlarm1(rtc_ds3231.now() + TimeSpan(0, ALARM1_EVERY_N_HRS, ALARM1_EVERY_N_MINS, ALARM1_EVERY_N_SECS), DS3231_A1_Minute))
    Serial.println("Error, alarm wasn't set!");
  Serial.println("Alarm set");

  digitalWrite(LED_BUILTIN, LOW);
  rtc_ds3231.clearAlarm(1);

  del = micros() - del;
  Serial.println(del / 1000000.00, 5);
}
