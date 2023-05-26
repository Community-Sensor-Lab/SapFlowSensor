float measureVoltage() {
  pinMode(A7, INPUT);
  return analogRead(A7) * 0.006445;
  //  int value = analogRead(A0);
  //  float vout = (value * 3.37046) / 1024.0; // The value of 2.97 is the calibrated Arduino ADC reference voltage (nominally 3.3V).  Worth double checking using a AC->DC converter.
  //  return vout / (R2 / (R1 + R2));
}

void readThermistor() {

  int16_t ADCout1 = ads1.readADC_SingleEnded(0);  //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3
  int16_t ADCout2 = ads1.readADC_SingleEnded(1);  //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3
  int16_t ADCout3 = ads1.readADC_SingleEnded(2);  //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3
  int16_t ADCout4 = ads1.readADC_SingleEnded(3);  //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3
  int16_t ADCout5 = ads2.readADC_SingleEnded(0);  //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3
  int16_t ADCout6 = ads2.readADC_SingleEnded(1);  //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3
  int16_t ADCout7 = ads2.readADC_SingleEnded(2);  //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3
  int16_t ADCout8 = ads2.readADC_SingleEnded(3);  //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3

  Serial.printf("ADCs: %d, %d, %d, %d, %d, %d, %d, %d \n",
                ADCout1, ADCout2, ADCout3, ADCout4, ADCout5, ADCout6, ADCout7, ADCout8);

  float ohms1 = SERIESRESISTOR * ((MAX_ADC / (float)ADCout1) - 1);
  float ohms2 = SERIESRESISTOR * ((MAX_ADC / (float)ADCout2) - 1);
  float ohms3 = SERIESRESISTOR * ((MAX_ADC / (float)ADCout3) - 1);
  float ohms4 = SERIESRESISTOR * ((MAX_ADC / (float)ADCout4) - 1);
  float ohms5 = SERIESRESISTOR * ((MAX_ADC / (float)ADCout5) - 1);
  float ohms6 = SERIESRESISTOR * ((MAX_ADC / (float)ADCout6) - 1);
  float ohms7 = SERIESRESISTOR * ((MAX_ADC / (float)ADCout7) - 1);
  float ohms8 = SERIESRESISTOR * ((MAX_ADC / (float)ADCout8) - 1);

  // Serial.print("ohms: ");
  // Serial.print(ohms1, 6);
  // Serial.print(", ");
  // Serial.print(ohms2, 6);
  // Serial.print(", ");
  // Serial.print(ohms3, 6);
  // Serial.print(", ");
  // Serial.print(ohms4, 6);
  // Serial.print(", ");
  // Serial.print(ohms5, 6);
  // Serial.print(", ");
  // Serial.print(ohms6, 6);
  // Serial.print(", ");
  // Serial.print(ohms7, 6);
  // Serial.print(", ");
  // Serial.print(ohms8, 6);
  // Serial.println();

  // this function temp(ohms) is a little sus
  tempC1 = 62.57 - ohms1 * (0.005314) + 0.0000001827 * ohms1 * ohms1 - 0.000000000002448 * ohms1 * ohms1 * ohms1;
  tempC2 = 62.57 - ohms2 * (0.005314) + 0.0000001827 * ohms2 * ohms2 - 0.000000000002448 * ohms2 * ohms2 * ohms2;
  tempC3 = 62.57 - ohms3 * (0.005314) + 0.0000001827 * ohms3 * ohms3 - 0.000000000002448 * ohms3 * ohms3 * ohms3;
  tempC4 = 62.57 - ohms4 * (0.005314) + 0.0000001827 * ohms4 * ohms4 - 0.000000000002448 * ohms4 * ohms4 * ohms4;
  tempC5 = 62.57 - ohms5 * (0.005314) + 0.0000001827 * ohms5 * ohms5 - 0.000000000002448 * ohms5 * ohms5 * ohms5;
  tempC6 = 62.57 - ohms6 * (0.005314) + 0.0000001827 * ohms6 * ohms6 - 0.000000000002448 * ohms6 * ohms6 * ohms6;
  tempC7 = 62.57 - ohms7 * (0.005314) + 0.0000001827 * ohms7 * ohms7 - 0.000000000002448 * ohms7 * ohms7 * ohms7;
  tempC8 = 62.57 - ohms8 * (0.005314) + 0.0000001827 * ohms8 * ohms8 - 0.000000000002448 * ohms8 * ohms8 * ohms8;

  // Serial.print("TEMP: ");
  // Serial.print(tempC1, 6);
  // Serial.print(", ");
  // Serial.print(tempC2, 6);
  // Serial.print(", ");
  // Serial.print(tempC3, 6);
  // Serial.print(", ");
  // Serial.print(tempC4, 6);
  // Serial.print(", ");
  // Serial.print(tempC5, 6);
  // Serial.print(", ");
  // Serial.print(tempC6, 6);
  // Serial.print(", ");
  // Serial.print(tempC7, 6);
  // Serial.print(", ");
  // Serial.print(tempC8, 6);
  // Serial.println();
}

void writeSD() {
  File myFile = SD.open("SAPFLUX.txt", FILE_WRITE);

  char datetime[32] = "YYYY/MM/DD hh:mm:ss";
  rtc_3231.now().toString(datetime);
  myFile.print(datetime);

  char tempString[128];


  String outString = String(tempC1) + String(", ") + String(tempC2) + String(", ") + String(tempC3) + String(", ") + String(tempC4) + String(", ")
                     + String(tempC5) + String(", ") + String(tempC6) + String(", ") + String(tempC7) + String(", ") + String(tempC8) + String(", ") + 'b';

  myFile.print(" ");
  myFile.println(outString);


  myFile.close();  //Data is not actually written to file until this runs
}

DateTime inputDateTime() {
  char datetime[32] = "YYYY/MM/DD hh:mm:ss";
  rtc_3231.now().toString(datetime);
  Serial.print("current RTC time: ");
  Serial.println(datetime);

  while (Serial.available() > 0)
    Serial.read();

  Serial.println("enter date-time as YYYY/MM/dd hh:mm:ss");

  static char message[32];
  static unsigned int message_pos = 0;

  while (Serial.available() == 0)
    ;
  while (Serial.available() > 0) {
    char inByte = Serial.read();
    //Message coming in (check not terminating character) and guard for over message size
    if (inByte != '\n' && (message_pos < 32 - 1)) {
      //Add the incoming byte to our message
      message[message_pos] = inByte;
      message_pos++;
    }
    //Full message received...
    else {
      //Add null character to string
      message[message_pos] = '\0';

      //Print the message (or do other things)
      Serial.println(message);

      //Reset for the next message
      message_pos = 0;
    }
  }

  if (message) {
    int Year, Month, Day, Hour, Minute, Second;
    sscanf(message, "%d/%d/%d %d:%d:%d", &Year, &Month, &Day, &Hour, &Minute, &Second);

    Serial.print("input message: ");
    Serial.println(message);
    Serial.print("Values: ");
    Serial.print(Year);
    Serial.print("/");
    Serial.print(Month);
    Serial.print("/");
    Serial.print(Day);
    Serial.print(" ");
    Serial.print(Hour);
    Serial.print(":");
    Serial.print(Minute);
    Serial.print(":");
    Serial.println(Second);
    Serial.println();

    return DateTime(Year, Month, Day, Hour, Minute, Second);
  } else
    return DateTime(0, 0, 0, 0, 0, 0);
}
