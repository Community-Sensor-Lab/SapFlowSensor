// routine to input date time from Serial

DateTime inputDateTime() {
  char datetime[32] = "YYYY/MM/DD hh:mm:ss";
  rtc_ds3231.now().toString(datetime);
  Serial.print("current rtc_ds3231 time: ");
  Serial.println(datetime);

  // flush the serial
  while (Serial.available() > 0)
    Serial.read();

  Serial.println("enter date-time as YYYY/MM/dd hh:mm:ss");

  static char message[32];
  static unsigned int message_pos = 0;

  while (Serial.available() == 0);

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

void printDateTime() {

  char dt[32] = "YY/MM/DD hh:mm:ss";
  rtc_ds3231.now().toString(dt);
  Serial.print("Alarm occured every ");
  Serial.print(ALARM1_EVERY_N_MINS);
  Serial.print(" mins + ");
  Serial.print(ALARM1_EVERY_N_SECS);
  Serial.print(" secs at ");
  Serial.print(dt);
}
