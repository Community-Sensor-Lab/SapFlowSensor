/* SAP FLOW SENSOR */
#include <RTClib.h>
#include <SD.h>
#include <Adafruit_ADS1X15.h>
/// T is the period between measurement events
#define T_HRS 0
#define T_MINS 3
#define T_SECS 0
/// PREH is the measurement period before heat on
#define PREH_HRS 0
#define PREH_MINS 0
#define PREH_SECS 20
/// H is the period to apply heat
#define H_HRS 0
#define H_MINS 0
#define H_SECS 10
/// POTSTH is the measurement period after heat
#define POSTH_HRS 0
#define POSTH_MINS 0
#define POSTH_SECS 30
/// TS is the read data period between measuements. The shortest cycle
#define TS_HRS 0
#define TS_MINS 0
#define TS_SECS 2
/// SLEEP time to stop for the day
#define SLEEP_HRS 20
#define SLEEP_MINS 10
/// WAKE time to start the day
#define WAKE_HRS 6
#define WAKE_MINS 30
// ......|______|----|_____________|...................... |____|----|_____________|
//         PREH   H       POSTH
//.......|............................T....................|....................
/// note: T > PREH + H + POSTH > TS
#define DEVICE_NAME "SFS02"
// states (periods) in cycle
#define PREH 0
#define H 1
#define POSTH 2
// ADC parameters
#define R2 10000.00
#define R1 100000.00
#define MAX_ADC 19999 // with a 10k ohm calibration resisto this should be 199720
#define MAX_ADCV 2.048
#define SERIESRESISTOR 10000.000
// 
#define SD_CHIP_SELECT 4
#define HEATER_PIN 6

RTC_DS3231 rtc_ds3231;
Adafruit_ADS1115 ads1, ads2;
File myFile;
bool ledToggle = true;
float vin, tempC1, tempC2, tempC3, tempC4, tempC5, tempC6, tempC7, tempC8;
int16_t ADCout1, ADCout2, ADCout3, ADCout4, ADCout5, ADCout6, ADCout7, ADCout8; 
int heatingState = PREH;
int readDataPeriod;
uint32_t acc = 0;
DateTime dt, sleepTime, wakeTime;
TimeSpan ts;

/*
*/
void setup() {
  Serial.begin(9600);
  delay(3000);
  Serial.println(__FILE__);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  initializeSD_ADC(); // this first for debugging info

  if (!rtc_ds3231.begin()) {
    Serial.println("Couldn't find RTC!");
    writeTextSD("Couldn't find RTC!");
    while (1) {};
  }
  else {
    Serial.println("RTC set");
    writeTextSD("\n");
    writeTextSD(" RTC set");
  }

  // uncomment to adjust time and immediately reupload with comment
  // rtc_ds3231.adjust(DateTime(F(__DATE__), F(__TIME__)) + TimeSpan(0,0,0,17));

  rtc_ds3231.disable32K();
  rtc_ds3231.writeSqwPinMode(DS3231_OFF);

  fireAlarm2(); // check to see if we are here because of button press

  readDataPeriod = TimeSpan(0, TS_HRS, TS_MINS, TS_SECS).totalseconds() * 1000;

  printDateTime(rtc_ds3231.now());
  Serial.println(" current RTC date/time");
  //RTC adjust
  if (rtc_ds3231.lostPower()) {
    dt = inputDateTime();
    if (dt.isValid())
      rtc_ds3231.adjust(dt);
  }

  writeHeaderSD();
  sleepOrMeasure();
}
/*

*/
void loop() {
  if (rtc_ds3231.alarmFired(1)) {
    Serial.println("alarm 1 fired in main loop");
    fireAlarm2();
    writeHeaderSD();
    sleepOrMeasure();
  }

  toggleLedDelay(1000);
  printDateTime(rtc_ds3231.now());
  Serial.println(" current time");
  printDateTime(rtc_ds3231.getAlarm1());
  Serial.println(" alarm 1 time");
}
/*

*/
void sleepOrMeasure() {
  // decide if time to sleep or work
  dt = rtc_ds3231.now();
  sleepTime = DateTime(dt.year(), dt.month(), dt.day(), SLEEP_HRS, SLEEP_MINS, 0);
  wakeTime = DateTime(dt.year(), dt.month(), dt.day(), WAKE_HRS, WAKE_MINS, 0);
  ts = wakeTime - sleepTime;

  printDateTime(sleepTime);
  Serial.println("sleep time");
  printDateTime(wakeTime);
  Serial.println("wake time");
  printTimeSpan(ts);
  Serial.println("time span");
  Serial.print("time span seconds: ");
  Serial.println(ts.totalseconds());
  printDateTime(dt);
  Serial.println("now");

  if (ts.totalseconds() >= 0) {                  // sleep during the day: sleep < wake
    if ((dt >= sleepTime) && (dt < wakeTime)) {  // somehow woke up during sleep time
      Serial.println("1) S<=W: WOKE DURING SLEEPTIME");
      writeTextSD("1) S<=W: WOKE DURING SLEEPTIME");
      putToSleep();
    } 
    else {  // woke up during work time
      Serial.println("2) S<=W: WOKE DURING WORK TIME");
      writeTextSD("2) S<=W: WOKE DURING WORK TIME");
      measureSetNextT();
      rtc_ds3231.clearAlarm(2);  // POWER OFF
    }
  } 
  else {  // sleep over night
    if ((dt >= wakeTime) && (dt < sleepTime)) {
      Serial.println("3) W<S WOKE DURING WORK TIME");
      writeTextSD("3) W<S WOKE DURING WORK TIME");
      measureSetNextT();
      rtc_ds3231.clearAlarm(2);   // POWER OFF
    } 
    else if (dt > sleepTime) {  // woke past bedtime at night
      Serial.println("4) W<S WOKE DURING SLEEP TIME (past bedtime)");
      writeTextSD("4) W<S WOKE DURING SLEEP TIME (past bedtime)");
      writeTextSD("current waketime ");
      char cdt[32] = "YY/MM/DD hh:mm:ss";
      wakeTime.toString(cdt);
      writeTextSD(String(cdt));
      wakeTime = wakeTime + TimeSpan(1, 0, 0, 0);  //next day
      strcpy(cdt, "YY/MM/DD hh:mm:ss");
      writeTextSD("adding a day waketime ");
      wakeTime.toString(cdt);
      writeTextSD(String(cdt));
      putToSleep();
    } 
    else {  // woke  too early in the morning
      Serial.println("4) W<S WOKE DURING SLEEP TIME (before waketime)");
      writeTextSD("4) W<S WOKE DURING SLEEP TIME (before waketime)");
      putToSleep();
    }
  }
}
/*
*/
void putToSleep() {

  Serial.println("Going to sleep. Wake at ");
  printDateTime(wakeTime);
  Serial.println();
  
  writeTextSD("Going to sleep. Wake at ");
  char cdt[32] = "YY/MM/DD hh:mm:ss";
  wakeTime.toString(cdt);
  writeTextSD(String(cdt));

  if (!rtc_ds3231.setAlarm2(wakeTime, DS3231_A2_Date)) {
    Serial.println("Error, A2 wasn't set!");
    writeTextSD("putToSleep: Error, A2 wasn't set!");
  }

  writeTextSD("putToSleep: A2 value before sleep");
  strcpy(cdt,"YY/MM/DD hh:mm:ss");
  rtc_ds3231.getAlarm2().toString(cdt);
  writeTextSD(String(cdt));

  rtc_ds3231.disableAlarm(1); // incase it fires ?
  rtc_ds3231.clearAlarm(2);  // POWER OFF
  rtc_ds3231.clearAlarm(1);  // just in case still on ?
}
/*

*/
void measureSetNextT() {
  printDateTime(rtc_ds3231.now());
  Serial.print("measureSetNext: entering measurements: PREH starting\n");
  writeTextSD("measureSetNext: entering measurements: PREH starting");

  heatingState = PREH;
  setPREH();

  while (true) {
    readThermistor();
    vin = measureVoltage();
    writeSD(heatingState);

    if (rtc_ds3231.alarmFired(1)) {
      switch (heatingState) {
        case PREH:
          {  // turn heater ON
            printDateTime(rtc_ds3231.now());
            Serial.print("turning heater on \n");
            heaterOn();
            heatingState = H;
            setH();
            break;
          }
        case H:
          {  // turn heater OFF
            printDateTime(rtc_ds3231.now());
            Serial.print("turning heater off");
            Serial.println();
            heaterOFF();
            heatingState = POSTH;
            setPOSTH();
            break;
          }
        case POSTH:
          {  // done cycle so set alarm 1 to T cycle and get out
            printDateTime(rtc_ds3231.now());
            Serial.print("leaving measurements and going to standby");
            Serial.println();
            heatingState = PREH;
            break;
          }
      }  // end switch
      if (heatingState == PREH)
        break;  // get out of the while(true)
    }           // end if A1 fired
    //    delay(readDataPeriod);
    toggleLedDelay(readDataPeriod);
  }  // end while(true)

  ts = TimeSpan(0, T_HRS, T_MINS, T_SECS) - TimeSpan(0, PREH_HRS, PREH_MINS, PREH_SECS)  //
       - TimeSpan(0, H_HRS, H_MINS, H_SECS) - TimeSpan(0, POSTH_HRS, POSTH_MINS, POSTH_SECS);
  dt = rtc_ds3231.now() + ts;

  printDateTime(rtc_ds3231.now());
  Serial.print("T will start at ");
  printTime(dt);
  Serial.println();

  rtc_ds3231.disableAlarm(1);
  rtc_ds3231.clearAlarm(1);
  digitalWrite(LED_BUILTIN, LOW);

  if (!rtc_ds3231.setAlarm2(dt, DS3231_A2_Day))
    Serial.println("Error, A2 T wasn't set!");
}
/*
*/
void fireAlarm2() {
  DateTime dt = DateTime(0, 0, 0, 0, 0, 0);
  rtc_ds3231.setAlarm2(dt, DS3231_A2_Minute);
  uint32_t strt = micros();
  DateTime nw = rtc_ds3231.now();
  rtc_ds3231.adjust(dt); // set to 0000 to fire
  rtc_ds3231.adjust(nw); // restore now time
  acc = acc + (micros()-strt); // us delay accumulator
  Serial.printf("acc (us): %d\n",acc);
  if (acc>610000) { // adjust this number to tune
    rtc_ds3231.adjust(rtc_ds3231.now()+TimeSpan(1)); // advance 1s
    acc =0; 
  }
}
