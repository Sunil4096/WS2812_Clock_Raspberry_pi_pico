
#include <Adafruit_NeoPixel.h>
#include "hardware/rtc.h"

#define PIN 0
#define NUM_LEDS 84
Adafruit_NeoPixel strip(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

#define bt_up 13
#define bt_set 14
#define bt_dwn 15

uint8_t mode = 0;
uint8_t bright_val = 2;
uint8_t delay_val = 50;
float battery_good_v = 3.3;
uint8_t old_sec;

uint32_t colorWhite = strip.Color(255, 255, 255);
uint32_t colorRed = strip.Color(255, 0, 0);
uint32_t colorToUse;

const uint8_t segments[7][2] = {
  { 0, 1 }, { 2, 3 }, { 4, 5 }, { 6, 7 }, { 8, 9 }, { 10, 11 }, { 12, 13 }
};

const bool digitSegments[10][7] = {
  { 1, 1, 1, 1, 1, 1, 0 },  // 0
  { 0, 1, 1, 0, 0, 0, 0 },  // 1
  { 1, 1, 0, 1, 1, 0, 1 },  // 2
  { 1, 1, 1, 1, 0, 0, 1 },  // 3
  { 0, 1, 1, 0, 0, 1, 1 },  // 4
  { 1, 0, 1, 1, 0, 1, 1 },  // 5
  { 1, 0, 1, 1, 1, 1, 1 },  // 6
  { 1, 1, 1, 0, 0, 1, 0 },  // 7
  { 1, 1, 1, 1, 1, 1, 1 },  // 8
  { 1, 1, 1, 1, 0, 1, 1 }   // 9
};

void showDigitAtPosition(int digit, int position, uint32_t color) {
  int offset = position * 14;
  for (int seg = 0; seg < 7; seg++) {
    for (int i = 0; i < 2; i++) {
      int ledIndex = offset + segments[seg][i];
      strip.setPixelColor(ledIndex, digitSegments[digit][seg] ? color : 0);
    }
  }
}

void setup() {
  pinMode(bt_up, INPUT_PULLUP);
  pinMode(bt_set, INPUT_PULLUP);
  pinMode(bt_dwn, INPUT_PULLUP);
  analogReadResolution(12);

  strip.begin();
  strip.clear();
  strip.setBrightness(2);
  strip.show();

  if (!rtc_running()) {
    rtc_init();
    datetime_t t = {
      .year = 2025,
      .month = 6,
      .day = 29,
      .dotw = 0,  // 0=Sunday
      .hour = 12,
      .min = 0,
      .sec = 0
    };
    rtc_set_datetime(&t);
  }
}

void loop() {
  datetime_t t;
  rtc_get_datetime(&t);

  float battery_v = (analogRead(29) * 9.9) / 4095.0;

  if (battery_v >= battery_good_v) {
    battery_good_v = 3.3;
    delay_val = 50;
    // Short press bt_set to switch modes
    if (!digitalRead(bt_set)) {
      mode++;
      if (mode > 4) mode = 0;
      while (!digitalRead(bt_set)) delay(10);
    }

    // Adjust time
    if (mode == 1) {
      if (!digitalRead(bt_up)) {
        t.hour = (t.hour + 1) % 12;
        rtc_set_datetime(&t);
        while (!digitalRead(bt_up)) delay(10);
      }
      if (!digitalRead(bt_dwn)) {
        t.hour = (t.hour == 0) ? 11 : t.hour - 1;
        rtc_set_datetime(&t);
        while (!digitalRead(bt_dwn)) delay(10);
      }
    } else if (mode == 2) {
      if (!digitalRead(bt_up)) {
        t.min = (t.min + 1) % 60;
        rtc_set_datetime(&t);
        while (!digitalRead(bt_up)) delay(10);
      }
      if (!digitalRead(bt_dwn)) {
        t.min = (t.min == 0) ? 59 : t.min - 1;
        rtc_set_datetime(&t);
        while (!digitalRead(bt_dwn)) delay(10);
      }
    } else if (mode == 3) {
      strip.clear();
      if (!digitalRead(bt_up)) {
        bright_val += 1;
        strip.setBrightness(bright_val);
        while (!digitalRead(bt_up)) delay(10);
      }
      if (!digitalRead(bt_dwn)) {
        bright_val -= 1;
        if (bright_val <= 1) { bright_val = 1; }
        strip.setBrightness(bright_val);
        while (!digitalRead(bt_dwn)) delay(10);
      }
      for (int i = 74; i < 84; i++) {
        strip.setPixelColor(i, strip.Color(255, 0, 0));

        long temp = bright_val;
        // Extract digits from rightmost to leftmost
        for (int pos = 0; pos < 3; pos++) {
          int digit = temp % 10;
          showDigitAtPosition(digit, pos, colorRed);  // White
          temp /= 10;
        }
      }
    }

    if (t.hour >= 13) {
      t.hour = 1;
      rtc_set_datetime(&t);
    }

    if (mode != 3) {
      // Format digits (HH MM SS)
      int digits[6];
      digits[5] = t.hour / 10;
      digits[4] = t.hour % 10;
      digits[3] = t.min / 10;
      digits[2] = t.min % 10;
      digits[1] = t.sec / 10;
      digits[0] = t.sec % 10;

      // strip.clear();
      for (int pos = 0; pos < 6; pos++) {
        if (mode != 4) {
          if (pos == 4 || pos == 5) {
            colorToUse = strip.Color(155, 255, 0);  // color for hour
          } else if (pos == 2 || pos == 3) {
            colorToUse = strip.Color(0, 177, 255);  // color for min
          } else if (pos == 1 || pos == 0) {
            colorToUse = strip.Color(255, 150, 100);  // color for sec
          }
        }

        if (mode == 2 && (pos == 2 || pos == 3)) {  // Highlight minute digits in red (positions 2 & 3)
          colorToUse = colorRed;
        }
        if (mode == 1 && (pos == 4 || pos == 5)) {  // Highlight hour digits in red (positions 4 & 5)
          colorToUse = colorRed;
        }
        if (mode == 4 && t.sec != old_sec) {
          if (pos == 4) {
            colorToUse = strip.Color(random(50, 255), random(50, 255), random(50, 255));  // color for hour
          } else if (pos == 2) {
            colorToUse = strip.Color(random(50, 255), random(50, 255), random(50, 255));  // color for min
          } else if (pos == 0) {
            colorToUse = strip.Color(random(50, 255), random(50, 255), random(50, 255));  // color for sec
          }
          showDigitAtPosition(digits[pos], pos, colorToUse);
        }
        if (mode != 4) {
          showDigitAtPosition(digits[pos], pos, colorToUse);
        }
      }
    }
    strip.show();
  } else {
    strip.clear();
    strip.show();
    battery_good_v = 3.5;
    delay_val = 2000;
  }
  old_sec = t.sec;
  delay(delay_val);
}
