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

#include <time.h>

#define MAX_SLOTS_PER_DAY 10 // Adjust based on your expected maximum

class Scheduler {
  public:
    Scheduler(int pin);
    virtual ~Scheduler() {}

    int begin();
    void update();

    bool isActive();

    void setTz(String timezone);

    bool jsonToSchedule(String json);
    String scheduleToJson();

protected:
    // Structure to store schedule for a day
    struct TimeSlot {
      int startHour;
      int startMinute;
      int endHour;
      int endMinute;
    };

    struct DaySchedule {
      TimeSlot slots[MAX_SLOTS_PER_DAY];
      int slotCount = 0; // How many slots are actually used
    };

  bool saveScheduleToEEPROM();
  bool loadScheduleFromEEPROM();

  bool isTimeWithinSlot(int currentHour, int currentMinute, TimeSlot slot);
  void initDefault();

  DaySchedule weekSchedule[7]; // 0 = Sunday, 6 = Saturday

  String tz;
  bool sntpSyncDone;
  int relayPin;
  bool relayOn;
  bool isOn;

  static const int EEPROMStorageSize;
};