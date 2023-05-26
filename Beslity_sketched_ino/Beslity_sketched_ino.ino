#include <Wire.h>
#include "uRTCLib.h"              //Library to run the RTC alarm -- there are other libraries available but this seemed most straightforward for just setting the alarm

#define URTCLIB_MODEL_DS3231 2        //RTC model

uRTCLib rtc(0x68);                    //RTC I2C address

void setup() {

  pinMode(0, OUTPUT);
  Serial.begin(9600);
  Wire.begin();

  //////////////////////////////     Set up DS3231 clock     /////////////////////////////////////////
  rtc.set_model(URTCLIB_MODEL_DS3231);
  rtc.set_rtc_address(0x68);
  rtc.alarmSet(URTCLIB_ALARM_TYPE_1_FIXED_MS, 0, 0, 0, 1); //Alarm for every hour on the hour
  rtc.alarmSet(URTCLIB_ALARM_TYPE_2_FIXED_M, 0, 30, 0, 1); //Alarm for every hour at the 30 minute mark

  //rtc.alarmSet(URTCLIB_ALARM_TYPE_1_FIXED_S, 30, 0, 0, 1); // this sets the alarm to turn the system on and run code every minute on the minute
  //rtc.alarmSet(URTCLIB_ALARM_TYPE_2_ALL_M, 0, 0, 0, 1);
  // other alarm types are  URTCLIB_ALARM_TYPE_1_ALL_S - Every second
  //URTCLIB_ALARM_TYPE_1_FIXED_S - Every minute at given second
  //URTCLIB_ALARM_TYPE_1_FIXED_MS - Every hour at given Minute:Second
  //URTCLIB_ALARM_TYPE_1_FIXED_HMS - Every day at given Hour:Minute:Second
  //URTCLIB_ALARM_TYPE_1_FIXED_DHMS - Every month at given DAY-Hour:Minute:Second
  //URTCLIB_ALARM_TYPE_1_FIXED_WHMS - Every week at given DOW + Hour:Minute:Second
  //URTCLIB_ALARM_TYPE_2_ALL_M - Every minute at 00 Seconds
  //URTCLIB_ALARM_TYPE_2_FIXED_M - Every hour at given Minute(:00)
  //URTCLIB_ALARM_TYPE_2_FIXED_HM - Every day at given Hour:Minute(:00)
  //URTCLIB_ALARM_TYPE_2_FIXED_DHM - Every month at given DAY-Hour:Minute(:00)
  //URTCLIB_ALARM_TYPE_2_FIXED_WHM - Every week at given DOW + Hour:Minute(:00)
  // for _FIXED_ alarms, the integers set the time of the alarm -- second, minute, hour, dayofweek
  //so for an alarm every hour at :30, (URTCLIB_ALARM_TYPE_1_FIXED_MS, 0, 30, 0, 1)
  // the library uses 1 as default for dayofweek so I haven't changed that but it doesn't affect most alarms
  // should be possible to alternate alarms 1 and 2 for more intervals but I haven't gotten that working

  //////////////////////////////     ADS1115 analog to digital converter     /////////////////////////////////////////

  //////////////////////////////         Skip undersired hours     /////////////////////////////////////////
  // The following code chunk can shut the system off at hours we don't want to sample. Set time for each reading
  rtc.refresh(); //update time from the rtc
  int mi  = rtc.minute();
  int y  = rtc.year();
  int mo = rtc.month();
  int d  = rtc.day();
  int hr  = rtc.hour();
  int m  = rtc.minute();
  float s  = rtc.second();

  //////////////////////////////     SET DESIRED HEATING and MEASURING LENGTHS     /////////////////////////////////////////
  int cycle = 100 * reps;      //Measurement length (s)
  int heattime = 2 * reps;     //Heating length (s)
  int basetime = 5 * reps;     //Baseline measurement (s)

  //////////////////////////////     Loop for baseline temp data     /////////////////////////////////////////
  // Turn Heater off
  while (basetime > 0) {
    
    //measure voltage using resitor bridge Serial.println(measureVoltage());
    
    //Read Thermistors convert resistance to temperature in C
    // write sensor data to file
    basetime = basetime - 1;
  }

  //////////////////////////////     Loop to turn on heater for set amount of time     /////////////////////////////////////////
  // Turn heater on
  while (heattime > 0) {
    //Read Thermistors convert resistance to temperature in C
    // write sensor data to file
    heattime = heattime - 1;
  }

  //////////////////////////////     Loop to mointor temperature probes for set amount of time     /////////////////////////////////////////
  // Turn Heater off
  while (cycle > 0) {
    // Read Thermistors convert resistance to temperature in C
    // write sensor data to file
    cycle = cycle - 1;
  }

  //////////////////////////////     Reset clock flag to turn power off     /////////////////////////////////////////
  if (mi == 0) {
    rtc.alarmClearFlag(URTCLIB_ALARM_1);
  }
  else if (mi == 30) {
    rtc.alarmClearFlag(URTCLIB_ALARM_2);
  } else {
    rtc.alarmClearFlag(URTCLIB_ALARM_1);
    rtc.alarmClearFlag(URTCLIB_ALARM_2);
  }

  //////////////////////////////     Error warning if the alarm doesn't work to turn off     /////////////////////////////////////////
}

void loop() {

}
