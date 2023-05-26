

/*  JS5 interface code
 *  
 *  Sap Flow HPV code
 *  SD Card Variant
 * 
 *  Justin Beslity (jobeslit@syr.edu)
 *  SUNY - ESF, Graduate Program of Environmental Science
 * 
 *  The objective of this code is to:
 *  a) Program RTC alarms
 *  b) Set baseline, heat pulse and cycle length (BE SURE TO CHANGE THESE IF NECESSARY)
 *  c) OPTIONAL: Skip measurements
 *  d) Collect baseline measurements
 *  e) Apply heat pusle
 *  f) Collect temp measurements from each of the thermistors
 *  g) Clear alarms opening MOSFET circuit and power down ESP8266
 *  
 *  Recommendations:
 *  For the HR and Tmax methods, the heat pusle should be monitored (at the absolute minimum)60 seconds and ideally 100 seconds.
 *  For the CHP method, the heat pulse should be monitored for 100 seconds during the day and longer during the night.
 *  
 *  To save battery power, nighttime measurements should be limited.
 *  
 */

//////////////////////////////     Load libraries     /////////////////////////////////////////////////

#include <Wire.h>

#include <Adafruit_ADS1X15.h>     //Adafruit ADS1115 library-this controls the analog to digital converter      
// sets I2C address for the ADC    
Adafruit_ADS1115 ads1115a;
Adafruit_ADS1115 ads1115b;//one of 4 addresses

#include "SPI.h"                          //SD Card Libraries
#include <SD.h>
File myFile;
const int chipSelect = 15;


#include "uRTCLib.h"              //Library to run the RTC alarm -- there are other libraries available but this seemed most straightforward for just setting the alarm
#define URTCLIB_MODEL_DS3231 2        //RTC model
uRTCLib rtc(0x68);                    //RTC I2C address

#include <Adafruit_INA219.h>     //For Current Meter if desired 
Adafruit_INA219 ina219;


//////////////////////////////     Setup     ////////////////////////////////////////////////////////

// The setup() code below should run each time the sensor wakes up

void setup() {
  pinMode(0, OUTPUT);
  Serial.begin(9600);
  Wire.begin();
  digitalWrite( SDA, LOW);      //This may not be necessary but could recover I2C if shut down incorrectly
  digitalWrite( SCL, LOW);
  SD.begin(15);                 //Set SD Card CS pin number and begin function

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

 //Each ADS1115 can operate 4 thermistors and up to 4 ADS1115's can be operated by a single microcontroller
 //see the datasheet for further instructions
 
    ads1115a.begin(0x48);    
    ads1115a.setGain(GAIN_ONE); //set gain on the ADC
    ads1115b.begin(0x49);    
    ads1115b.setGain(GAIN_ONE); //set gain on the ADC
    pinMode(LED_BUILTIN, OUTPUT);

//////////////////////////////     Set Variables for readings     /////////////////////////////////////////

  //Thermistor variables
  float reading1;
  float reading2;
  float reading3;
  float reading4;
  float reading5;
  float reading6;
  float reading7;
  float reading8;   
  float tempC1;
  float tempC2;
  float tempC3;
  float tempC4;
  float tempC5;
  float tempC6;
  float tempC7;
  float tempC8;
  int16_t ADCout1;
  int16_t ADCout2;
  int16_t ADCout3;
  int16_t ADCout4;
  int16_t ADCout5;
  int16_t ADCout6;
  int16_t ADCout7;
  int16_t ADCout8;
  float ohms1;
  float ohms2;
  float ohms3;
  float ohms4;
  float ohms5;
  float ohms6;
  float ohms7;
  float ohms8;
  float SERIESRESISTOR = 10000.000;
  float maxADC = 19434;

  //Voltage Variables
  float vout;
  float vin;
  float R1 = 100000.0; // resistance of R1 (100K)
  float R2 = 10000.0; // resistance of R2 (10K)
  int value = 0;


//////////////////////////////         Skip undersired hours     /////////////////////////////////////////

//The following code chunk can shut the system off at hours we don't want to sample.
//Set time for each reading
      rtc.refresh(); //update time from the rtc
      int mi  = rtc.minute();
      
      int y  = rtc.year();
      int mo = rtc.month();
      int d  = rtc.day();
      int hr  = rtc.hour();
      int m  = rtc.minute();
      float s  = rtc.second();


    
//////////////////////////////     Set time between temperature readings (ms)     /////////////////////////////////////////

//Just Change samplingfrequency!!!!!!!!!!!!!!!!!!! (500 for a temp reading every half second and 1000 for every second)
      int samplingfrequency = 500; //Input desired sampling frequency (ms)
      
      int reps = 1000 / samplingfrequency; //Adjusts for sampling frequency

//////////////////////////////     Adjust for computational Delay     /////////////////////////////////////////

      int compdelay = 87; //Mean computer processing delay in milliseconds
      int rep = samplingfrequency - compdelay; //measuring frequency in milliseconds minus a small correction for computer processing time
      
//////////////////////////////     SET DESIRED HEATING and MEASURING LENGTHS     /////////////////////////////////////////

//Just change the numbers!!!!!!!!!!!!!!!!!!!!!!!!!

      int cycle = 100 * reps;      //Measurement length (s)
      int heattime = 2 * reps;     //Heating length (s)
      int basetime = 5 * reps;     //Baseline measurement (s)

      

//////////////////////////////     Loop for baseline temp data     /////////////////////////////////////////

 //Baseline data before applying the heatpulse helps identify the pulse and adjust for weird measurements 
 //occuring during the pulse, likely due to the current spike.

while(basetime >0){
      
  //Turn Heater off
      digitalWrite(LED_BUILTIN, HIGH);
      digitalWrite(0,LOW);

  //measure voltage using resitor bridge
      value = analogRead(A0);
      vout = (value * 3.37046) / 1024.0; // The value of 2.97 is the calibrated Arduino ADC reference voltage (nominally 3.3V).  Worth double checking using a AC->DC converter.
      vin = vout / (R2/(R1+R2));    
 
  //Read Thermistors
  ADCout1 = ads1115a.readADC_SingleEnded(0); //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3
  ADCout2 = ads1115a.readADC_SingleEnded(1); //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3
  ADCout3 = ads1115a.readADC_SingleEnded(2); //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3
  ADCout4 = ads1115a.readADC_SingleEnded(3); //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3

  ADCout5 = ads1115b.readADC_SingleEnded(0); //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3
  ADCout6 = ads1115b.readADC_SingleEnded(1); //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3
  ADCout7 = ads1115b.readADC_SingleEnded(2); //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3
  ADCout8 = ads1115b.readADC_SingleEnded(3); //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3
  
  reading1 = (maxADC / ADCout1)  - 1;    
  ohms1 = SERIESRESISTOR * reading1;
  reading2 = (maxADC / ADCout2)  - 1;    
  ohms2 = SERIESRESISTOR * reading2;
  reading3 = (maxADC / ADCout3)  - 1;    
  ohms3 = SERIESRESISTOR * reading3;
  reading4 = (maxADC / ADCout4)  - 1;    
  ohms4 = SERIESRESISTOR * reading4;

  reading5 = (maxADC / ADCout5)  - 1;    
  ohms5 = SERIESRESISTOR * reading5;
  reading6 = (maxADC / ADCout6)  - 1;    
  ohms6 = SERIESRESISTOR * reading6;
  reading7 = (maxADC / ADCout7)  - 1;    
  ohms7 = SERIESRESISTOR * reading7;
  reading8 = (maxADC / ADCout8)  - 1;    
  ohms8 = SERIESRESISTOR * reading8;
 
  //convert resistance to temperature in C via empirical relationship for GA10K3MCD1 thermistors (or could use Steinhart-Hart)
  tempC1 = 62.57 - ohms1*(0.005314) + 0.0000001827*ohms1*ohms1 - 0.000000000002448*ohms1*ohms1*ohms1;
  tempC2 = 62.57 - ohms2*(0.005314) + 0.0000001827*ohms2*ohms2 - 0.000000000002448*ohms2*ohms2*ohms2;
  tempC3 = 62.57 - ohms3*(0.005314) + 0.0000001827*ohms3*ohms3 - 0.000000000002448*ohms3*ohms3*ohms3;
  tempC4 = 62.57 - ohms4*(0.005314) + 0.0000001827*ohms4*ohms4 - 0.000000000002448*ohms4*ohms4*ohms4;
  tempC5 = 62.57 - ohms5*(0.005314) + 0.0000001827*ohms5*ohms5 - 0.000000000002448*ohms5*ohms5*ohms5;
  tempC6 = 62.57 - ohms6*(0.005314) + 0.0000001827*ohms6*ohms6 - 0.000000000002448*ohms6*ohms6*ohms6;
  tempC7 = 62.57 - ohms7*(0.005314) + 0.0000001827*ohms7*ohms7 - 0.000000000002448*ohms7*ohms7*ohms7;
  tempC8 = 62.57 - ohms8*(0.005314) + 0.0000001827*ohms8*ohms8 - 0.000000000002448*ohms8*ohms8*ohms8;
      
  // write sensor data to file
      //Open SD card file    
      myFile = SD.open("SAPFLUX.txt", FILE_WRITE); // open SD file   
     
      myFile.print(y);       //Year
      myFile.print(",");
      myFile.print(mo);      //Month
      myFile.print(",");
      myFile.print(d);       //Day
      myFile.print(",");
      myFile.print(hr);      //Hour
      myFile.print(",");
      myFile.print(m);       //Minute
      myFile.print(",");
      myFile.print(s);       //Second
      myFile.print(",");
      myFile.print(tempC1, 3);   //Temp (C) from Thermistor 1
      myFile.print(",");
      myFile.print(tempC2, 3);   //Temp (C) from Thermistor 2
      myFile.print(",");
      myFile.print(tempC3, 3);   //Temp (C) from Thermistor 3
      myFile.print(",");
      myFile.print(tempC4, 3);   //Temp (C) from Thermistor 4
      myFile.print(",");
      myFile.print(tempC5, 3);   //Temp (C) from Thermistor 1
      myFile.print(",");
      myFile.print(tempC6, 3);   //Temp (C) from Thermistor 2
      myFile.print(",");
      myFile.print(tempC7, 3);   //Temp (C) from Thermistor 3
      myFile.print(",");
      myFile.print(tempC8, 3);   //Temp (C) from Thermistor 4
      myFile.print(",");
      myFile.print(vin);     //Voltage
      myFile.print(",");
      myFile.println("b");   //Flag for Baseline measurement
      myFile.close();        //Data is not actually written to file until this runs
      
      delay(rep);
      s = s + 0.5;
      if(s > 59.5){
        s = 0;
        m = m+1;
      }
      basetime = basetime-1;
  }
   

//////////////////////////////     Loop to turn on heater for set amount of time     /////////////////////////////////////////

 while(heattime > 0){
      
      //Turn heater on
      digitalWrite(0,HIGH);
      digitalWrite(LED_BUILTIN, LOW);

  //Read Thermistors    
  ADCout1 = ads1115a.readADC_SingleEnded(0); //A0 input on ADS1115a; change to 1=A1, 2=A2, 3=A3
  ADCout2 = ads1115a.readADC_SingleEnded(1); //A1 input on ADS1115a; change to 1=A1, 2=A2, 3=A3
  ADCout3 = ads1115a.readADC_SingleEnded(2); //A2 input on ADS1115a; change to 1=A1, 2=A2, 3=A3
  ADCout4 = ads1115a.readADC_SingleEnded(3); //A3 input on ADS1115a; change to 1=A1, 2=A2, 3=A3

  ADCout5 = ads1115b.readADC_SingleEnded(0); //A0 input on ADS1115b; change to 1=A1, 2=A2, 3=A3
  ADCout6 = ads1115b.readADC_SingleEnded(1); //A1 input on ADS1115b; change to 1=A1, 2=A2, 3=A3
  ADCout7 = ads1115b.readADC_SingleEnded(2); //A2 input on ADS1115b; change to 1=A1, 2=A2, 3=A3
  ADCout8 = ads1115b.readADC_SingleEnded(3); //A3 input on ADS1115b; change to 1=A1, 2=A2, 3=A3
  
  reading1 = (maxADC / ADCout1)  - 1;    
  ohms1 = SERIESRESISTOR * reading1;
  reading2 = (maxADC / ADCout2)  - 1;    
  ohms2 = SERIESRESISTOR * reading2;
  reading3 = (maxADC / ADCout3)  - 1;    
  ohms3 = SERIESRESISTOR * reading3;
  reading4 = (maxADC / ADCout4)  - 1;    
  ohms4 = SERIESRESISTOR * reading4;

  reading5 = (maxADC / ADCout5)  - 1;    
  ohms5 = SERIESRESISTOR * reading5;
  reading6 = (maxADC / ADCout6)  - 1;    
  ohms6 = SERIESRESISTOR * reading6;
  reading7 = (maxADC / ADCout7)  - 1;    
  ohms7 = SERIESRESISTOR * reading7;
  reading8 = (maxADC / ADCout8)  - 1;    
  ohms8 = SERIESRESISTOR * reading8;
  
  //convert resistance to temperature in C via empirical relationship for GA10K3MCD1 thermistors (or could use Steinhart-Hart)
  tempC1 = 62.57 - ohms1*(0.005314) + 0.0000001827*ohms1*ohms1 - 0.000000000002448*ohms1*ohms1*ohms1;
  tempC2 = 62.57 - ohms2*(0.005314) + 0.0000001827*ohms2*ohms2 - 0.000000000002448*ohms2*ohms2*ohms2;
  tempC3 = 62.57 - ohms3*(0.005314) + 0.0000001827*ohms3*ohms3 - 0.000000000002448*ohms3*ohms3*ohms3;
  tempC4 = 62.57 - ohms4*(0.005314) + 0.0000001827*ohms4*ohms4 - 0.000000000002448*ohms4*ohms4*ohms4;
  tempC5 = 62.57 - ohms5*(0.005314) + 0.0000001827*ohms5*ohms5 - 0.000000000002448*ohms5*ohms5*ohms5;
  tempC6 = 62.57 - ohms6*(0.005314) + 0.0000001827*ohms6*ohms6 - 0.000000000002448*ohms6*ohms6*ohms6;
  tempC7 = 62.57 - ohms7*(0.005314) + 0.0000001827*ohms7*ohms7 - 0.000000000002448*ohms7*ohms7*ohms7;
  tempC8 = 62.57 - ohms8*(0.005314) + 0.0000001827*ohms8*ohms8 - 0.000000000002448*ohms8*ohms8*ohms8;
      
  // write sensor data to file
      //Open SD card file    
      myFile= SD.open("SAPFLUX.txt", FILE_WRITE); // open SD file   
     
      myFile.print(y);        //Year
      myFile.print(",");
      myFile.print(mo);       //Month
      myFile.print(",");
      myFile.print(d);        //Day
      myFile.print(",");
      myFile.print(hr);       //Hour
      myFile.print(",");
      myFile.print(m);        //Minute
      myFile.print(",");
      myFile.print(s);        //Second
      myFile.print(",");
      myFile.print(tempC1, 3);   //Temp (C) from Thermistor 1
      myFile.print(",");
      myFile.print(tempC2, 3);   //Temp (C) from Thermistor 2
      myFile.print(",");
      myFile.print(tempC3, 3);   //Temp (C) from Thermistor 3
      myFile.print(",");
      myFile.print(tempC4, 3);   //Temp (C) from Thermistor 4
      myFile.print(",");
      myFile.print(tempC5, 3);   //Temp (C) from Thermistor 1
      myFile.print(",");
      myFile.print(tempC6, 3);   //Temp (C) from Thermistor 2
      myFile.print(",");
      myFile.print(tempC7, 3);   //Temp (C) from Thermistor 3
      myFile.print(",");
      myFile.print(tempC8, 3);   //Temp (C) from Thermistor 4
      myFile.print(",");
      myFile.print(vin);      //Voltage
      myFile.print(",");
      myFile.println("h");    //Flag for Heating measurement
      myFile.close();         //Data is not actually written to file until this runs
      
      delay(rep);
      s = s + 0.5;
      if(s > 59.5){
        s = 0;
        m = m+1;
      }
      heattime = heattime-1;
  }

//////////////////////////////     Loop to mointor temperature probes for set amount of time     /////////////////////////////////////////

while(cycle >0){    
      
      //Turn Heater off
      digitalWrite(0,LOW);
      digitalWrite(LED_BUILTIN, HIGH);
 
  //Read Thermistors
  ADCout1 = ads1115a.readADC_SingleEnded(0); //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3
  ADCout2 = ads1115a.readADC_SingleEnded(1); //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3
  ADCout3 = ads1115a.readADC_SingleEnded(2); //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3
  ADCout4 = ads1115a.readADC_SingleEnded(3); //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3

  ADCout5 = ads1115b.readADC_SingleEnded(0); //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3
  ADCout6 = ads1115b.readADC_SingleEnded(1); //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3
  ADCout7 = ads1115b.readADC_SingleEnded(2); //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3
  ADCout8 = ads1115b.readADC_SingleEnded(3); //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3
  
  reading1 = (maxADC / ADCout1)  - 1;    
  ohms1 = SERIESRESISTOR * reading1;
  reading2 = (maxADC / ADCout2)  - 1;    
  ohms2 = SERIESRESISTOR * reading2;
  reading3 = (maxADC / ADCout3)  - 1;    
  ohms3 = SERIESRESISTOR * reading3;
  reading4 = (maxADC / ADCout4)  - 1;    
  ohms4 = SERIESRESISTOR * reading4;

  reading5 = (maxADC / ADCout5)  - 1;    
  ohms5 = SERIESRESISTOR * reading5;
  reading6 = (maxADC / ADCout6)  - 1;    
  ohms6 = SERIESRESISTOR * reading6;
  reading7 = (maxADC / ADCout7)  - 1;    
  ohms7 = SERIESRESISTOR * reading7;
  reading8 = (maxADC / ADCout8)  - 1;    
  ohms8 = SERIESRESISTOR * reading8;
 
  //convert resistance to temperature in C via empirical relationship for GA10K3MCD1 thermistors (or could use Steinhart-Hart)
  tempC1 = 62.57 - ohms1*(0.005314) + 0.0000001827*ohms1*ohms1 - 0.000000000002448*ohms1*ohms1*ohms1;
  tempC2 = 62.57 - ohms2*(0.005314) + 0.0000001827*ohms2*ohms2 - 0.000000000002448*ohms2*ohms2*ohms2;
  tempC3 = 62.57 - ohms3*(0.005314) + 0.0000001827*ohms3*ohms3 - 0.000000000002448*ohms3*ohms3*ohms3;
  tempC4 = 62.57 - ohms4*(0.005314) + 0.0000001827*ohms4*ohms4 - 0.000000000002448*ohms4*ohms4*ohms4;
  tempC5 = 62.57 - ohms5*(0.005314) + 0.0000001827*ohms5*ohms5 - 0.000000000002448*ohms5*ohms5*ohms5;
  tempC6 = 62.57 - ohms6*(0.005314) + 0.0000001827*ohms6*ohms6 - 0.000000000002448*ohms6*ohms6*ohms6;
  tempC7 = 62.57 - ohms7*(0.005314) + 0.0000001827*ohms7*ohms7 - 0.000000000002448*ohms7*ohms7*ohms7;
  tempC8 = 62.57 - ohms8*(0.005314) + 0.0000001827*ohms8*ohms8 - 0.000000000002448*ohms8*ohms8*ohms8;
      
  // write sensor data to file
      //Open SD card file    
      myFile= SD.open("SAPFLUX.txt", FILE_WRITE); // open SD file   
     
      myFile.print(y);        //Year
      myFile.print(",");
      myFile.print(mo);       //Month
      myFile.print(",");
      myFile.print(d);        //Day
      myFile.print(",");
      myFile.print(hr);       //Hour
      myFile.print(",");
      myFile.print(m);        //Minute
      myFile.print(",");
      myFile.print(s);        //Second
      myFile.print(",");
      myFile.print(tempC1, 3);   //Temp (C) from Thermistor 1
      myFile.print(",");
      myFile.print(tempC2, 3);   //Temp (C) from Thermistor 2
      myFile.print(",");
      myFile.print(tempC3, 3);   //Temp (C) from Thermistor 3
      myFile.print(",");
      myFile.print(tempC4, 3);   //Temp (C) from Thermistor 4
      myFile.print(",");
      myFile.print(tempC5, 3);   //Temp (C) from Thermistor 1
      myFile.print(",");
      myFile.print(tempC6, 3);   //Temp (C) from Thermistor 2
      myFile.print(",");
      myFile.print(tempC7, 3);   //Temp (C) from Thermistor 3
      myFile.print(",");
      myFile.print(tempC8, 3);   //Temp (C) from Thermistor 4
      myFile.print(",");
      myFile.print(vin);      //Voltage
      myFile.print(",");
      myFile.println("c");    //Flag for cycle measurement
      myFile.close();         //Data is not actually written to file until this runs
      
      
      delay(rep);
      s = s + 0.5;
      if(s > 59.5){
        s = 0;
        m = m+1;
      }
      cycle = cycle-1;
     }

      
//////////////////////////////     Reset clock flag to turn power off     /////////////////////////////////////////
   
      if(mi == 0){
      rtc.alarmClearFlag(URTCLIB_ALARM_1);
      }
      else if(mi == 30){
      rtc.alarmClearFlag(URTCLIB_ALARM_2);
      }else{
      rtc.alarmClearFlag(URTCLIB_ALARM_1);
      rtc.alarmClearFlag(URTCLIB_ALARM_2);
      }
      
      
//////////////////////////////     Error warning if the alarm doesn't work to turn off     /////////////////////////////////////////    

      
myFile= SD.open("SAPFLUX.txt", FILE_WRITE); // open SD file   
  myFile.println("err1");
  myFile.close();                                    // Err1 = ESP did not turn off after clearing the alarm flag. 
                                                    //This will appear once in data when ESP is plugged in because it cannot power off
  
  ESP.deepSleep(60e6); //this can be used instead of ESP.restart if D0 is connected to rst
  ESP.restart();
    
}

//////////////////////////////     Microcontroller should never reach this point     /////////////////////////////////////////
void loop() {
  
  }
  
