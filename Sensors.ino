#include <SPIFFS.h>
#include "Sensors.h"
#include "Comm.h"
#include "time.h"    // ESP32 time library

#define BME_POWER_PIN 25
#define BH1750_POWER_PIN 26
int disabledPins[] = {12,13,14,27};

Sensors sensors;
WebServer server(80);
Comm comm(&server, &sensors, BME_POWER_PIN, BH1750_POWER_PIN);

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 8 * 3600;  // Philippines
const int daylightOffset_sec = 0;

unsigned long lastLog = 0;
const unsigned long logInterval = 600000; // 10 min

void setup() {
  Serial.begin(115200);
  

  // Initialize pins
  for (int p : disabledPins) pinMode(p, INPUT_PULLDOWN);
  pinMode(BME_POWER_PIN, OUTPUT);
  pinMode(BH1750_POWER_PIN, OUTPUT);

  // Power sensors and initialize
  sensors.power(true, BME_POWER_PIN, BH1750_POWER_PIN);
  sensors.begin();

  // Start SPIFFS
  if (!SPIFFS.begin(true)) while (1);

  // ===== WIFI =====
  // ===== WIFI (DHCP – DEBUG SAFE) =====
  WiFi.mode(WIFI_STA);
  WiFi.begin("realme", "12345678");

  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
}
  Serial.println();

  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());


  // ===== NTP TIME (non-blocking) =====
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Start web server
  comm.begin();
  Serial.println("WEB SERVER STARTED");

}

void loop() {
  server.handleClient();

  // Log sensor data every logInterval
  if (millis() - lastLog >= logInterval) {
    lastLog = millis();
    logSensorData();
  }
}

void logSensorData() {
  sensors.read();

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Time not yet synced");
    return; // skip logging until NTP sync
  }

  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);

  String line = String(buf) + ", " +
                "T=" + String(sensors.getTemp(), 2) + "C, " +
                "H=" + String(sensors.getHum(), 2) + "%, " +
                "P=" + String(sensors.getPres(), 2) + "hPa, " +
                "L=" + String(sensors.getLux(), 2) + "lx\n";

  Serial.print(line);

  File f = SPIFFS.open("/logs.txt", "a");
  if (f) {
    f.print(line);
    f.close();
  } else {
    Serial.println("Failed to open log file");
  }
}
