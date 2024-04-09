i/*
Copyright (c) 2023 David Carson (dacarson)

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

#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <ArduinoJson.h> // For parsing JSON schedule

const char* ssid = "yourSSID";
const char* password = "yourPassword";
const int led = LED_BUILTIN;
#if defined (ESP8266)
  const int relayPin = D1;
#else if defined (ESP32_DEV)
  const int relayPin = 22;
#endif

// We'll use time provided by Network Time Protocol to get EpochTime 
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Structure to store schedule for a day
struct TimeSlot {
  int startHour;
  int startMinute;
  int endHour;
  int endMinute;
};

// Example of a single day's schedule
// You will need to expand this concept for a full week's schedule
TimeSlot todaySchedule[] = {
  {7, 0, 9, 0},
  {18, 0, 21, 0}
};

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW); // Ensure relay is off initially

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

    // Start the timeclient
  timeClient.begin();
  // update every 7 days (1000ms * 60mins * 60hrs * 24hrs * 5days)
  timeClient.setUpdateInterval(1000 * 60 * 60 * 24 * 5);


  // Initialize time library, connect to NTP, etc.
}

void loop() {
  timeClient.update();
  // Main logic to check schedule and toggle relay
  checkAndToggleRelay();
}

void checkAndToggleRelay() {
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();

  for (TimeSlot slot : todaySchedule) {
    if (isTimeWithinSlot(currentHour, currentMinute, slot)) {
      // Turn on the relay every 10 minutes within the time slot
      if (currentMinute % 10 == 0) {
        digitalWrite(relayPin, HIGH);
        Serial.println("Relay ON");
      } else {
        digitalWrite(relayPin, LOW);
        Serial.println("Relay OFF");
      }
    }
  }
}

bool isTimeWithinSlot(int currentHour, int currentMinute, TimeSlot slot) {
  // Convert times to minutes since midnight for comparison
  int currentTimeInMinutes = currentHour * 60 + currentMinute;
  int slotStartInMinutes = slot.startHour * 60 + slot.startMinute;
  int slotEndInMinutes = slot.endHour * 60 + slot.endMinute;

  return currentTimeInMinutes >= slotStartInMinutes && currentTimeInMinutes <= slotEndInMinutes;
}
