#include <SPIFFS.h>
#include <WiFi.h>
#include <WebServer.h>
#include "Sensors.h"
#include "Comm.h"
#include "time.h"
// ===== MQ-135 =====
#define MQ_PIN 34  // ADC pin

Sensors sensors(MQ_PIN);
WebServer server(80);
Comm comm(&server, &sensors);

// ===== TIME =====
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 8 * 3600;  // Philippines
const int daylightOffset_sec = 0;

unsigned long lastLog = 0;
const unsigned long logInterval = 600000;  // 10 min

void setup() {
  Serial.begin(115200);

  sensors.begin();

  // SPIFFS
  if (!SPIFFS.begin(true))
    while (1)
      ;

  // WIFI
  WiFi.mode(WIFI_STA);
  WiFi.begin("HUAWEI-2.4G", "xY4QS6gh");

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.println(WiFi.localIP());

  // TIME
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // WEB
  comm.begin();
  Serial.println("WEB SERVER STARTED");
}

void loop() {
  server.handleClient();

  if (millis() - lastLog >= logInterval) {
    lastLog = millis();
    logSensorData();
  }
}

void logSensorData() {
  sensors.read();

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;

  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);

  String line = String(buf) + ", ADC=" + String(sensors.getADC()) + ", V=" + String(sensors.getVoltage(), 3) + "\n";

  Serial.print(line);

  File f = SPIFFS.open("/logs.txt", "a");
  if (f) {
    f.print(line);
    f.close();
  }
}
