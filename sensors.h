#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>

class Sensors {
  private:
    int mqPin;
    int lastADC = 0;
    float lastVoltage = 0;

  public:
    Sensors(int pin) : mqPin(pin) {}

    void begin() {
      pinMode(mqPin, INPUT);
      analogReadResolution(12);      
      analogSetAttenuation(ADC_11db);
    }

    void read() {
      lastADC = analogRead(mqPin);
      lastVoltage = lastADC * (3.3 / 4095.0);
    }

    int getADC() { return lastADC; }
    float getVoltage() { return lastVoltage; }
};

#endif
