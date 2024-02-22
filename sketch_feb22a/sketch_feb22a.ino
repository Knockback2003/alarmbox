#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <NTPClient.h>
#include <PubSubClient.h>
#include <FS.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <Discord_WebHook.h>
#include <Arduino.h>
#include <U8g2lib.h>

// INTERNAL TIMING
unsigned long apStartTime = 0;
unsigned long startTime = 0;
const unsigned long modeSwitchInterval = 600000;
bool inAPMode = false;
bool dualModeActive = false;
bool nightMode = false;
unsigned long currentTime = millis();

// RUNTIME
unsigned long previousMillis_runtime = 0;
const long interval_runtime_msg = 3600000;
unsigned long apTimeRemaining;
unsigned int apMinutesRemaining;

// Setup WLAN
unsigned long previousMillis_setupwlan = 0;
const long interval_setupwlan = 1000;

// LOOP LED
unsigned long previousMillis_led = 0;
const long interval_led = 500;

// LOOP LED AP
unsigned long previousMillis_ledAP = 0;
const long interval_ledAP = 200;

// MQTT
unsigned long previousMillis_callbackled = 0;
const long interval_callbackled = 10000;

// Status LED's / Input Port
const int greenLedPin = 14;
const int redLedPin = 12;
const int blueLedPin = 0;
const int inputPort = 2;

// Declaration
WiFiClient espClient1;
WiFiClient espClient2;
PubSubClient client(espClient1);
PubSubClient client2(espClient2);

// Variables Connections
bool wasWlanConnected = false;
bool isWlanConnected = false;
const char* wlanssid; 
const char* wlanpass;

// Discord
// String DiscordMSG;
// bool block;
// bool discord_connected = false;
// String DISCORD_WEBHOOK;
// Discord_Webhook discord;

// NTP-Config
const char* ntpServerName = "0.de.pool.ntp.org";
WiFiUDP udp;
NTPClient timeClient(udp, ntpServerName);
unsigned long epochTime; // Deklariere die Variable hier, um sie global verfügbar zu machen
const long utcOffset = 1;  // Offset in Sekunden (1 Stunde in diesem Beispiel)
TimeChangeRule myDST = {"CEST", Last, Sun, Mar, 2, 120};  // Sommerzeitregel
TimeChangeRule mySTD = {"CET", Last, Sun, Oct, 3, 60};   // Winterzeitregel
Timezone myTZ(myDST, mySTD);
time_t utc;
time_t local;
bool parseTime(const String& timeStr, int& hour, int& minute);

// Webserver
ESP8266WebServer server(80);
bool APClientConnected;
int randomNumber = 0;
String validToken ="";

// MQTT Variables
bool alarm = false;
bool response = false;
bool mqttConnected = false;
unsigned long lastMqttReconnectAttempt = 0;
const unsigned long mqttReconnectInterval = 5000;
bool mqttConnectedState = false;
bool mqttConnected2State = false;
bool wasMqttConnected = false;
bool wasMqttConnected2 = false;
bool was_alarm = false;

// MQTT 1
const char* mqttServer1;
int mqttPort1 = 0;
const char* mqttTopic1;
const char* mqttResponseTopic1;
const int publishIntervall = 15000;
unsigned long lastPublishTime = 0;

// MQTT 2
const char* mqttServer2;
int mqttPort2 = 0;
const char* mqttTopic2;
const int publishIntervall2 = 15000;
unsigned long lastPublishTime2 = 0;
const char* mqttUsername2;
const char* mqttPassword2;
bool response2 = false;
bool mqttConnected2 = false;
unsigned long lastMqttReconnectAttempt2 = 0;
const unsigned long mqttReconnectInterval2 = 5000;
static bool mqtt1Configured = false;
static bool mqtt2Configured = false;

// IFTTT
unsigned long previousMillis_ifttt = 0;
const long interval_ifttt = 30000;

// LED
bool ledState = false;

// Config
const size_t maxConfigSize = 1024;

// Display
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
unsigned long previousMillis_display = 0;
const long interval_display = 5000;
bool showKey = false;
unsigned long lastChangeTime = 0;
const unsigned long screenOffDelay = 600000;  // 10 Minuten in Millisekunden
bool display_start = false;


class ConfigData {
public:  
  String ssid;
  String password;
  String discordWebhook;
  bool enableDiscord;
  String mqttServer1;
  int mqttPort1;
  String mqttTopic1;
  String mqttResponse1;
  bool enableMqtt1;
  String mqttServer2;
  int mqttPort2;
  String mqttTopic2;
  String mqttUsername2;
  String mqttPassword2;
  bool enableMqtt2;
  String iftttKey;
  String iftttEvent;
  bool enableIfttt;
  bool nightModeEnabled;
  int nightModeStartHour;
  int nightModeStartMinute;
  int nightModeEndHour;
  int nightModeEndMinute;
  String apSSID;
  String apPassword;

  ConfigData() {
    ssid = "";
    password = "";
    enableMqtt1 = false;
    mqttServer1 = "";
    mqttPort1 = 0;
    mqttTopic1 = "";
    mqttResponse1 = "";
    enableMqtt2 = false;
    mqttServer2 = "";
    mqttPort2 = 0;
    mqttTopic2 = "";
    mqttUsername2 = "";
    mqttPassword2 = "";
    enableIfttt = false;
    iftttKey = "";
    iftttEvent = "";
    enableDiscord = false;
    discordWebhook = "";
    nightModeEnabled = false;
    nightModeStartHour = 0;
    nightModeStartMinute = 0;
    nightModeEndHour = 0;
    nightModeEndMinute = 0;
    apSSID = "Alarmbox";
    apPassword = "12345678";
  }

      bool operator!=(const ConfigData& other) const {
    // Vergleichen Sie die Mitglieder der Konfigurationsdaten mit den entsprechenden Mitgliedern der anderen Instanz (other).
    // Wenn mindestens ein Mitglied unterschiedlich ist, geben Sie true zurück, andernfalls false.

    return (ssid != other.ssid ||
            password != other.password ||
            enableMqtt1 != other.enableMqtt1 ||
            mqttServer1 != other.mqttServer1 ||
            mqttPort1 != other.mqttPort1 ||
            mqttTopic1 != other.mqttTopic1 ||
            mqttResponse1 != other.mqttResponse1 ||
            enableMqtt2 != other.enableMqtt2 ||
            mqttServer2 != other.mqttServer2 ||
            mqttPort2 != other.mqttPort2 ||
            mqttTopic2 != other.mqttTopic2 ||
            mqttUsername2 != other.mqttUsername2 ||
            mqttPassword2 != other.mqttPassword2 ||
            enableIfttt != other.enableIfttt ||
            iftttKey != other.iftttKey ||
            iftttEvent != other.iftttEvent ||
            enableDiscord != other.enableDiscord ||
            discordWebhook != other.discordWebhook ||
            nightModeEnabled != other.nightModeEnabled ||
            nightModeStartHour != other.nightModeStartHour ||
            nightModeStartMinute != other.nightModeStartMinute ||
            nightModeEndHour != other.nightModeEndHour ||
            nightModeEndMinute != other.nightModeEndMinute ||
            apSSID != other.apSSID ||
            apPassword != other.apPassword);
        }
};

ConfigData config;
ConfigData previousConfig;

String epochToTime(time_t epochTime) {
  char buf[20];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
           year(epochTime), month(epochTime), day(epochTime),
           hour(epochTime), minute(epochTime), second(epochTime));
  return String(buf);
}


void handleSaveToken() {
    if (server.hasArg("token")) {
        validToken = server.arg("token");
        server.send(200, "text/plain", "Token gespeichert!");
    } else {
        server.send(400, "text/plain", "Token nicht gefunden");
    }
}

String generateToken() {
const String charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const size_t tokenLength = 20; // Die Länge des Tokens

    String token;
    for (size_t i = 0; i < tokenLength; ++i) {
        token += charset[random(0, charset.length())];
    }

    validToken = token;
    return token;
}

void handleCheckKey() {
    if (server.hasArg("key")) {
        String keyToCheck = server.arg("key");
        
        if (keyToCheck.toInt() == randomNumber) {
            String validToken = generateToken();
            server.send(200, "text/plain", validToken);
        } else {
            server.send(401, "text/plain", "Invalid");
        }
    } else {
        server.send(400, "text/plain", "Bad Request");
    }
}

void handleLogin() {
    String html = "<!DOCTYPE html>";
    html += "<html>";
    html += "<head>";
    html +=     "<title>Login</title>";
    html += "</head>";
    html += "<body>";
    html +=     "<h2>Login</h2>";
    html +=     "<form id=\"loginForm\">";
    html +=         "<label for=\"key\">Key:</label>";
    html +=         "<input type=\"text\" id=\"key\" name=\"key\">";
    html +=         "<input type=\"submit\" value=\"Login\">";
    html +=     "</form>";

    html +=     "<script>";
    html +=         "document.getElementById('loginForm').addEventListener('submit', function(event) {";
    html +=             "event.preventDefault();";

    html +=             "var key = document.getElementById('key').value;";

    html +=             "var xhr = new XMLHttpRequest();";
    html +=             "xhr.open('GET', '/checkKey?key=' + key, true);";
    html +=             "xhr.onreadystatechange = function() {";
    html +=                 "if (xhr.readyState == XMLHttpRequest.DONE) {";
    html +=                     "if (xhr.status == 200) {";
    html +=                         "var token = xhr.responseText;"; // Token von der Antwort erhalten

    // Local save token in browser
    html +=                         "localStorage.setItem('token', token);";

    // redirect to next page with token
    html +=                         "window.location.href = '/start?token=' + token;";
    html +=                     "} else {";
    html +=                         "alert('Falscher Key!');";
    html +=                     "}";
    html +=                 "}";
    html +=             "};";
    html +=             "xhr.send();";
    
    html +=         "});";
    html +=     "</script>";
    html += "</body>";
    html += "</html>";

    server.send(200, "text/html", html);
}


bool verifyToken() {
    String token = server.arg("token");
    if (token.equals(validToken)) {
        return true;
    } else {
        return false;
    }
}

void handleStart() {
   verifyToken();
 
 String html;
if (verifyToken()) {
  html += "<html><head><style>";
  html += "/* Versteckte Checkbox */";
  html += "#menu-toggle { display: none; }";
  html += "/* Symbol für das Menü */";
  html += "#menu-icon::before { content: '\\2630'; font-size: 82px; cursor: pointer; }"; // Größere Menü-Schaltfläche
  html += "/* Stil für das Menü */";
  html += "#menu { display: none; position: absolute; background-color: #333; color: #fff; padding: 50px; }";
  html += "#menu-toggle:checked ~ #menu { display: block; }";  // Diese Zeile sorgt dafür, dass das Menü angezeigt wird, wenn die Checkbox aktiviert ist

  html += "body {"
    "height: 3000px;"
    "background: linear-gradient(141deg, #0fb8ad 0%, #1fc8db 51%, #2cb5e8 75%);"
    "}";

  // CSS-Config for Mobile Devices
  html += "@media (max-width: 768px) {";
  html += "#menu {";
  html += "font-size: 62px; /* Größerer Text im Menü */";
  html += "}";

  // Menu Config
  html += "#menu a {";
  html += "display: block; /* Jeder Menüpunkt in einer neuen Zeile */";
  html += "margin: 80px 0; /* Abstand zwischen den Menüpunkten */";
  html += "font-size: 60px; /* Ändern Sie die Schriftgröße für die Menüpunkte */";
  html += "color: #fff; /* Textfarbe der Menüpunkte */";
  html += "}";
  html += "}";

  html += "</style></head><body>";

  // Hamburger-Checkbox and Menu Icon
  html += "<input type='checkbox' id='menu-toggle'>"; // Die versteckte Checkbox
  html += "<div id='menu-icon'></div>"; // Das Symbol für das Menü

  // Menu
  html += "<div id='menu'>";
  html += "<a href='/config?token=" + String(server.arg("token")) + "'>Konfiguration</a>"; // Hier fügen Sie Ihre Menüpunkte hinzu
  html += "<a href='/menu-item-2'>Menüpunkt 2</a>";
  html += "<a href='/menu-item-3'>Menüpunkt 3</a>";
  html += "</div>";

  html += "<script>";
  html += "function toggleMenu() {";
  html += "var menu = document.getElementById('menu');";
  html += "var menuToggle = document.getElementById('menu-toggle');";

  html += "if (menu.style.display === 'block') {";
  html += "menu.style.display = 'none';";
  html += "} else {";
  html += "menu.style.display = 'block';";
  html += "}";
  html += "}";

  // Event-Listener Menu Button
  html += "document.getElementById('menu-icon').addEventListener('click', toggleMenu);";
  html += "</script>";

  html += "</body></html>";

  server.send(200, "text/html", html);
 } else {
  server.send(401, "text/html", "Invalid Token");

 }
}


void handleConfig() {
  verifyToken();
  
  String html;
if (verifyToken()) {
  
  html += "<html><head><style>";
  html += "<meta http-equiv='refresh' content='5;url=/config'>";
  html += "body { font-family: Arial, sans-serif; }";
  html += "h1 { background-color: #333; color: #fff; padding: 10px; }";
  html += "form { padding: 20px; }";
  html += "input[type='text'], input[type='password'], input[type='time'] { width: 100%; padding: 5px; margin: 5px 0; }";
  html += "label { display: block; font-weight: bold; }";
  html += "body {"
          "height: 3000px;"
          "background: linear-gradient(141deg, #0fb8ad 0%, #1fc8db 51%, #2cb5e8 75%);"
          "}";
  html += "<meta charset='UTF-8'>";
  html += "</style></head><body>";
  html += "<h1>Alarmbox Konfiguration</h1>";
  html += "<p>Access Point bleibt f&uumlr " + String(apMinutesRemaining) + " Minuten ge&oumlffnet.</p>";
  html += "<form method='post' action='/save?token=" + String(server.arg("token")) + "'>";

  // WiFi
  html += "<label for='ssid'>WLAN SSID:</label><input type='text' name='ssid' value='" + config.ssid + "'><br>";
  html += "<label for='password'>WLAN Passwort:</label><input type='password' name='password' value='" + config.password + "'><br>";
  
  // AP SSID und AP Password
  html += "<label for='apSSID'>Alarmbox SSID:</label><input type='text' name='apSSID' value='" + config.apSSID + "'><br>";
  html += "<label for='apPassword'>Alarmbox Passwort:</label><input type='password' name='apPassword' value='" + config.apPassword + "'><br>";
  
  
  // MQTT Server 1 Checkbox and Fields
  html += "<label for='enableMqtt1'>MQTT Server 1 aktivieren:</label><input type='checkbox' name='enableMqtt1' ";
  html += config.enableMqtt1 ? "checked='checked'" : "";
  html += "><br>";

  html += "<label for='mqttServer1'>MQTT Server 1:</label><input type='text' name='mqttServer1' value='" + config.mqttServer1 + "' " + (config.enableMqtt1 ? "" : "disabled") + "><br>";
  html += "<label for='mqttPort1'>MQTT Port 1:</label><input type='text' name='mqttPort1' value='" + String(config.mqttPort1) + "' " + (config.enableMqtt1 ? "" : "disabled") + "><br>";
  html += "<label for='mqttTopic1'>MQTT Topic 1:</label><input type='text' name='mqttTopic1' value='" + config.mqttTopic1 + "' " + (config.enableMqtt1 ? "" : "disabled") + "><br>";
  html += "<label for='mqttResponse1'>MQTT Response 1:</label><input type='text' name='mqttResponse1' value='" + config.mqttResponse1 + "' " + (config.enableMqtt1 ? "" : "disabled") + "><br>";

  // MQTT Server 2 Checkbox and Fields
  html += "MQTT Server 2 aktivieren: <input type='checkbox' name='enableMqtt2' ";
  html += config.enableMqtt2 ? "checked='checked'" : "";
  html += "><br>";

  html += "MQTT Server 2: <input type='text' name='mqttServer2' value='" + config.mqttServer2 + "' " + (config.enableMqtt2 ? "" : "disabled") + "><br>";
  html += "MQTT Port 2: <input type='text' name='mqttPort2' value='" + String(config.mqttPort2) + "' " + (config.enableMqtt2 ? "" : "disabled") + "><br>";
  html += "MQTT Topic 2: <input type='text' name='mqttTopic2' value='" + config.mqttTopic2 + "' " + (config.enableMqtt2 ? "" : "disabled") + "><br>";
  html += "MQTT Username 2: <input type='text' name='mqttUsername2' value='" + config.mqttUsername2 + "' " + (config.enableMqtt2 ? "" : "disabled") + "><br>";
  html += "<label for='password'>MQTT Passwort 2:</label><input type='password' name='mqttPassword2' value='" + config.mqttPassword2 + "' " + (config.enableMqtt2 ? "" : "disabled") + "><br>";

  // Discord Checkbox and Fields
  html += "Discord Webhook aktivieren: <input type='checkbox' name='enableDiscord' ";
  html += config.enableDiscord ? "checked='checked'" : "";
  html += "><br>";

  html += "Discord Webhook: <input type='text' name='discordWebhook' value='" + config.discordWebhook + "' " + (config.enableDiscord ? "" : "disabled") + "><br>";

  // IFTTT Checkbox and Fields
  html += "IFTTT Webhook aktivieren: <input type='checkbox' name='enableIfttt' ";
  html += config.enableIfttt ? "checked='checked'" : "";
  html += "><br>";

  html += "IFTTT Key: <input type='text' name='iftttKey' value='" + config.iftttKey + "' " + (config.enableIfttt ? "" : "disabled") + "><br>";
  html += "IFTTT Event: <input type='text' name='iftttEvent' value='" + config.iftttEvent + "' " + (config.enableIfttt ? "" : "disabled") + "><br>";

  // Nighttime Checkbox and Fields
  html += "Nachtmodus aktivieren: <input type='checkbox' name='nightModeEnabled' ";
  html += config.nightModeEnabled ? "checked='checked'" : "";
  html += "><br>";

  // Nighttime Start
  html += "Nachtmodus Startzeit: <input type='time' name='nightModeStart' value='" + 
        String(config.nightModeStartHour < 10 ? "0" : "") + String(config.nightModeStartHour) + ":" +
        String(config.nightModeStartMinute < 10 ? "0" : "") + String(config.nightModeStartMinute) +
        "' " + (config.nightModeEnabled ? "" : "disabled") + "><br>";

  // Nighttime End
  html += "Nachtmodus Endzeit: <input type='time' name='nightModeEnd' value='" + 
        String(config.nightModeEndHour < 10 ? "0" : "") + String(config.nightModeEndHour) + ":" +
        String(config.nightModeEndMinute < 10 ? "0" : "") + String(config.nightModeEndMinute) +
        "' " + (config.nightModeEnabled ? "" : "disabled") + "><br>";


  // Save-Button
  html += "<input type='submit' value='Speichern'>";
  html += "</form>";

  // Reset Button
  html += "<form method='post' action='/reset?token=" + String(server.arg("token")) + "'>";
  html += "<input type='submit' value='Zur&uuml;cksetzen'>";
  html += "</form>";

  // Nighttime-Button
  html += "<label for='nightModeToggle'>Nachtmodus aktivieren:</label>";
  html += "<input type='checkbox' name='nightModeToggle' id='nightModeToggle' ";
  html += "><br>";

  // JavaScript toggle Nighttime
  html += "<script>";
  html += "var nightModeToggle = document.getElementById('nightModeToggle');";

  // Function activate / deactivate Nighttime
  html += "nightModeToggle.onclick = function() { toggleNightMode(); };";

  // CSS for Darkmode on
  html += "var nightModeCSS = document.createElement('style');";
  html += "nightModeCSS.innerHTML = '";
  html += "  body.night-mode {";
  html += "    background-color: #000;";
  html += "    color: #fff;";
  html += "  }";
  html += "';";
  html += "document.head.appendChild(nightModeCSS);";

  // JavaScript toggle Darkmode
  html += "function toggleNightMode() {";
  html += "  var isChecked = nightModeToggle.checked;";
  html += "  if (isChecked) {";
  html += "    document.body.classList.add('night-mode');";
  html += "  } else {";
  html += "    document.body.classList.remove('night-mode');";
  html += "  }";
  html += "}";

  html += "</script>";

  // CSS for Darkmode off
  html += "<script>";
  html += "var nightModeCSS = document.createElement('style');";
  html += "nightModeCSS.innerHTML = '";
  html += "  body.night-mode {";
  html += "    background-color: #171719;";
  html += "    color: #fff;";
  html += "  }";
  html += "';";
  html += "document.head.appendChild(nightModeCSS);";

  // JavaScript-Funktion for Darkmode
  html += "function toggleNightMode() {";
  html += "  var isChecked = nightModeToggle.checked;";
  html += "  if (isChecked) {";
  html += "    document.body.classList.add('night-mode');";
  html += "  } else {";
  html += "    document.body.classList.remove('night-mode');";
  html += "  }";
  html += "}";
  html += "</script>";
  
  // JavaScript for toggle input fields
  html += "<script>";
  html += "var mqtt1_enabled = document.getElementsByName('enableMqtt1')[0];";
  html += "var mqtt2_enabled = document.getElementsByName('enableMqtt2')[0];";
  html += "var discord_enabled = document.getElementsByName('enableDiscord')[0];";
  html += "var ifttt_enabled = document.getElementsByName('enableIfttt')[0];";
  html += "var nightMode_enabled = document.getElementsByName('nightModeEnabled')[0];";
  
  html += "mqtt1_enabled.onclick = function() { toggleDisabled('mqttServer1', 'mqttPort1', 'mqttTopic1', 'mqttResponse1'); };";
  html += "mqtt2_enabled.onclick = function() { toggleDisabled('mqttServer2', 'mqttPort2', 'mqttTopic2', 'mqttUsername2', 'mqttPassword2'); };";
  html += "discord_enabled.onclick = function() { toggleDisabled('discordWebhook'); };";
  html += "ifttt_enabled.onclick = function() { toggleDisabled('iftttKey', 'iftttEvent'); };";
  html += "nightMode_enabled.onclick = function() { toggleDisabled('nightModeStart', 'nightModeEnd'); };";
  
  html += "function toggleDisabled(checkbox, ...fields) {";
  html += "  var isChecked = checkbox.checked;";
  html += "  for (var i = 0; i < fields.length; i++) {";
  html += "    var input = document.getElementsByName(fields[i])[0];";
  html += "    if (input) {";
  html += "      input.disabled = !isChecked;";
  html += "    }";
  html += "  }";
  html += "}";

  html += "mqtt1_enabled.onclick = function() { toggleDisabled(mqtt1_enabled, 'mqttServer1', 'mqttPort1', 'mqttTopic1', 'mqttResponse1'); };";
  html += "mqtt2_enabled.onclick = function() { toggleDisabled(mqtt2_enabled, 'mqttServer2', 'mqttPort2', 'mqttTopic2', 'mqttUsername2', 'mqttPassword2'); };";
  html += "discord_enabled.onclick = function() { toggleDisabled(discord_enabled, 'discordWebhook'); };";
  html += "ifttt_enabled.onclick = function() { toggleDisabled(ifttt_enabled, 'iftttKey', 'iftttEvent'); };";
  html += "nightMode_enabled.onclick = function() { toggleDisabled(nightMode_enabled, 'nightModeStart', 'nightModeEnd'); };";
  
  html += "</script>";
  
  html += "</body></html>";

  server.send(200, "text/html", html);
} else {
  server.send(401, "text/html", "Invalid Token");
}

}


void loadConfigFromFile() {
  if (SPIFFS.begin()) {
    File configFile = SPIFFS.open("/config.json", "r");
    if (!configFile) {
      Serial.println("Konnte Konfigurationsdatei nicht öffnen.");
      return;
    }

    DynamicJsonDocument jsonDoc(maxConfigSize);
    DeserializationError error = deserializeJson(jsonDoc, configFile);
    configFile.close();

    if (!error) {
      config.ssid = jsonDoc["ssid"].as<String>();
      config.password = jsonDoc["password"].as<String>();
      config.enableMqtt1 = jsonDoc["enableMqtt1"].as<bool>();
      config.mqttServer1 = jsonDoc["mqttServer1"].as<String>();
      config.mqttPort1 = jsonDoc["mqttPort1"].as<int>();
      config.mqttTopic1 = jsonDoc["mqttTopic1"].as<String>();
      config.mqttResponse1 = jsonDoc["mqttResponse1"].as<String>();
      config.enableMqtt2 = jsonDoc["enableMqtt2"].as<bool>();
      config.mqttServer2 = jsonDoc["mqttServer2"].as<String>();
      config.mqttPort2 = jsonDoc["mqttPort2"].as<int>();
      config.mqttTopic2 = jsonDoc["mqttTopic2"].as<String>();
      config.mqttUsername2 = jsonDoc["mqttUsername2"].as<String>();
      config.mqttPassword2 = jsonDoc["mqttPassword2"].as<String>();
      config.enableIfttt = jsonDoc["enableIfttt"].as<bool>();
      config.iftttKey = jsonDoc["iftttKey"].as<String>();
      config.iftttEvent = jsonDoc["iftttEvent"].as<String>();
      config.enableDiscord = jsonDoc["enableDiscord"].as<bool>();
      config.discordWebhook = jsonDoc["discordWebhook"].as<String>();
      config.nightModeEnabled = jsonDoc["nightModeEnabled"].as<bool>();
      config.nightModeStartHour = jsonDoc["nightModeStartHour"].as<int>();
      config.nightModeStartMinute = jsonDoc["nightModeStartMinute"].as<int>();
      config.nightModeEndHour = jsonDoc["nightModeEndHour"].as<int>();
      config.nightModeEndMinute = jsonDoc["nightModeEndMinute"].as<int>();
      config.apSSID = jsonDoc["apSSID"].as<String>();
      config.apPassword = jsonDoc["apPassword"].as<String>();
    }
  } else {
    Serial.println("Fehler beim Initialisieren des Dateisystems (SPIFFS).");
  }

    Serial.println("[loadConfigFromFile] Nach dem Laden der Konfigurationsdaten:");
    Serial.print("[loadConfigFromFile] Nachtmodus Startzeit: ");
    Serial.println(config.nightModeStartHour + ":" + config.nightModeStartMinute);

}

void saveConfigToFile() {

   Serial.println("[saveConfigToFile] Vor dem Speichern der Konfigurationsdaten:");
   Serial.print("[saveConfigToFile] Nachtmodus Startzeit: ");
   Serial.println(config.nightModeStartHour + ":" + config.nightModeStartMinute);



  if (SPIFFS.begin()) {
    DynamicJsonDocument jsonDoc(maxConfigSize);
    jsonDoc["ssid"] = config.ssid;
    jsonDoc["password"] = config.password;
    jsonDoc["enableMqtt1"] = config.enableMqtt1;
    jsonDoc["mqttServer1"] = config.mqttServer1;
    jsonDoc["mqttPort1"] = config.mqttPort1;
    jsonDoc["mqttTopic1"] = config.mqttTopic1;
    jsonDoc["mqttResponse1"] = config.mqttResponse1;
    jsonDoc["enableMqtt2"] = config.enableMqtt2;
    jsonDoc["mqttServer2"] = config.mqttServer2;
    jsonDoc["mqttPort2"] = config.mqttPort2;
    jsonDoc["mqttTopic2"] = config.mqttTopic2;
    jsonDoc["mqttUsername2"] = config.mqttUsername2;
    jsonDoc["mqttPassword2"] = config.mqttPassword2;
    jsonDoc["enableIfttt"] = config.enableIfttt;
    jsonDoc["iftttKey"] = config.iftttKey;
    jsonDoc["iftttEvent"] = config.iftttEvent;
    jsonDoc["enableDiscord"] = config.enableDiscord;
    jsonDoc["discordWebhook"] = config.discordWebhook;
    jsonDoc["nightModeEnabled"] = config.nightModeEnabled;
    jsonDoc["nightModeStartHour"] = config.nightModeStartHour;
    jsonDoc["nightModeStartMinute"] = config.nightModeStartMinute;
    jsonDoc["nightModeEndHour"] = config.nightModeEndHour;
    jsonDoc["nightModeEndMinute"] = config.nightModeEndMinute;
    jsonDoc["apSSID"] = config.apSSID;
    jsonDoc["apPassword"] = config.apPassword;

    File configFile = SPIFFS.open("/config.json", "w");
    if (configFile) {
      if (serializeJson(jsonDoc, configFile) == 0) {
        Serial.println("Fehler beim Schreiben der Konfigurationsdaten.");
      }
      configFile.close();
      loadConfigFromFile();
    } else {
      Serial.println("Fehler beim Öffnen der Konfigurationsdatei zum Schreiben.");
    }
  } else {
    Serial.println("Fehler beim Initialisieren des Dateisystems (SPIFFS).");
  }

}
void handleSave() {
  //Serial.println("[handleSAVE] start");
  if (server.args() > 0) {
    config.enableMqtt1 = false;
    config.enableMqtt2 = false;
    config.enableDiscord = false;
    config.enableIfttt = false;
    config.nightModeEnabled = false;

    for (int i = 0; i < server.args(); i++) {
      if (server.argName(i) == "ssid") {
        config.ssid = server.arg(i);
      } else if (server.argName(i) == "password") {
        config.password = server.arg(i);
      } else if (server.argName(i) == "nightModeEnabled") {
        config.nightModeEnabled = server.arg(i).equals("on");
      } else if (server.argName(i) == "apSSID") {
        config.apSSID = server.arg(i);
      } else if (server.argName(i) == "apPassword") {
        config.apPassword = server.arg(i);
      }

      if (server.argName(i) == "enableMqtt1") {
        config.enableMqtt1 = server.arg(i).equals("on");
      } else if (server.argName(i) == "mqttServer1") {
        config.mqttServer1 = server.arg(i).c_str();
      } else if (server.argName(i) == "mqttPort1") {
        config.mqttPort1 = server.arg(i).toInt();
      } else if (server.argName(i) == "mqttTopic1") {
        config.mqttTopic1 = server.arg(i).c_str();
      } else if (server.argName(i) == "mqttResponse1") {
        config.mqttResponse1 = server.arg(i).c_str();
      }

      if (server.argName(i) == "enableMqtt2") {
        config.enableMqtt2 = server.arg(i).equals("on");
      } else if (server.argName(i) == "mqttServer2") {
        config.mqttServer2 = server.arg(i).c_str();
      } else if (server.argName(i) == "mqttPort2") {
        config.mqttPort2 = server.arg(i).toInt();
      } else if (server.argName(i) == "mqttTopic2") {
        config.mqttTopic2 = server.arg(i).c_str();
      } else if (server.argName(i) == "mqttUsername2") {
        config.mqttUsername2 = server.arg(i).c_str();
      } else if (server.argName(i) == "mqttPassword2") {
        config.mqttPassword2 = server.arg(i).c_str();
      }

      if (server.argName(i) == "enableDiscord") {
        config.enableDiscord = server.arg(i).equals("on");
      } else if (server.argName(i) == "discordWebhook") {
        config.discordWebhook = server.arg(i);
      }

      if (server.argName(i) == "enableIfttt") {
        config.enableIfttt = server.arg(i).equals("on");
      } else if (server.argName(i) == "iftttKey") {
        config.iftttKey = server.arg(i);
      } else if (server.argName(i) == "iftttEvent") {
        config.iftttEvent = server.arg(i);
      }
    
      if (config.nightModeEnabled) {
        if (server.argName(i) == "nightModeStart") {
          String timeStr = server.arg(i);
          int colonIndex = timeStr.indexOf(':');
            if (colonIndex != -1) {
                if (timeStr.length() == 5) {
                  config.nightModeStartHour = timeStr.substring(0, colonIndex).toInt();
                    config.nightModeStartMinute = timeStr.substring(colonIndex + 1).toInt();
                  }
              }
        } else if (server.argName(i) == "nightModeEnd") {
          String timeStr = server.arg(i);
            int colonIndex = timeStr.indexOf(':');
              if (colonIndex != -1) {
                  if (timeStr.length() == 5) {
                    config.nightModeEndHour = timeStr.substring(0, colonIndex).toInt();
                      config.nightModeEndMinute = timeStr.substring(colonIndex + 1).toInt();
                  }
               }
            }
          }
        }
    //Serial.println("[handleSAVE] erfolgreich");
    // Speichern Sie die Konfigurationsdaten in einer Datei
    //Serial.println("[handleSAVE] Start saveConfigFile");
    saveConfigToFile();

    // Redirect User to succes page
    server.sendHeader("Location", "/saved?token=" + String(server.arg("token")));
    server.send(302, "text/plain", "");
  }
}

void handleSaved() {
  verifyToken();
  // Load Config from File
  loadConfigFromFile();
  String html;

  if (verifyToken()) {
  
  html += "<html>";
  html += "<head>";
  // JavaScript to block backkey of browser
  html += "<script>";
  html += "window.addEventListener('popstate', function(event) {";
  html += "  window.history.pushState(null, document.title, window.location.href);";
  html += "});";

  html += "function goToConfigPage() {";
  html += "  window.location.href = '/config?token="+ String(server.arg("token")) + "';";
  html += "}";
  html += "</script>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; }";
  html += "h1 { background-color: #333; color: #fff; padding: 10px; }";
  html += "p { font-size: 18px; }";
  html += "button { font-size: 20px; padding: 10px 20px; background-color: #0073e6; color: #fff; border: none; }";
  
  html += "body {"
          "height: 3000px;"
          "background: linear-gradient(141deg, #0fb8ad 0%, #1fc8db 51%, #2cb5e8 75%);"
          "}";

  html += "</style></head><body>";
  html += "<h1>Konfiguration gespeichert</h1>";
  html += "<p>SSID: " + config.ssid + "</p>";
  html += "<p>Passwort: " + config.password + "</p>";
  html += "<p>ESP SSID: " + config.apSSID + "</p>";
  html += "<p>ESP Passwort " + config.apPassword + "</p>";
  html += "<p>MQTT 1 Aktiv: " + String(config.enableMqtt1) + "</p>";
  html += "<p>MQTT 1 Server: " + config.mqttServer1 + "</p>";
  html += "<p>MQTT 1 Port: " + String(config.mqttPort1) + "</p>";
  html += "<p>MQTT 1 Topic: " + config.mqttTopic1 + "</p>";
  html += "<p>MQTT 1 Response: " + config.mqttResponse1 + "</p>";
  html += "<p>MQTT 2 Aktiv: " + String(config.enableMqtt2) + "</p>";
  html += "<p>MQTT 2 Server: " + config.mqttServer2 + "</p>";
  html += "<p>MQTT 2 Port: " + String(config.mqttPort2) + "</p>";
  html += "<p>MQTT 2 Topic: " + config.mqttTopic2 + "</p>";
  html += "<p>MQTT 2 Username: " + config.mqttUsername2 + "</p>";
  html += "<p>MQTT 2 Password: " + config.mqttPassword2 + "</p>";
  html += "<p>IFTTT Aktiv: " + String(config.enableIfttt) + "</p>";
  html += "<p>IFTTT Key: " + config.iftttKey + "</p>";
  html += "<p>IFTTT Event: " + config.iftttEvent + "</p>";
  html += "<p>Discord Aktiv: " + String(config.enableDiscord) + "</p>";
  html += "<p>Discord Webhook: " + config.discordWebhook + "</p>";
  html += "<p>Nachtmodus Aktiv: " + String(config.nightModeEnabled) + "</p>";
  html += "<p>Nachtmodus Startzeit: " + String(config.nightModeStartHour < 10 ? "0" : "") + String(config.nightModeStartHour) + ":" + String(config.nightModeStartMinute < 10 ? "0" : "") + String(config.nightModeStartMinute) + "</p>";
  html += "<p>Nachtmodus Endzeit: " +  String(config.nightModeEndHour < 10 ? "0" : "") + String(config.nightModeEndHour) + ":" + String(config.nightModeEndMinute < 10 ? "0" : "") + String(config.nightModeEndMinute) + "</p>";

  html += "</body></html>";

  html += "<button onclick='goToConfigPage()'>Zur&uumlck zur Konfiguration</button>";

  //Serial.println("[handleSaved] erfolgreich");
  server.send(200, "text/html", html);
  } else {
  server.send(401, "text/html", "Invalid Token");
  }
}

void resetConfig() {
  config = ConfigData();
  saveConfigToFile();
}

void handleReset() {  
  verifyToken();
  
  if (server.method() == HTTP_POST) {
    resetConfig();
    
    // Redirect User to success Page
    server.sendHeader("Location", "/reset-done?token=" + String(server.arg("token")));
    server.send(302, "text/plain", "");
  }
}

void handleResetDone() {
  verifyToken();
  
  if (verifyToken()) {
  String html;
  html += "<html><body>";
  html += "<h1>Konfiguration zurückgesetzt</h1>";
  html += "<p>Die Konfiguration wurde erfolgreich zurückgesetzt.</p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
} else {
  server.send(401, "text/html", "Invalid Token");
  }
}

template <typename... Values>
void printIfEnabled(const char* name, bool enabled, const Values&... values) {
  Serial.print(name);
  Serial.print(" Enabled: ");
  Serial.println(enabled);
  if (enabled) {
    (Serial.print(name), ..., (Serial.print(" "), Serial.print(values)));
  }
}

void showConfig() {
  int startHour, startMinute, endHour, endMinute;

  Serial.println("Aktuelle Konfiguration:");
  Serial.print("SSID: ");
  Serial.println(config.ssid);
  Serial.print("Passwort: ");
  Serial.println(config.password);
  Serial.print("ESP SSID: ");
  Serial.println(config.apSSID);
  Serial.print("ESP Passwort: ");
  Serial.println(config.apPassword);
  printIfEnabled("MQTT1", config.enableMqtt1, config.mqttServer1, config.mqttPort1, config.mqttTopic1, config.mqttResponse1);
  printIfEnabled("MQTT2", config.enableMqtt2, config.mqttServer2, config.mqttPort2, config.mqttTopic2, config.mqttUsername2, config.mqttPassword2);
  printIfEnabled("Discord", config.enableDiscord, config.discordWebhook);
  printIfEnabled("IFTTT", config.enableIfttt, config.iftttKey, config.iftttEvent);
  if (config.nightModeEnabled) {
  String startHourStr = String(startHour);
  String startMinuteStr = String(startMinute);
  String endHourStr = String(endHour);
  String endMinuteStr = String(endMinute);

  printIfEnabled("NightMode", config.nightModeEnabled, startHourStr, startMinuteStr, endHourStr, endMinuteStr);
  }
}

bool parseTime(const String& timeStr, int& hour, int& minute) {
  // Check if the format is "HH:MM"
  if (timeStr.length() != 5 || timeStr[2] != ':') {
    return false;
  }

  // Extract Hours and Minutes
  int parsedHour = timeStr.substring(0, 2).toInt();
  int parsedMinute = timeStr.substring(3).toInt();

  // Validate extraction
  if (parsedHour >= 0 && parsedHour <= 23 && parsedMinute >= 0 && parsedMinute <= 59) {
    // Gültige Uhrzeit
    hour = parsedHour;
    minute = parsedMinute;
    return true;
  } else {
    return false;
  }
}

void connectToWiFi() {
  WiFi.begin(config.ssid, config.password);
  delay(4000);
}

void u8g2_prepare(void) {
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

void u8g2_draw(void) {
  currentTime = millis();
  u8g2_prepare();
  u8g2.clearBuffer();

  if (!display_start) {
    u8g2.firstPage();
    do {
      u8g2.drawRFrame(5, 10, 118, 44, 8);
      u8g2.setFont(u8g2_font_inb16_mf);
      int textWidth = u8g2.getStrWidth("Start");
      int textHeight = u8g2.getAscent();
      int frameWidth = 118;
      int frameHeight = 44;
      int textX = (frameWidth - textWidth) / 2 + 5;
      int textY = 6 + (frameHeight - textHeight) / 2 + textHeight / 2 - 1;
      u8g2.drawStr(textX, textY, "Start");
      display_start = true;
    } while (u8g2.nextPage());
  } else if (inAPMode || dualModeActive && !alarm) {
    if (currentTime - previousMillis_display >= interval_display) {
      previousMillis_display = currentTime;

      u8g2.firstPage();
      do {
        u8g2.drawRFrame(5, 10, 118, 44, 8); // Make Frame

        if (showKey) {
          u8g2.setFont(u8g2_font_helvR10_tf);
          int keyWidth = u8g2.getStrWidth("Key");
          int keyHeight = u8g2.getAscent();
          int frameWidth = 118;
          int frameHeight = 44;
          int keyX = (frameWidth - keyWidth) / 2 + 5;
          int keyY = 10 + frameHeight - keyHeight + 4;
          u8g2.drawStr(keyX, keyY, "Key");

          u8g2.setFont(u8g2_font_inb16_mf);
          String randomStr = String(randomNumber);
          int randomWidth = u8g2.getStrWidth(randomStr.c_str());
          int randomHeight = u8g2.getAscent();
          int randomX = (frameWidth - randomWidth) / 2 + 5;
          int randomY = 15 + (randomHeight - randomHeight) / 2 + randomHeight / 2 - 1;
          u8g2.drawStr(randomX, randomY, randomStr.c_str());
        } else {
          u8g2.setFont(u8g2_font_helvR10_tf);
          int ipWidth = u8g2.getStrWidth("IP");
          int ipHeight = u8g2.getAscent();
          int frameWidth = 118;
          int frameHeight = 44;
          int ipX = (frameWidth - ipWidth) / 2 + 5;
          int ipY = 10 + frameHeight - ipHeight + 4;
          u8g2.drawStr(ipX, ipY, "IP");

          u8g2.setFont(u8g2_font_helvR14_tf);
          String ipAddress = WiFi.softAPIP().toString();
          int iptxtWidth = u8g2.getStrWidth(ipAddress.c_str());
          int iptxtHeight = u8g2.getAscent();
          int ipframeWidth = 118;
          int ipframeHeight = 44;
          int iptxtX = (ipframeWidth - iptxtWidth) / 2 + 5;
          int iptxtY = 4 + (ipframeHeight - iptxtHeight) / 2 + iptxtHeight / 2 - 1;
          u8g2.drawStr(iptxtX, iptxtY, ipAddress.c_str());
        }

        showKey = !showKey;
      } while (u8g2.nextPage());

      u8g2.sendBuffer(); // act. display
    }

  } else if (alarm) {
    u8g2.firstPage();
    do {
      u8g2.drawRFrame(5, 10, 118, 44, 8);
      u8g2.setFont(u8g2_font_inb16_mf);
      int textWidth = u8g2.getStrWidth("Alarm");
      int textHeight = u8g2.getAscent();
      int frameWidth = 118;
      int frameHeight = 44;
      int textX = (frameWidth - textWidth) / 2 + 5;
      int textY = 6 + (frameHeight - textHeight) / 2 + textHeight / 2 - 1;
      u8g2.drawStr(textX, textY, "Alarm");
      display_start = true;
    } while (u8g2.nextPage());
  }

  if (inAPMode || dualModeActive || alarm) {
     u8g2.setPowerSave(0);
     lastChangeTime = millis();
  } else if (!inAPMode && !dualModeActive && !alarm) {
     // Check time of inactivity
     if (millis() - lastChangeTime > screenOffDelay) {
       u8g2.setPowerSave(1);
     }
  }


}


void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("Setup gestartet");
  Serial.print("-------[SETUP]--------------------[SETUP]--------------------[SETUP]-------------------[SETUP]--------------------[SETUP]--------------");

  randomSeed(micros()); // Random Int from analog Pin
  delay(100);
  randomNumber = random(1234, 9999);
  validToken = generateToken();

  // Display
  u8g2.begin();
  u8g2.setPowerSave(0);
  u8g2_draw();

  currentTime = millis();

  // Initial Pins
  pinMode(greenLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  pinMode(blueLedPin, OUTPUT);
  pinMode(inputPort, INPUT_PULLUP);

  // LED-Brightnes
  analogWrite(greenLedPin, 202);
  analogWrite(redLedPin, 202);
  analogWrite(blueLedPin, 202);


  if (SPIFFS.begin()) {
    if (SPIFFS.exists("/config.json")) {
      Serial.println("[SETUP] Die Datei /config.json existiert und ist lesbar.");
      loadConfigFromFile();
    } else {
      Serial.println("[SETUP] Die Datei /config.json existiert nicht.");
    }
  } else {
    Serial.println("[SETUP] Fehler beim Initialisieren des Dateisystems (SPIFFS).");
  }

  delay(4000);
  showConfig();

  // Check if SSID and Password exists
  if (config.ssid != "" && config.password != "") {
    connectToWiFi();
    delay(4000);
    
    if (WiFi.status() != WL_CONNECTED) {
      setupAPMode();
    } else {
      setupDualMode();
    }
  } else {
    setupAPMode();
  }


if (WiFi.status() == WL_CONNECTED) {
  // Get Time from NTP
  Serial.print("Waiting for NTP time sync...");
  timeClient.begin();
  timeClient.update();
  unsigned long ntpSyncTimeout = 15000; // Timeout NTP-Synchronisation (15 Seconds)
  unsigned long ntpSyncStartTime = millis();
  
  while (!timeClient.getEpochTime()) {
    timeClient.update();
    if (millis() - ntpSyncStartTime >= ntpSyncTimeout) {
      Serial.println("NTP-Synchronisation fehlgeschlagen");
      break;
    }
  }
  if (timeClient.getEpochTime()) {
    Serial.println("NTP-Synchronisation erfolgreich");
  }
}

// Start Webserver for Config Page
server.on("/", HTTP_GET, handleLogin);
server.on("/checkKey", HTTP_GET, handleCheckKey);
server.on("/start", HTTP_GET, handleStart);
server.on("/config", HTTP_GET, handleConfig);
server.on("/save", HTTP_POST, handleSave);
server.on("/saved", HTTP_GET, handleSaved);
server.on("/reset", HTTP_POST, handleReset);
server.on("/reset-done", HTTP_GET, handleResetDone);
server.begin();
}

void setupAPMode() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(config.apSSID, config.apPassword);
  inAPMode = true;
  dualModeActive = false;
  apStartTime = millis();
  Serial.println("[SETUP] WLAN Keine Daten");
}

void setupDualMode() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(config.apSSID, config.apPassword);
  inAPMode = true;
  dualModeActive = true;
  apStartTime = millis();
  ArduinoOTA.begin();
  ArduinoOTA.setHostname("ESP8266-OTA"); // Change Hostname
  ArduinoOTA.setPassword("9112"); // Set Password for OTA Updates
  Serial.println("[SETUP] WLAN Verbunden");
}

void loop() {
  currentTime = millis();

  if (inAPMode && WiFi.mode(WIFI_AP_STA)) {
    // Check remaining Time in Seconds
    apTimeRemaining = apStartTime + modeSwitchInterval - currentTime;
    // Convert Seconds to Minutes
    apMinutesRemaining = apTimeRemaining / 60000UL;

    server.handleClient();

    // trim Config Data
    config.iftttKey.trim();
    config.mqttResponse1.trim();
    config.mqttTopic1.trim();
    config.mqttUsername2.trim();
    config.mqttPassword2.trim();
    config.mqttTopic2.trim();
    config.mqttServer1.trim();
    config.mqttServer2.trim();

    wlanssid = (config.ssid).c_str();
    wlanpass = (config.password).c_str();
    mqttServer1 = (config.mqttServer1).c_str();
    mqttPort1 = config.mqttPort1;
    mqttTopic1 = config.mqttTopic1.c_str();
    mqttResponseTopic1 = (config.mqttResponse1).c_str();
    mqttServer2 = (config.mqttServer2).c_str();
    mqttPort2 = config.mqttPort2;
    mqttTopic2 = config.mqttTopic2.c_str();
    mqttUsername2 = config.mqttUsername2.c_str();
    mqttPassword2 = config.mqttPassword2.c_str();

    // LED AP Mode
    if (!APClientConnected) {
      if (currentTime - previousMillis_ledAP >= interval_ledAP) {
        analogWrite(blueLedPin, 0);
        previousMillis_ledAP = currentTime;
      }
    } else {
      analogWrite(blueLedPin, 10);
    }
  }

  // Check WiFi Connection
  wasWlanConnected = isWlanConnected;
  isWlanConnected = (WiFi.status() == WL_CONNECTED);
  if (!isWlanConnected && wasWlanConnected != isWlanConnected) {
    connectToWiFi();
    Serial.println("[loop] Verbindung zum WLAN herstellen...");
  }

  // Check OTA Update
  ArduinoOTA.handle();

  // Sync Time from NTP
  timeClient.update();
  utc = timeClient.getEpochTime();
  local = myTZ.toLocal(utc);
  
  nightModeControl();
  updateLedDisplay();

  // Display
  u8g2_draw();

  // Check AP Mode numb of Client connections
  int numClients = WiFi.softAPgetStationNum();
  APClientConnected = (numClients > 0);

  // Check if config has changed
  if (config != previousConfig) {
    showConfig();
    previousConfig = config;
  }

  if (inAPMode && currentTime - startTime >= modeSwitchInterval) {
    WiFi.softAPdisconnect();
    server.close();
    dualModeActive = false;
    inAPMode = false;
    WiFi.mode(WIFI_STA);
    Serial.println("[loop] Dual-Modus beendet.");
  }

  // check for Alarm
  checkAndHandleAlarm();

    
  client.loop();
  client2.loop();

  // Send Discord Messages
  //sendDiscordMessages();

  // Check MQTT Connection and Manage
  if (config.enableMqtt1 || config.enableMqtt2) {
    ManageMqttClients();
  }

  // Trigger webhook
  if (config.enableIfttt && alarm) {
    triggerWebhook();
  }
  
  // Update LED's
  updateLedDisplay();

  // Delay Loop
  delay(1000);
}

void checkAndHandleAlarm() {
  bool alarmTriggered = (digitalRead(inputPort) == LOW);

  if (alarmTriggered && !alarm) {
    alarm = true;
    analogWrite(redLedPin, 202);

    if (config.enableDiscord) {
      // sendDiscordMessage("Alarm Eingang");
      // sendDiscordMessage("Alarm Message versandt");
      // sendDiscordMessage("Alarm Message 2 versandt");
    }

    if (config.enableMqtt1 && client.connected()) {
      client.publish(mqttTopic1, "Alarm");
      if (config.enableDiscord) {
        // sendDiscordMessage("Alarm Message versandt");
      }
    }

    if (config.enableMqtt2 && client2.connected()) {
      client2.publish(mqttTopic2, "Alarm2");
      if (config.enableDiscord) {
        // sendDiscordMessage("Alarm Message 2 versandt");
      }
    }

    if (config.enableIfttt) {
      triggerWebhook();
    }

    if (config.enableMqtt1 || config.enableMqtt2 || config.enableIfttt) {
      analogWrite(blueLedPin, 202);
      if (currentTime - previousMillis_ledAP >= interval_ledAP && !APClientConnected && !inAPMode) {
        analogWrite(blueLedPin, 0);
        previousMillis_runtime = currentTime;
      }
    }
  } else if (!alarmTriggered) {
    alarm = false;
    if (!APClientConnected && !inAPMode) {
      analogWrite(blueLedPin, 0);
    }
    delay(500);

    if (was_alarm != alarm && config.enableDiscord) {
      // sendDiscordMessage("Alarm abgestellt");
    }

    was_alarm = alarm;
  }
}

void ManageMqttClients() {
  
  // Für Broker 1 (without Username und Passwort)
 if (config.enableMqtt1 && !mqtt1Configured && !client.connected()) {
  Serial.println("[ManageMqttClients] MQTT1 Config start");
  client.setServer(mqttServer1, mqttPort1);
  client.connect("ESP8266_S8");
  client.setCallback(callback);
  mqtt1Configured = true;
 }
  // Für Broker 2 (with Username und Passwort)
 if (config.enableMqtt2 && !mqtt2Configured && !client2.connected()) {
  if (mqttUsername2[0] != '\0' && mqttPassword2[0] != '\0') {
    Serial.println("[ManageMqttClients] MQTT2 Config gestartet");
    client2.connect("ESP", mqttUsername2, mqttPassword2);
  }
  client2.setServer(mqttServer2, mqttPort2);
  mqtt2Configured = true;
 }

  // Check Connection and handle
  if (!client.connected() && config.enableMqtt1) {
    if (millis() - lastMqttReconnectAttempt >= mqttReconnectInterval) {
       Serial.println("[MQTT1 Function] start");
        lastMqttReconnectAttempt = millis();
          wasMqttConnected = mqttConnected; // Vorheriger Verbindungsstatus
            if (client.connect("ESP8266_S8")) {
             client.subscribe(mqttResponseTopic1);
              mqttConnected = true;
            } else {
              mqttConnected = false;
             Serial.print("Fehlgeschlagen, rc=");
            Serial.print(client.state());
           Serial.println(" Nächster Versuch in 5 Sekunden...");
          delay(500);
         }
      }
  }

  if (!client2.connected() && config.enableMqtt2) {
    if (millis() - lastMqttReconnectAttempt2 >= mqttReconnectInterval2) {
       Serial.println("[MQTT2 Function] start");
        lastMqttReconnectAttempt2 = millis();
          wasMqttConnected2 = mqttConnected2; // Vorheriger Verbindungsstatus
            if (client2.connect("13dptz3960", mqttUsername2, mqttPassword2)) {
                mqttConnected2 = true;
            } else {
                mqttConnected2 = false;
                Serial.print("Fehlgeschlagen (Broker 2), rc=");
                Serial.print(client2.state());
                Serial.println(" Nächster Versuch in 5 Sekunden...");
                delay(500);
            }
        }
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (message.equals(mqttResponseTopic1)) {
    response = true;
    analogWrite(blueLedPin, 202);
    if (!APClientConnected) {
      analogWrite(blueLedPin, 0);
      Serial.println("[CALLBACK] ALARM BESTÄTIGT");
      previousMillis_callbackled = currentTime;
    }
    if (config.enableIfttt) {
      // DiscordMSG = "Alarm Eingang vom S8 bestätigt                " + (epochToTime(local));
      // sendDiscordMessage();
    }
  } else {
    response = false;
    Serial.println("[CALLBACK] Falsche Antwort erhalten");
    if (!APClientConnected && inAPMode) {
      analogWrite(blueLedPin, 0);
    }
  }
}

void triggerWebhook() {
 if (currentTime - previousMillis_ifttt >= interval_ifttt) {
    previousMillis_ifttt = currentTime;
 Serial.println("Sende HTTP-Anfrage...");
  if (espClient1.connect("maker.ifttt.com", 80)) {
     espClient1.print("GET /trigger/" + config.iftttEvent + "/with/key/" + config.iftttKey + " HTTP/1.1\r\n");
     espClient1.print("Host: maker.ifttt.com\r\n");
     espClient1.print("Connection: close\r\n\r\n");
     delay(500); // Warte kurz, um die Antwort zu erhalten (kann auch weggelassen werden)
    while (espClient1.available()) {
      Serial.write(espClient1.read());
    }
     Serial.println();
     Serial.println("Anfrage gesendet.");
     espClient1.stop();
    if (config.enableIfttt) {
     //DiscordMSG = "Webhook gesendet                                      " + (epochToTime(local));
     //sendDiscordMessage();
    }
  } else {
     Serial.println("Verbindung fehlgeschlagen.");
     if (config.enableIfttt) {
     //DiscordMSG = "Webhook senden fehlgeschlagen!                          " + (epochToTime(local));
     //sendDiscordMessage();
      }
    }
  }
} 

// Nighttime and LED
void nightModeControl() {
    bool isNightMode = false;
    if (config.nightModeEnabled) {
    // Check if NightTime
    int currentHour = hour(local);
    int currentMinute = minute(local);

      if (config.nightModeStartHour < config.nightModeEndHour) {
      // Nighttime within the same day
        if ((currentHour > config.nightModeStartHour || (currentHour == config.nightModeStartHour && currentMinute >= config.nightModeStartMinute)) &&
          (currentHour < config.nightModeEndHour || (currentHour == config.nightModeEndHour && currentMinute < config.nightModeEndMinute))) {
            isNightMode = true;
          } else {
            isNightMode = false;
          }
      } else {
        // Nighttime goes over 00:00 PM
        if ((currentHour > config.nightModeStartHour || (currentHour == config.nightModeStartHour && currentMinute >= config.nightModeStartMinute)) ||
          (currentHour < config.nightModeEndHour || (currentHour == config.nightModeEndHour && currentMinute < config.nightModeEndMinute))) {
              isNightMode = true;
          } else {
            isNightMode = false;
        }
     }
  } else {
     isNightMode = false;
  }
    if (isNightMode != nightMode) {
     nightMode = isNightMode;
  }
}


void updateLedDisplay() {
  
    unsigned long currentMillis = millis();
if (currentMillis - previousMillis_led >= interval_led) {
    previousMillis_led = currentMillis;
     ledState = !ledState;

 if (WiFi.status() == WL_CONNECTED) {
  
    if (!nightMode) {
      if (WiFi.status() == WL_CONNECTED && client.connected() && client2.connected() && !alarm) {
         analogWrite(greenLedPin, 1);
          analogWrite(redLedPin, 0);
          if (!inAPMode) {
          analogWrite(blueLedPin, 0);
          }
      } else if (WiFi.status() == WL_CONNECTED && !client.connected() && client2.connected() && !alarm) {
          analogWrite(greenLedPin, ledState ? 1 : 0);
          analogWrite(redLedPin, 0);
          if (!inAPMode) {
          analogWrite(blueLedPin, 0);
         }
      } else if (WiFi.status() == WL_CONNECTED && !client.connected() && !client2.connected() && !alarm) {
          analogWrite(greenLedPin, ledState ? 1 : 0);
         if (!inAPMode) {
         analogWrite(blueLedPin, ledState ? 1 : 0);
         }
       }
     } else {
        if (WiFi.status() == WL_CONNECTED && client.connected() && client2.connected() && !alarm) {
          analogWrite(redLedPin, 0);
          if (!inAPMode) {
         analogWrite(blueLedPin, 0);
          }
        analogWrite(greenLedPin, 0);
       } else if (WiFi.status() == WL_CONNECTED && client.connected() && !client2.connected() && !alarm) {
          analogWrite(redLedPin, 0);
          if (!inAPMode) {
          analogWrite(blueLedPin, 0);
          }
        analogWrite(greenLedPin, 0);
       } else if (WiFi.status() == WL_CONNECTED && !client.connected() && client2.connected() && !alarm) {
         analogWrite(redLedPin, ledState ? 1 : 0);
         analogWrite(greenLedPin, 0);
         if (!inAPMode) {
         analogWrite(blueLedPin, 0);
        }
       } else if (WiFi.status() == WL_CONNECTED && !client.connected() && !client2.connected() && !alarm) {
          analogWrite(redLedPin, ledState ? 1 : 0);
          analogWrite(blueLedPin, ledState ? 1 : 0);
          analogWrite(greenLedPin, 0);
         }
       }
     } else {
        analogWrite(greenLedPin, 0);
      }
  }  
}
