#include <avr/wdt.h>

#include "Adafruit_NeoPixel.h"
#include "LsmFadeOutput.h"

// speed for the com port for talking with xlights. From 9600 to 115200. Use the
// same speed as set in xlights.
const unsigned long SERIAL_SPEED = 250000;
const int CHANNEL_COUNT = 703;
const int NEOPIXEL_COUNT = 150;
const int NEOPIXEL_PAD_COUNT = 81;

Adafruit_NeoPixel pixels(NEOPIXEL_COUNT, /* pin */ 8, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel pixelsPad(NEOPIXEL_PAD_COUNT*2, /* pin */ 9, NEO_GRB + NEO_KHZ800);
LsmFadeOutput output;

uint8_t buffer_data[CHANNEL_COUNT];
bool isReading = true;
int offset = 0;
bool showed = false;

void setup() {
  // set up Serial according to the speed defined above.
  Serial.begin(SERIAL_SPEED);

  // enable the watchdog timer with a time of 1 second. If the board freezes, it
  // will reset itself after 1 second.
  wdt_enable(WDTO_1S);

  pixels.begin();
  pixelsPad.begin();

  // 2ms timeout
  Serial.setTimeout(1);

  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
}

uint8_t blockingRead() {
  while (Serial.available() == 0) {
  }
  return Serial.read();
}

uint8_t blockingPeek() {
  while (Serial.available() == 0) {
  }
  return Serial.peek();
}

// read "lsm"
bool readPrefix() {
  while (Serial.available() >= 3) {
    if (Serial.read() != 'l') {
      continue;
    }
    if (Serial.peek() != 's') {
      continue;
    }
    Serial.read();
    if (Serial.peek() != 'm') {
      continue;
    }
    Serial.read();
    return true;
  }
  return false;
}

void doProcessBuffer() {
  // read prefix
  if (!isReading) {
    if (readPrefix()) {
      isReading = true;
    } else {
      return;
    }
  }
  
  while (Serial.available() > 0 && offset < CHANNEL_COUNT) {
    buffer_data[offset++] = Serial.read();
  }
}

bool processBuffer() {
  doProcessBuffer();
  // read content
  if (offset == CHANNEL_COUNT) {
    offset = 0;
    isReading = false;
    return true;
  }
  return false;
}

void loop() {
  // resets the watchdog
  wdt_reset();

  if (processBuffer()) {
    int i = 10;
    for (int j = 0; j < NEOPIXEL_COUNT; j++) {
      byte r = buffer_data[i++];
      byte g = buffer_data[i++];
      byte b = buffer_data[i++];
      pixels.setPixelColor(j, pixels.Color(r, g, b));
    }
    for (int j = 0; j < NEOPIXEL_PAD_COUNT; j++) {
      byte r = buffer_data[i++];
      byte g = buffer_data[i++];
      byte b = buffer_data[i++];
      int dest = j;
      // invert order every 2 rows
      int row = j / 9;
      int col = j % 9;
      if (row % 2 == 0) {
        col=8-col;
        dest = row * 9 + col;
      }
      dest=dest*2;
      pixelsPad.setPixelColor(dest, pixels.Color(r, g, b));
      pixelsPad.setPixelColor(dest+1, pixels.Color(r, g, b));
    }
    showed = false;
  }

  if (!showed && pixels.canShow()) {
    pixels.show();
    pixelsPad.show();
    showed = true;
  }

  static bool inverted = false;
  inverted = !inverted;
  int shift = 0;
  digitalWrite(2, inverted ? buffer_data[shift] > 127 : 0);
  digitalWrite(3, !inverted ? buffer_data[shift + 1] > 127 : 0);
  digitalWrite(4, inverted ? buffer_data[shift + 2] > 127 : 0);
  digitalWrite(5, !inverted ? buffer_data[shift + 3] > 127 : 0);
}
