![Github License](https://img.shields.io/github/license/dacarson/TacoGenieScheduler) ![Github Release](https://img.shields.io/github/v/release/dacarson/TacoGenieScheduler?display_name=tag)

<img src="https://github.com/dacarson/TacoGenieScheduler/blob/main/Logo.png" height="200"/>

# TacoGenieScheduler
 ESP8266/Wemos D1 mini schedule controller for TacoGenie
 

## Description
The TacoGenie is a water circulator used to heat pipes so that hot water
is available when you turn the tap on. Keeping the pipes hot continously is a waste
of energy, and conversly, failing to remember to activate the unit can 
waste water waiting for it to get hot. 
The user can attach a Starter button and/or Wireless Remotes or Motion 
Sensors. The wiring for the Wireless Remotes or Motion Sensors consists 
of 12V DC (white and black wires) and a start line (green wire).
The TacoGenie runs until it detects hot water and then switches off.

Through the web page you can configure when you would like the unit to run. The web page allows you to have 10 different time slots per day. Each day can be configured separately. When in an active timeslot, the software tells the unit to start every 10mins. When the pipes are hot, it automatically turns off.

You can also configure a vacation timespan so that when you are away, the pipes are not being heated. 

### Home screen
<img width="1315" alt="Home Screen" src="https://github.com/user-attachments/assets/c308c9ba-76bb-4c2c-a860-35a8b60febdc"/>

### Scheduling screen
<img width="1315" alt="Schedule" src="https://github.com/user-attachments/assets/b07a10e8-e3f6-4ff7-ba36-316ec204af24"/>

### Vacation mode screen
<img width="1315" alt="Vacation Settings" src="https://github.com/user-attachments/assets/4b8b8c7b-223e-48ac-afea-fc201bcf7fa4"/>


## Setup
### Hardware
Using a [relay shield](https://www.wemos.cc/en/latest/d1_mini_shield/relay.html) on a [Wemos mini D1](https://www.wemos.cc/en/latest/d1/d1_mini.html), wiring the NO (Normally Open) 
side to the white and green wires.  The relay can then trigger the unit
to start. The unit automatically runs till the pipes are warm.

Using a [DC Power Shield](https://www.wemos.cc/en/latest/d1_mini_shield/dc_power.html) (7-24V DC) on the Wemos, the Wemos can be 
powered from the TacoGenie 12V DC line.

### Software
Change the lines in `TacoGenieScheduler.ino` to contain your WiFi SSID and Password:
```
const char *ssid     = "your_wifi_ssid";
const char *password = "your_wifi_password";
```
Make sure you [upload](https://github.com/earlephilhower/arduino-littlefs-upload) the web files to a LittleFS file system on the Wemos unit.

## License
This library is licensed under [MIT License](https://opensource.org/license/mit/)
