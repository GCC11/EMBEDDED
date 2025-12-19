#ifndef COMM_H
#define COMM_H

#include <WiFi.h>
#include <WebServer.h>
#include "Sensors.h"
#include "SPIFFS.h"
#include "esp_sleep.h"
// Recommended light levels (lux)
#define LIGHT_READING_MIN 300
#define LIGHT_READING_MAX 750
#define LIGHT_TV_MIN 50
#define LIGHT_TV_MAX 150
#define LIGHT_SLEEP_MAX 10

class Comm {
  private:
    WebServer* server;
    Sensors* sensors;
    int bmePin;
    int bhPin;

    String username = "admin";
    String password = "1234";

    unsigned long lastLog = 0;
    const unsigned long LOG_INTERVAL = 600000;

    bool isLoggedIn() {
      if (!server->hasHeader("Cookie")) return false;
      return server->header("Cookie").indexOf("ESPSESSION=1") >= 0;
    }

    String loginPage() {
      return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>Login</title>
<style>
body{font-family:Arial;background:#f2f2f2}
.box{width:300px;margin:120px auto;padding:20px;background:#fff;border-radius:8px}
input,button{width:100%;padding:8px;margin-top:10px;box-sizing:border-box;border-radius:4px;}
@media (max-width:600px){
  .box{
    width:90%;
    margin:40px auto;
    padding:15px;
  }
}
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

    void logData() {
      sensors->read();
      File f = SPIFFS.open("/logs.txt", FILE_APPEND);
      if(!f) return;
      String line = String(millis()/1000) + "s, " +
                    "T=" + String(sensors->getTemp(),2) + "C, " +
                    "H=" + String(sensors->getHum(),2) + "%, " +
                    "P=" + String(sensors->getPres(),2) + "hPa, " +
                    "L=" + String(sensors->getLux(),2) + "lx\n";
      f.print(line);
      f.close();
    }

    String getRecommendations() {
      float t = sensors->getTemp();
      float h = sensors->getHum();
      float l = sensors->getLux();

      String r = "";

      if (t > 32) r += "<span style='color:red'>● High temperature: improve ventilation</span><br>";
      else if (t < 18) r += "<span style='color:orange'>● Low temperature: consider heating</span><br>";
      else r += "<span style='color:green'>● Temperature normal</span><br>";

      if (h > 75) r += "<span style='color:red'>● High humidity: risk of condensation</span><br>";
      else if (h < 30) r += "<span style='color:orange'>● Low humidity: air too dry</span><br>";
      else r += "<span style='color:green'>● Humidity normal</span><br>";

      if (l >= LIGHT_READING_MIN && l <= LIGHT_READING_MAX) {
        r += "<span style='color:green'>● Light suitable for reading/studying</span><br>";
  } 
      else if (l >= LIGHT_TV_MIN && l <= LIGHT_TV_MAX) {
        r += "<span style='color:green'>● Light suitable for watching TV/screens</span><br>";
  } 
      else if (l <= LIGHT_SLEEP_MAX) {
        r += "<span style='color:green'>● Light suitable for sleeping</span><br>";
  } 
      else if (l < LIGHT_READING_MIN && l > LIGHT_TV_MAX) {
        r += "<span style='color:orange'>● Light is low for reading, high for sleeping</span><br>";
  } 
      else if (l > LIGHT_READING_MAX) {
        r += "<span style='color:orange'>● Light is too bright for reading</span><br>";
  } 
      else {
        r += "<span style='color:orange'>● Light level outside recommended ranges</span><br>";
  }


    return r;
  }

  public:
    Comm(WebServer* s, Sensors* sens, int bmePower, int bhPower)
      : server(s), sensors(sens), bmePin(bmePower), bhPin(bhPower) {}

    void begin() {
      const char* keys[] = {"Cookie"};
      server->collectHeaders(keys, 1);

      server->on("/", [this](){ handleRoot(); });
      server->on("/login", HTTP_POST, [this](){ handleLogin(); });
      server->on("/logout", [this](){ handleLogout(); });
      server->on("/data", [this](){ handleData(); });
      server->on("/logs", [this](){ handleLogs(); });
      server->on("/sleep", [this](){ handleSleep(); });
      server->on("/actions", [this](){ handleActions(); });
      

      server->begin();
    }

    void handleRoot() {
      if (!isLoggedIn()) {
        server->send(200, "text/html", loginPage());
        return;
      }

      String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>ESP32 Sensor Dashboard</title>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
body{font-family:Arial}
.mode{display:inline-block;padding:15px 25px;margin:5px;border-radius:8px;cursor:pointer;font-weight:bold;background:#ddd}
.active{background:#4CAF50;color:white}
.section{margin-top:15px;padding:10px;border:1px solid #ccc;border-radius:8px}
@media (max-width:600px){
  body{
    font-size:18px;
  }

  h2{
    font-size:24px;
  }

  .mode{
    display:block;
    width:100%;
    margin:8px 0;
    padding:18px;
    font-size:18px;
    text-align:center;
  }

  .section{
    padding:20px;
    font-size:18px;
  }

  #data{
    font-size:20px;
  }

  button{
    width:100%;
    padding:16px;
    font-size:18px;
  }
}
/* 📱 PHONE PORTRAIT ONLY */
@media (max-width:600px) and (orientation:portrait){

  body{
    margin:8px;
  }

  h2{
    text-align:center;
  }

  /* stack mode buttons vertically */
  .mode{
    display:block;
    width:100%;
    margin:10px 0;
    padding:20px;
    font-size:20px;
  }

  /* make cards look like mobile cards */
  .section{
    padding:18px;
    border-radius:12px;
  }

  /* live data bigger and clearer */
  #data{
    font-size:22px;
    line-height:1.8;
  }

  /* links & buttons full-width */
  a, button{
    display:block;
    width:100%;
    text-align:center;
    margin-top:12px;
    font-size:18px;
  }
}

</style>
</head>
<body>

<h2>HOME COMFORTABILITY SENSOR</h2>
<a href="/logout">Logout</a>

<div>
  <div class="mode active" id="autoBtn" onclick="setMode('auto')">AUTO</div>
  <div class="mode" id="maintBtn" onclick="setMode('maint')">MAINTENANCE</div>
  <div class="mode" id="sleepBtn" onclick="setMode('sleep')">SLEEP</div>
</div>

<div id="auto" class="section">
  <h3>Live Data</h3>
  <div id="data">Loading...</div>

  <hr>

  <h3>Recommended Actions</h3>
  <div id="actions">Loading...</div>

  <br>
  <a href="/logs" target="_blank">View Logs</a> |
  <button onclick="downloadPDF()">Download Logs as PDF</button>
</div>

<div id="maint" class="section" style="display:none">
  <p>Maintenance mode</p>
</div>

<div id="sleep" class="section" style="display:none">
  <h3>Sleep Mode</h3>
  <p>Sensors powered off.</p>
</div>

<script src="https://cdnjs.cloudflare.com/ajax/libs/jspdf/2.5.1/jspdf.umd.min.js"></script>
<script>
let timer;

function setMode(m){
  ['auto','maint','sleep'].forEach(id=>{
    document.getElementById(id).style.display='none';
    document.getElementById(id+'Btn').classList.remove('active');
  });
  document.getElementById(m).style.display='block';
  document.getElementById(m+'Btn').classList.add('active');
  if(m==='auto'){start()} else {stop()}
  if(m==='sleep'){fetch('/sleep')}
}

function start(){
  update();
  timer=setInterval(update,1000);
}
function stop(){clearInterval(timer)}

async function update(){
  let j=await (await fetch('/data')).json();
  document.getElementById('data').innerHTML=
    'Temp: '+j.temp+' °C<br>'+
    'Hum: '+j.hum+' %<br>'+
    'Pressure: '+j.pres+' hPa<br>'+
    'Light: '+j.lux+' lx';
  loadActions();
}

async function loadActions(){
  let t = await (await fetch('/actions')).text();
  document.getElementById('actions').innerHTML = t;
}

async function downloadPDF(){
  let t=await (await fetch('/logs')).text();
  const {jsPDF}=window.jspdf;
  const d=new jsPDF();
  let y=10;
  t.split('\n').forEach(l=>{
    d.text(l,10,y); y+=7;
    if(y>280){d.addPage();y=10}
  });
  d.save('logs.pdf');
}

start();
</script>

</body>
</html>
)rawliteral";

      server->send(200,"text/html",html);
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
      if(!isLoggedIn()){ server->send(403); return; }
      sensors->read();
      server->send(200,"application/json",
        "{\"temp\":"+String(sensors->getTemp(),2)+
        ",\"hum\":"+String(sensors->getHum(),2)+
        ",\"pres\":"+String(sensors->getPres(),2)+
        ",\"lux\":"+String(sensors->getLux(),2)+"}");
    }

    void handleLogs() {
      if(!isLoggedIn()){ server->send(403); return; }
      if(!SPIFFS.exists("/logs.txt")){
        server->send(200,"text/plain","No logs yet."); return;
      }
      File f=SPIFFS.open("/logs.txt","r");
      server->send(200,"text/plain",f.readString());
      f.close();
    }

    void handleActions() {
      if(!isLoggedIn()){ server->send(403); return; }
      sensors->read();
      server->send(200,"text/html", getRecommendations());
    }

    void handleSleep() {
      if(!isLoggedIn()){ server->send(403); return; }
      sensors->power(false,bmePin,bhPin);
      esp_sleep_enable_ext0_wakeup(GPIO_NUM_0,0);
      esp_deep_sleep_start();
    }

    void loop() {
      server->handleClient();
      if(millis() - lastLog >= LOG_INTERVAL) {
        lastLog = millis();
        logData();
      }
    }
};

#endif
