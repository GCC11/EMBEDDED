#ifndef SENSORS_H
#define SENSORS_H

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <BH1750.h>
#include <Wire.h>

class Sensors {
  private:
    Adafruit_BME280 bme;
    BH1750 lightMeter;
  public:
    float lastTemp=0, lastHum=0, lastPres=0, lastLux=0;

    void begin() {
      Wire.begin(21,22);
      if(!bme.begin(0x76) && !bme.begin(0x77)) while(1);
      if(!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) while(1);
    }

    void power(bool state, int bmePin, int bhPin) {
      digitalWrite(bmePin, state?HIGH:LOW);
      digitalWrite(bhPin, state?HIGH:LOW);
    }

    void read() {
      lastTemp = bme.readTemperature();
      lastHum = bme.readHumidity();
      lastPres = bme.readPressure()/100.0F;
      lastLux = lightMeter.readLightLevel();
    }

    float getTemp(){ return lastTemp; }
    float getHum(){ return lastHum; }
    float getPres(){ return lastPres; }
    float getLux(){ return lastLux; }
};

#endif
