/*
 Copyright (c) 2024 David Carson (dacarson)
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

/*
 TacoGenieScheduler
 
 The TacoGenie is a water circulator used to heat pipes so that hot water
 is available when you turn the tap on. Keeping the pipes hot is a waste
 of energy, and conversly, failing to remember to activate the unit can
 waste water waiting for it to get hot.
 The user can attach a Starter button and/or Wireless Remotes or Motion
 Sensors. The wiring for the Wireless Remotes or Motion Sensors consists
 of 12V DC (white and black wires) and a start line (green wire).
 
 Using a relay shield on a Wemos mini D1, wiring the NO (Normally Open)
 side to the white and green wires.  The relay can then trigger the unit
 to start. The unit automatically runs till the pipes are warm.
 
 Using a DC Power Shield (7-24V DC) on the Wemos, the Wemos can be
 powered from the TacoGenie 12V DC line.
 */

#if defined (ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #include <ESP8266mDNS.h>
#else if defined (ESP32_DEV)
  #include <WiFi.h>
  #include <WebServer.h>
  #include <ESPmDNS.h>
#endif
#include <FS.h>
#include <LittleFS.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <time.h>

#include "Scheduler.h"

  // WiFi Credentials
const char *ssid     = "your_wifi_name";
const char *password = "your_wifi_password";

#if defined (ESP8266)
  const int relayPin = D1;
  const int led = LED_BUILTIN;
#else if defined (ESP32_DEV)
  const int relayPin = 22;
  const int led = 2;
#endif

  // Construct a global scheduler object to manage the schedule
Scheduler scheduler(relayPin);

  // Construct a webserver
#if defined (ESP8266)
  ESP8266WebServer server(80);
#else if defined (ESP32_DEV)
  WebServer server(80);
#endif

void handleSetConfig() {
    // Check if there's any POST data
  if (server.hasArg("plain") == false) { // "plain" is for raw POST data
    server.send(500, "text/plain", "POST body not found");
    return;
  }
  
  if (!scheduler.jsonToSchedule(server.arg("plain"))) {
    server.send(500, "text/plain", "Error parsing JSON");
    return;
  }
  server.send(200, "text/plain", "Schedule updated");
}

void handleGetConfig() {
  server.send(200, "application/json", scheduler.scheduleToJson());
}

void handleGetTimezone() {
  if (server.hasArg("tz")) {
    String timezone = server.arg("tz");
    scheduler.setTz(timezone);
  } else {
    Serial.println("Missing timezone from timezone image request");
  }
  server.send(200, "image/png", "");
}

void setup() {
  pinMode ( led, OUTPUT );
  
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println ( F("") );
  Serial.print ( F("Connecting to WiFi "));
  
    // Ensure relay is off initially
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);
  
    // Wait for connection
  int flash = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (flash & 0x1) {
      digitalWrite(led, HIGH);
    } else {
      digitalWrite(led, LOW);
    }
    flash++;
  }
  
  Serial.println ( F("") );
  Serial.print (F("Connected to ") );
  Serial.println ( ssid );
  Serial.print ( F("IP address: ") );
  Serial.println ( WiFi.localIP() );
  
  if(!LittleFS.begin()){
    Serial.println("LittleFS Mount Failed");
  }
  
  scheduler.begin();
  
  server.on("/getConfig", HTTP_GET, handleGetConfig);
  server.on("/setConfig", HTTP_POST, handleSetConfig);
  
  server.on("/timezone.png", HTTP_GET, handleGetTimezone);
  
  server.serveStatic ("/", LittleFS, "/index.html");
  server.serveStatic ("/main.css", LittleFS, "/main.css");
  server.serveStatic ("/bkgnd.jpg", LittleFS, "/bkgnd.jpg");
  server.serveStatic ("/main.js", LittleFS, "/main.js");
  server.serveStatic ("/moment.js", LittleFS, "/moment.js");
  server.serveStatic ("/momentTimezone.js", LittleFS, "/momentTimezone.js");
  server.serveStatic ("/favicon.ico", LittleFS, "/favicon.ico");
  server.begin();
  
}

void loop() {
  server.handleClient();
  scheduler.update();
  if (scheduler.isActive()) {
    digitalWrite(led, HIGH);
  } else {
    digitalWrite(led, LOW);
  }
}
