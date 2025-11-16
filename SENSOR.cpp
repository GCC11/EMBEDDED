#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "BH1750.h"  // For GY-302 Light Sensor

// ---- WiFi CONFIG ----
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// ---- SENSOR OBJECTS ----
Adafruit_BME280 bme;     // BME280
BH1750 lightMeter;        // GY-302

// ---- MQ135 PIN ----
int mq135Pin = 34;  // ESP32 ADC pin

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // --- WiFi Connect ---
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");

  // --- Start BME280 ---
  if (!bme.begin(0x76)) {  // Address for most BME280
    Serial.println("Could not find BME280!");
  }

  // --- Start GY-302 Light Sensor ---
  if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("GY-302 Light Sensor error!");
  }

  pinMode(mq135Pin, INPUT);
}

void loop() {
  // ---- Read BME280 ----
  float temp = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F;

  // ---- Read GY-302 ----
  float lux = lightMeter.readLightLevel();

  // ---- Read MQ-135 ----
  int mqRaw = analogRead(mq135Pin);
  float mqVoltage = mqRaw * (3.3 / 4095.0);

  // ---- Print Data ----
  Serial.println("--------- SENSOR DATA ---------");
  Serial.print("Temperature: "); Serial.println(temp);
  Serial.print("Humidity: "); Serial.println(humidity);
  Serial.print("Pressure: "); Serial.println(pressure);
  Serial.print("Light (lux): "); Serial.println(lux);
  Serial.print("MQ-135 Voltage: "); Serial.println(mqVoltage);

  delay(2000);
}
