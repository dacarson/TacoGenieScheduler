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
#if defined (ESP8266)
  #include <ESP_EEPROM.h>
#elif defined (ESP32)
  #include <EEPROM.h>
#endif
#include <ArduinoJson.h>
#include "esp_netif_sntp.h"

#include "Scheduler.h"

const int EEPROM_SCHEDULE_ADDR = 0;
const int EEPROM_TZ_ADDR = ((4 * MAX_SLOTS_PER_DAY + 1) * 7) * sizeof(int);
const int EEPROM_TZ_MAX_LEN = 100;
const int EEPROM_VACA_START_ADDR = EEPROM_TZ_ADDR + EEPROM_TZ_MAX_LEN;
const int EEPROM_TIME_T_LEN = sizeof(time_t);
const int EEPROM_VACA_END_ADDR = EEPROM_VACA_START_ADDR + EEPROM_TIME_T_LEN;
const int Scheduler::EEPROMStorageSize = EEPROM_VACA_END_ADDR + sizeof(time_t);

#define NTP_SERVER "pool.ntp.org"

const char* daysOfWeek[7] = {"sunday", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday"};


Scheduler::Scheduler(int pin)
: relayPin(pin), sntpSyncDone(false), isOn(false), 
startVacationTime(-1),  endVacationTime(-1), 
isVacation(false), currentState(InActive) {
  relayOn = false;
  digitalWrite(relayPin, LOW);
}

void Scheduler::initDefault() {
  for (int i = 0; i < 7; ++i) {
    weekSchedule[i].slots[0].startHour = 7;
    weekSchedule[i].slots[0].startMinute = 0;
    weekSchedule[i].slots[0].endHour = 9;
    weekSchedule[i].slots[0].endMinute = 0;
    
    weekSchedule[i].slots[1].startHour = 18;
    weekSchedule[i].slots[1].startMinute = 0;
    weekSchedule[i].slots[1].endHour = 21;
    weekSchedule[i].slots[1].endMinute = 0;
    
    weekSchedule[i].slotCount = 2;
  }
}

time_t Scheduler::parseDateTime(String dateTime) {
    int year, month, day, hour, minute;
    int numParsed = sscanf(dateTime.c_str(), "%d-%d-%dT%d:%d", &year, &month, &day, &hour, &minute);
    if (numParsed != 5) {
        Serial.print("Error: Date and time format empty or incorrect: ");
        Serial.println(dateTime);
        return 0; // Return 0 or some other error indicator
    }

    // tmElements_t is a struct that holds time elements separately
    tm convert_tm;
    convert_tm.tm_year = year - 1900;  // tm_year is the number of years since 1900
    convert_tm.tm_mon = month - 1; // tm_mon is 0 - 11
    convert_tm.tm_mday = day;
    convert_tm.tm_hour = hour;
    convert_tm.tm_min = minute;
    convert_tm.tm_sec = 0;  // Assuming seconds are zero if not specified

    return mktime(&convert_tm);  // Convert to time_t
}

bool Scheduler::saveScheduleToEEPROM() {
  
    // Don't write unchanged data
  DaySchedule weekScheduleEEPROM[7];
  if (EEPROM.readBytes(EEPROM_SCHEDULE_ADDR, weekScheduleEEPROM, sizeof(weekScheduleEEPROM)) != sizeof(weekScheduleEEPROM)) {
    Serial.println("Failed to load schedule from EEPROM for save comparison.");
  }
  
  if (memcmp(weekSchedule, weekScheduleEEPROM, sizeof(weekSchedule)) == 0) {
    Serial.println("Schedule unchanged, not saving to EEPROM");
    return true;
  }
  
  if (EEPROM.writeBytes(EEPROM_SCHEDULE_ADDR, weekSchedule, sizeof(weekSchedule)) != sizeof(weekSchedule)) {
    Serial.println("Failed to save schedule to EEPROM.");
    return false;
  }
  EEPROM.commit();
  return true;
}

bool Scheduler::loadScheduleFromEEPROM() {
  if (EEPROM.readBytes(EEPROM_SCHEDULE_ADDR, weekSchedule, sizeof(weekSchedule)) != sizeof(weekSchedule)) {
    Serial.println("Failed to load schedule from EEPROM.");
    return false;
  }
  
    // Validate what we loaded.
  for (int i = 0; i < 7; i++) {
    if (weekSchedule[i].slotCount > MAX_SLOTS_PER_DAY) {
      Serial.println("Schedule corrupt in EEPROM.");
      return false;
    }
    for (int x = 0; x < weekSchedule[i].slotCount; x++) {
      if (weekSchedule[i].slots[x].startHour < 0 || weekSchedule[i].slots[x].startHour > 23) {
        Serial.println("Schedule corrupt in EEPROM.");
        return false;
      }
      if (weekSchedule[i].slots[x].startMinute < 0 || weekSchedule[i].slots[x].startMinute > 59) {
        Serial.println("Schedule corrupt in EEPROM.");
        return false;
      }
      if (weekSchedule[i].slots[x].endHour < 0 || weekSchedule[i].slots[x].endHour > 23) {
        Serial.println("Schedule corrupt in EEPROM.");
        return false;
      }
      if (weekSchedule[i].slots[x].endMinute < 0 || weekSchedule[i].slots[x].endMinute > 59) {
        Serial.println("Schedule corrupt in EEPROM.");
        return false;
      }
    }
  }
  
  return true;
}

/* 
  Returns true on success
*/
bool Scheduler::jsonToSchedule(String json) {
  JsonDocument doc;
  
  auto error = deserializeJson(doc, json);
  if (error) {
    return false;
  }
  
  Serial.println("Scheduler jsonToSchedule:");
  Serial.println(json);
  
  for (int i = 0; i < 7; ++i) { // Iterate through each day of the week
    auto dayArray = doc[daysOfWeek[i]];
    int slotIndex = 0;
    
    for (JsonObject slot : dayArray.as<JsonArray>()) {
      if (slotIndex >= MAX_SLOTS_PER_DAY) break; // Prevent overflow
      
      const char* startTime = slot["start"];
      const char* endTime = slot["end"];
      
        // Parse and store start and end times
      sscanf(startTime, "%d:%d", &weekSchedule[i].slots[slotIndex].startHour, &weekSchedule[i].slots[slotIndex].startMinute);
      sscanf(endTime, "%d:%d", &weekSchedule[i].slots[slotIndex].endHour, &weekSchedule[i].slots[slotIndex].endMinute);
      
      slotIndex++;
    }
    weekSchedule[i].slotCount = slotIndex; // Update count of valid slots for the day
  }
  
  if (!saveScheduleToEEPROM())
    return false;
  
  return true;
}

String Scheduler::scheduleToJson() {
  JsonDocument doc;
  
  for (int i = 0; i < 7; i++) {
    JsonArray dayArray = doc[daysOfWeek[i]].to<JsonArray>();
    for (int j = 0; j < weekSchedule[i].slotCount; j++) {
      JsonObject slot = dayArray.add<JsonObject>();
      slot["start"] = (weekSchedule[i].slots[j].startHour < 10 ? "0" : "") + String(weekSchedule[i].slots[j].startHour)
      + ":" + (weekSchedule[i].slots[j].startMinute < 10 ? "0" : "") + String(weekSchedule[i].slots[j].startMinute);
      slot["end"] = (weekSchedule[i].slots[j].endHour < 10 ? "0" : "") + String(weekSchedule[i].slots[j].endHour)
      + ":" + (weekSchedule[i].slots[j].endMinute < 10 ? "0" : "") + String(weekSchedule[i].slots[j].endMinute);
    }
  }
  
  String jsonStr;
  serializeJson(doc, jsonStr);
  
  Serial.println("Scheduler scheduleToJson: ");
  Serial.println(jsonStr);
  
  return jsonStr;
}

String Scheduler::stringToDateTime(time_t value) {
    char buffer[26];
    struct tm* tm_info;

    tm_info = localtime(&value);
    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);

    return buffer;
}

bool Scheduler::jsonToVacationTime(String json) {
  JsonDocument doc;
  
  auto error = deserializeJson(doc, json);
  if (error) {
    return false;
  }
  
  Serial.println("Scheduler jsonToVacationTime:");
  Serial.println(json);

  time_t newStartTime = parseDateTime(doc["startDate"]);
  time_t newEndTime = parseDateTime(doc["endDate"]);

  if (newStartTime != startVacationTime || newEndTime != endVacationTime) {
    startVacationTime = newStartTime;
    endVacationTime = newEndTime;

    Serial.print("New vacation Start: ");
    Serial.println(stringToDateTime(startVacationTime));
    Serial.print("New vacation End: ");
    Serial.println(stringToDateTime(endVacationTime));

    if (EEPROM.writeLong(EEPROM_VACA_START_ADDR, startVacationTime) == sizeof(startVacationTime)) {
      if (EEPROM.writeLong(EEPROM_VACA_END_ADDR, endVacationTime) == sizeof(endVacationTime)) {
        Serial.println("Saved start and end vacation time to EEPROM.");
        EEPROM.commit();
      } else {
        Serial.println("Failed to save vacation end to EEPROM.");
      }
    } else {
      Serial.println("Failed to save vacation start to EEPROM.");
    }
  } else {
    Serial.println("Vacation unchanged, not saving to EEPROM");
  }

   return true;
}

String Scheduler::vacationTimeToJson() {
  JsonDocument doc;

  if (startVacationTime) {
    doc["startDate"] = stringToDateTime(startVacationTime);
  } else {
    doc["startDate"] = "";
  }

  if (endVacationTime) {
    doc["endDate"] = stringToDateTime(endVacationTime);
  } else {
    doc["endDate"] = "";
  }
  
  String jsonStr;
  serializeJson(doc, jsonStr);
  Serial.println("Scheduler vacationTimeToJson: ");
  Serial.println(jsonStr);

  return jsonStr;
}

void Scheduler::setVacationState(bool active) {
  endVacationTime = 0;

  if (active) {
    startVacationTime = time(nullptr);
  } else {
    startVacationTime = 0;
  }
}

Scheduler::State Scheduler::getNextState(time_t *nextStateTime) const {

  if (currentState == State::Vacation) {
    if (nextStateTime) {
      *nextStateTime = endVacationTime;
      return State::Vacation;
    }
  }

// Find the next event
  time_t now = time(nullptr);
  struct tm *tm_struct = localtime(&now);
  
  int currentHour = tm_struct->tm_hour;
  int currentMinute = tm_struct->tm_min;
  int currentDay = tm_struct->tm_wday;

  int currentTimeInMinutes = currentHour * 60 + currentMinute;

  for (int dayoffset = 0; dayoffset < 7; dayoffset++) {
    int day = (currentDay + dayoffset) % 7;
    for (int slot = 0; slot < weekSchedule[day].slotCount; slot++) {
        int startMin = dayoffset * 24 * 60 + weekSchedule[day].slots[slot].startHour * 60 + weekSchedule[day].slots[slot].startMinute;
        if (startMin > currentTimeInMinutes) {
          // Calculate start time of next item
          tm nextState_tm;
          nextState_tm.tm_year = tm_struct->tm_year;
          nextState_tm.tm_mon = tm_struct->tm_mon;
          nextState_tm.tm_mday = tm_struct->tm_mday + dayoffset;
          nextState_tm.tm_hour = weekSchedule[day].slots[slot].startHour;
          nextState_tm.tm_min = weekSchedule[day].slots[slot].startMinute;
          nextState_tm.tm_sec = 0; 
          *nextStateTime = mktime(&nextState_tm);
          if (startVacationTime && startVacationTime < *nextStateTime) {
            *nextStateTime = startVacationTime;
            return State::Vacation;
          }
          return State::Active;
        }
        int endMin = dayoffset * 24 * 60 + weekSchedule[day].slots[slot].endHour * 60 + weekSchedule[day].slots[slot].endMinute;
        if (endMin > currentTimeInMinutes) {
          // Calculate start time of next item
          tm nextState_tm;
          nextState_tm.tm_year = tm_struct->tm_year;
          nextState_tm.tm_mon = tm_struct->tm_mon;
          nextState_tm.tm_mday = tm_struct->tm_mday + dayoffset;
          nextState_tm.tm_hour = weekSchedule[day].slots[slot].endHour;
          nextState_tm.tm_min = weekSchedule[day].slots[slot].endMinute;
          nextState_tm.tm_sec = 0; 
          *nextStateTime = mktime(&nextState_tm);
          // Check to see if vacation will start before we hit this event.
          if (startVacationTime && startVacationTime < *nextStateTime) {
            *nextStateTime = startVacationTime;
            return State::Vacation;
          }
          return State::InActive;
        }
    }
  }

// Found no timeslots or vacation slot, so we are in active indefinitely
  *nextStateTime = 0;
  return InActive;
}

bool Scheduler::isTimeWithinSlot(int currentHour, int currentMinute, TimeSlot slot) {
    // Convert times to minutes since midnight for comparison
  int currentTimeInMinutes = currentHour * 60 + currentMinute;
  int slotStartInMinutes = slot.startHour * 60 + slot.startMinute;
  int slotEndInMinutes = slot.endHour * 60 + slot.endMinute;
  
  return currentTimeInMinutes >= slotStartInMinutes && currentTimeInMinutes <= slotEndInMinutes;
}

void Scheduler::setTz(String timezone) {
  if (tz == timezone) {
    Serial.print(timezone);
    Serial.println(" Timezones are the same, not updating.");
  } else {
    Serial.print("Updating timezone to ");
    Serial.println(timezone);
    tz = timezone;
    
    setenv("TZ", timezone.c_str(), 1);
    tzset();
    
    size_t saveLength = EEPROM.writeString(EEPROM_TZ_ADDR, tz);
    if (saveLength < tz.length()) {
      Serial.println("Failed to save TZ to EEPROM");
    }
    EEPROM.commit();
  }
}

bool Scheduler::isActive() {
  return isOn;
}

bool Scheduler::vacationActive() {
  return isVacation;
}

int Scheduler::begin() {
  if (!EEPROM.begin(EEPROMStorageSize)) {
    Serial.println("Failed to access EEPROM");
    return false;
  }
  
  // If we failed to load the schedule from EEPROM, then
  // init to a default schedule.
  if (!loadScheduleFromEEPROM()) {
    Serial.println("No saved Schedule, loading default.");
    initDefault();
  }
  
  tz = EEPROM.readString(EEPROM_TZ_ADDR);
  if (tz.length() == 0) {
    Serial.println("No saved TZ, schedules won't run until set by loading the webpage.");
    return false;
  }

  if ((startVacationTime = EEPROM.readLong(EEPROM_VACA_START_ADDR)) < 0) {
    Serial.println("Failed to load vacation start time.");
    startVacationTime = 0;
  }
  if ((endVacationTime = EEPROM.readLong(EEPROM_VACA_END_ADDR)) < 0) {
    Serial.println("Failed to load vacation end time.");
    endVacationTime = 0;
  }

  Serial.print("Restoring saved TZ: ");
  Serial.println(tz);
  setenv("TZ", tz.c_str(), 1);
  tzset();

  // Enable SNTP
  esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(NTP_SERVER);
  esp_netif_sntp_init(&config); // DEFAULT_CONFIG is to start the server on init()
  sntpSyncDone = false;
  
  return true;
}

void Scheduler::update() {
  if (!sntpSyncDone && esp_netif_sntp_sync_wait(2000) == ESP_ERR_TIMEOUT) {
    if(tz.length() > 0) {
      Serial.print("+");
    }
    return;
  }
  if (!sntpSyncDone) {
    Serial.println("SNTP Sync done");
  }
  
  time_t now = time(nullptr);
  struct tm *tm_struct = localtime(&now);
  
  int currentHour = tm_struct->tm_hour;
  int currentMinute = tm_struct->tm_min;
  int currentDay = tm_struct->tm_wday;
  
  currentState = State::InActive;
  if ((!startVacationTime || now < startVacationTime) && (!endVacationTime || now > endVacationTime)) {
    for (int i = 0; i < weekSchedule[currentDay].slotCount; i++) {
      if (isTimeWithinSlot(currentHour, currentMinute, weekSchedule[currentDay].slots[i])) {
        currentState = State::Active;
          // Turn on the relay every 10 minutes within the time slot
          // Or if we just finished syncing and it should be on
        if (!sntpSyncDone || currentMinute % 30 == 0) {
          if (!relayOn) {
            digitalWrite(relayPin, HIGH);
            Serial.println("Relay ON");
          }
          relayOn = true;
        } else {
          if (relayOn) {
            digitalWrite(relayPin, LOW);
            Serial.println("Relay OFF");
          }
          relayOn = false;
        }
      }
    }
  } else {
    currentState = State::Vacation;
  }

  sntpSyncDone = true;
}
