#pragma once

#include "Arduino.h"

// analog output pins on any port
class LsmFadeOutput {
public:
  LsmFadeOutput();

  void analogWrite(int pin, uint8_t val);

  void loop();

private:
  struct OutputState {
    bool enabled = false;

    volatile uint8_t *outputRegister = 0;
    uint8_t bitMask = 0;

    // fade value
    uint8_t value;

    // instant value
    bool on = false;
  };

  // microseconds loop
  static constexpr int CYCLE = 1000;

  // covers both digital and analogs (eg: 70 for mega is 54+16)
  static constexpr int SIZE = NUM_DIGITAL_PINS;

  OutputState _pins[SIZE];

  void setOutput(int pinNumber, bool value) {
    OutputState &pin = _pins[pinNumber];
    if (value) {
      *pin.outputRegister |= pin.bitMask;
    } else {
      *pin.outputRegister &= ~pin.bitMask;
    }
  }

  void flushLights() {}
};

inline LsmFadeOutput::LsmFadeOutput() {
  for (int i = 0; i < SIZE; i++) {
    OutputState &pin = _pins[i];
    pin.outputRegister = portOutputRegister(digitalPinToPort(i));
    pin.bitMask = digitalPinToBitMask(i);
  }
}

inline void LsmFadeOutput::analogWrite(int pinNumber, uint8_t val) {
  OutputState &pin = _pins[pinNumber];
  pin.value = val;
  if (!pin.enabled) {
    pin.enabled = true;
    pinMode(pinNumber, OUTPUT);
  }
}

inline void LsmFadeOutput::loop() {
  unsigned long start = micros();

  // turn on outputs
  for (int i = 0; i < SIZE; i++) {
    OutputState &pin = _pins[i];
    if (pin.enabled) {
      pin.on = pin.value > 0;
      setOutput(i, pin.on);
    }
  }
  flushLights();

  // turn off a the right time
  while (true) {
    uint32_t delta = micros() - start;
    if (delta > CYCLE)
      break;
    uint8_t cycle = delta * 255 / CYCLE;
    for (int i = 0; i < SIZE; i++) {
      OutputState &pin = _pins[i];
      if (pin.enabled && pin.on && cycle >= pin.value) {
        setOutput(i, false);
        pin.on = false;
      }
    }
    flushLights();
  }

  // turn off all
  for (int i = 0; i < SIZE; i++) {
    OutputState &pin = _pins[i];
    if (pin.enabled) {
      setOutput(i, false);
    }
  }
  flushLights();
}
