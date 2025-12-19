#ifndef COMM_H
#define COMM_H

#include <WiFi.h>
#include <WebServer.h>
#include "Sensors.h"
#include "SPIFFS.h"

#define SMOKE_THRESHOLD 500

class Comm {
  private:
    WebServer* server;
    Sensors* sensors;

    String username = "admin";
    String password = "1234";

    bool isLoggedIn() {
      if (!server->hasHeader("Cookie")) return false;
      return server->header("Cookie").indexOf("ESPSESSION=1") >= 0;
    }

    // ===== SAME LOGIN STYLE =====
    String loginPage() {
      return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>Login</title>
<style>
body{font-family:Arial;background:#f2f2f2}
.box{width:300px;margin:120px auto;padding:20px;background:#fff;border-radius:8px}
input,button{width:100%;padding:8px;margin-top:10px;border-radius:4px}
</style>
</head>
<body>
<div class="box">
<h3>Login</h3>
<form method="POST" action="/login">
<input name="user" placeholder="Username">
<input name="pass" type="password" placeholder="Password">
<button type="submit">Login</button>
</form>
</div>
</body>
</html>
)rawliteral";
    }

  public:
    Comm(WebServer* s, Sensors* sens) : server(s), sensors(sens) {}

    void begin() {
      const char* keys[] = {"Cookie"};
      server->collectHeaders(keys, 1);

      server->on("/", [this](){ handleRoot(); });
      server->on("/login", HTTP_POST, [this](){ handleLogin(); });
      server->on("/logout", [this](){ handleLogout(); });
      server->on("/data", [this](){ handleData(); });
      server->on("/logs", [this](){ handleLogs(); });

      server->begin();
    }

    void handleRoot() {
      if (!isLoggedIn()) {
        server->send(200, "text/html", loginPage());
        return;
      }

      // ===== SAME DASHBOARD STYLE =====
      String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>MQ-135 Dashboard</title>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
body{font-family:Arial}
.mode{display:inline-block;padding:15px 25px;margin:5px;border-radius:8px;font-weight:bold;background:#ddd}
.active{background:#4CAF50;color:white}
.section{margin-top:15px;padding:15px;border:1px solid #ccc;border-radius:10px}
@media(max-width:600px){
  body{font-size:18px}
  .mode{display:block;width:100%;padding:18px}
  .section{padding:20px}
}
</style>
</head>
<body>

<h2>MQ-135 AIR QUALITY</h2>
<a href="/logout">Logout</a>

<div>
  <div class="mode active">AUTO</div>
</div>

<div class="section">
  <h3>Live Data</h3>
  <div id="data">Loading...</div>

  <hr>

  <h3>Status</h3>
  <div id="status">Loading...</div>

  <br>
  <a href="/logs" target="_blank">View Logs</a>
</div>

<script>
async function update(){
  let j = await (await fetch('/data')).json();
  document.getElementById('data').innerHTML =
    'ADC: ' + j.adc + '<br>' +
    'Voltage: ' + j.v + ' V';

  document.getElementById('status').innerHTML =
    j.status === 'SMOKE DETECTED'
    ? '<span style="color:red;font-weight:bold">SMOKE / GAS DETECTED</span>'
    : '<span style="color:green">AIR NORMAL</span>';
}
setInterval(update,1000);
update();
</script>

</body>
</html>
)rawliteral";

      server->send(200, "text/html", html);
    }

    void handleLogin() {
      if (server->arg("user")==username && server->arg("pass")==password) {
        server->sendHeader("Set-Cookie","ESPSESSION=1; Path=/");
        server->sendHeader("Location","/");
        server->send(302);
      } else {
        server->send(401,"text/plain","Wrong login");
      }
    }

    void handleLogout() {
      server->sendHeader("Set-Cookie","ESPSESSION=0; Path=/; Max-Age=0");
      server->sendHeader("Location","/");
      server->send(302);
    }

    void handleData() {
      if (!isLoggedIn()) { server->send(403); return; }

      sensors->read();
      int adc = sensors->getADC();
      String status = adc > SMOKE_THRESHOLD ? "SMOKE DETECTED" : "AIR NORMAL";

      server->send(200,"application/json",
        "{\"adc\":"+String(adc)+
        ",\"v\":"+String(sensors->getVoltage(),3)+
        ",\"status\":\""+status+"\"}");
    }

    void handleLogs() {
      if (!isLoggedIn()) { server->send(403); return; }
      if (!SPIFFS.exists("/logs.txt")) {
        server->send(200,"text/plain","No logs");
        return;
      }
      File f = SPIFFS.open("/logs.txt","r");
      server->send(200,"text/plain", f.readString());
      f.close();
    }
};
#endif
