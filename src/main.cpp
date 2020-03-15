#include <Arduino.h>
#include <RtcDS3231.h>
#include <Wire.h>

RtcDS3231<TwoWire> Rtc(Wire);

// Data pin (DS)
int DATA_PIN = 3;
// Latch pin (ST_CP)
int LATCH_PIN = 4;
// Clock pin (SH_CP)
int CLOCK_PIN = 5;
int COMMON_CATHOD_LED_PINS[6] = {7, 8, 9, 10, 11, 12};
int PWM_PIN = 6;
int LDR_INPUT_PIN = 1;
int BANNER_DISPLAY_TIME_MS = 5000;
int TIME_DISPLAY_TIME_MS = 3000;
int HR1_MATRIX_WIDTH = 1;
int HR2_MATRIX_WIDTH = 3;
int MM1_MATRIX_WIDTH = 2;
int MM2_MATRIX_WIDTH = 3;
int MATRIX_ROWS = 3;

RtcDateTime getTimeFromRtc() {
  if (!Rtc.IsDateTimeValid()) {
    if (Rtc.LastError() != 0) {
      Serial.print("RTC communications error: ");
      Serial.println(Rtc.LastError());
    } else {
      Serial.println("RTC lost confidence in the DateTime!");
    }
  }

  return Rtc.GetDateTime();
}

void displayLed(int ch1[3], int ch2[3]) {
  for (int j = 0; j < 3; j++) {
    digitalWrite(LATCH_PIN, LOW);
    digitalWrite(COMMON_CATHOD_LED_PINS[j], LOW);
    digitalWrite(COMMON_CATHOD_LED_PINS[j + 3], LOW);

    shiftOut(DATA_PIN, CLOCK_PIN, LSBFIRST, ch2[j]);
    shiftOut(DATA_PIN, CLOCK_PIN, LSBFIRST, ch1[j]);

    digitalWrite(LATCH_PIN, HIGH);
    digitalWrite(LATCH_PIN, LOW);
    
    shiftOut(DATA_PIN, CLOCK_PIN, LSBFIRST, 0); // To get rid of flicker when
    
    digitalWrite(LATCH_PIN, HIGH);
    digitalWrite(COMMON_CATHOD_LED_PINS[j], HIGH);
    digitalWrite(COMMON_CATHOD_LED_PINS[j + 3], HIGH);
  }
}

String createBinaryStr(int ledArrLength, int ledOnCount) {
  String str = "";
  int randomIndexes[12] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  int counter = 0;

  for (int i = 0; i < ledArrLength; i++) {
    str += "0";
  }

  while (counter < ledOnCount) {
    int randomIndexFound = false;
    int randomIndex = random(0, ledArrLength);

    for (int i = 0; i < ledOnCount; i++) {
      if (randomIndexes[i] == randomIndex) {
        randomIndexFound = true;
        break;
      }
    }

    if (!randomIndexFound) {
      randomIndexes[counter] = randomIndex;
      counter++;
    }
  }

  for (int i = 0; i < ledOnCount; i++) {
    if (randomIndexes[i] != -1) {
      str.setCharAt(randomIndexes[i], '1');
    }
  }

  return str;
}

int readBinaryString(String ss) {
  char *s = const_cast<char *>(ss.c_str());
  int result = 0;
  while (*s) {
    result <<= 1;
    if (*s++ == '1')
      result |= 1;
  }
  return result;
}

void showStartupBanner() {
  int ledArr1[3] = {readBinaryString("11101101"), readBinaryString("01001010"),
                    readBinaryString("01001101")};
  int ledArr2[3] = {readBinaryString("0"), readBinaryString("0"),
                    readBinaryString("0")};

  int loopCounter = 0;
  for (int i = 0; i < BANNER_DISPLAY_TIME_MS; i++) {
    if (loopCounter % 500 == 0) {
      analogWrite(PWM_PIN, map(analogRead(LDR_INPUT_PIN), 0, 1024, 0, 255));
    }
    displayLed(ledArr1, ledArr2);
    loopCounter++;
  }
}

void setup() {
  Serial.begin(9600);

  Serial.print("Current Date Time: ");
  Serial.print(__DATE__);
  Serial.print(" ");
  Serial.println(__TIME__);
  delay(100);

  Rtc.Begin();

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

  if (!Rtc.IsDateTimeValid()) {
    if (Rtc.LastError() != 0) {
      Serial.print("RTC communications error: ");
      Serial.println(Rtc.LastError());
    } else {
      Serial.println("RTC lost confidence in the DateTime!");
      Rtc.SetDateTime(compiled);
    }
  }

  if (!Rtc.GetIsRunning()) {
    Serial.println("RTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled) {
    Serial.println("RTC is older than compile time!  (Updating DateTime)");
    Rtc.SetDateTime(compiled);
  } else if (now > compiled) {
    Serial.println("RTC is newer than compile time. (this is expected)");
  } else if (now == compiled) {
    Serial.println(
        "RTC is the same as compile time! (not expected but all is fine)");
  }

  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);

  Serial.begin(9600);
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);

  for (int i = 0; i < 6; i++) {
    pinMode(COMMON_CATHOD_LED_PINS[i], OUTPUT);
    digitalWrite(COMMON_CATHOD_LED_PINS[i], HIGH);
  }
  pinMode(PWM_PIN, OUTPUT);

  showStartupBanner();
}

void loop() {
  RtcDateTime dateTime = getTimeFromRtc();
  int hour = dateTime.Hour();
  int minute = dateTime.Minute();

  int h1 = hour / 10;
  int h2 = hour % 10;
  int m1 = minute / 10;
  int m2 = minute % 10;

  String hh1 = createBinaryStr(HR1_MATRIX_WIDTH * MATRIX_ROWS, h1);
  String hh2 = createBinaryStr(HR2_MATRIX_WIDTH * MATRIX_ROWS, h2);
  String mm1 = createBinaryStr(MM1_MATRIX_WIDTH * MATRIX_ROWS, m1);
  String mm2 = createBinaryStr(MM2_MATRIX_WIDTH * MATRIX_ROWS, m2);

  String ledArr1Str1 = hh2.substring(0, HR2_MATRIX_WIDTH) +
                       mm1.substring(0, MM1_MATRIX_WIDTH) +
                       mm2.substring(0, MM2_MATRIX_WIDTH);
  String ledArr1Str2 = hh2.substring(HR2_MATRIX_WIDTH, HR2_MATRIX_WIDTH * 2) +
                       mm1.substring(MM1_MATRIX_WIDTH, MM1_MATRIX_WIDTH * 2) +
                       mm2.substring(MM2_MATRIX_WIDTH, MM2_MATRIX_WIDTH * 2);
  String ledArr1Str3 =
      hh2.substring(HR2_MATRIX_WIDTH * 2, HR2_MATRIX_WIDTH * 3) +
      mm1.substring(MM1_MATRIX_WIDTH * 2, MM1_MATRIX_WIDTH * 3) +
      mm2.substring(MM2_MATRIX_WIDTH * 2, MM2_MATRIX_WIDTH * 3);

  String ledArr2Str1 = hh1.substring(0, HR1_MATRIX_WIDTH);
  String ledArr2Str2 = hh1.substring(HR1_MATRIX_WIDTH, HR1_MATRIX_WIDTH * 2);
  String ledArr2Str3 =
      hh1.substring(HR1_MATRIX_WIDTH * 2, HR1_MATRIX_WIDTH * 3);

  int ledArr1[3] = {readBinaryString(ledArr1Str1),
                    readBinaryString(ledArr1Str2),
                    readBinaryString(ledArr1Str3)};
  int ledArr2[3] = {readBinaryString(ledArr2Str1),
                    readBinaryString(ledArr2Str2),
                    readBinaryString(ledArr2Str3)};

  int loopCounter = 0;
  for (int i = 0; i < TIME_DISPLAY_TIME_MS; i++) {
    if (loopCounter % 500 == 0) {
      analogWrite(PWM_PIN, map(analogRead(LDR_INPUT_PIN), 0, 1024, 0, 255));
    }
    displayLed(ledArr1, ledArr2);
    loopCounter++;
  }
}