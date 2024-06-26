# alarmbox                                                                                    v1.0.0

This is a small code for an `ESP8266` to integrate analog signals into smart homes.
I use it to connect my pager `Airbus P8GR` to Home Assistant.
Since I couldn't find any comparable code, I thought I'd just make it available to everyone.
He's certainly not perfect.


<img src="https://github.com/Knockback2003/alarmbox/blob/main/pictures/1.jpg"  width="200" height="200">


## Requirements

* ESP8266
* OLED Display
* (RGB LEDs)
* Wires


## Installation

* Flash the Code with Arduino IDE
* Green LED PIN: 14
* RED LED PIN: 12
* BLUE LED PIN: 0
* INPUT PORT: 2

* The input port works as a pushdown resistor. The ESP gets triggered, if the voltage from pin 2 falls.

* ! If you want to youse it also for an Airbus P8GR you must connect Pin 2 to the left relay pin (on front view to the homestation) and the right relay pin to the ground on the ESP!


## Configuration

When you plug in the ESP8266, if you have installed LEDs, all LEDs should light up and the display should say "Start" for about 10 seconds. He then opened the Access Point and the display should switch between the IP address of the ESP8266 and a generated key. If no configuration parameters have been set, which should be the case when you first start, only the red LED should light up.

* Connect a device with the WiFi: Its default `Alarmbox` with the Password `12345678`.
* The blue LED on the device should now light up (a Client is connected).
* Type in your browser the IP that is shown on the display. You should now see a login page.
* Type in the generated Key that is shown on your display and press the login button: You should now see an ugly page with only a hamburger menu on it. (There are plans to revise this but feel free to create a pull request).
* Press on the first link on the menu (The only one that functions).
* You should now see a Config-Page with the minutes remaining of the Access Point available.
* Type in your configurations and press "Save" or "Speichern".
* If the save process was successful you should be redirected to an overview Page. Yes, the data is saved. You should not forward the port of the ESP8266 and use it only in your private network.
* If the data is correct you can disconnect from the Wifi of the ESP and were done! The Display should go off when the access point switches off or 10 minutes after inactivity (such as no alarm)

* In some cases you may have to unplug the ESP and then plug it back in for it to load the data.

* But the Nighttime can be set on the go.


## Features

* NTP
* 2 MQTT Broker
* IFTTT Integration
* Nighttime Mode (LEDs are handled differently)
* (ESP Logging via Discord) For some reason the Discord Webhook Libery is not compatible with the U8g2lib
* Real-Time Alarm Notification over MQTT (up to < 2 sec.)

### LEDs

#### Nighttime enabled and set

##### Day

* GREEN: Light = Everything is ok
         Flash = No connection to MQTT 1
         OFF   = No Connection to WiFi

* RED:  Light = Alarm
        OFF   = Everything is ok

* BLUE: Light = MQTT response / Webhook response on Alarm is ok
        OFF   = Everything is ok when no alarm / Alarm: You should check your library
        Flash = Alarm Message sent successfully (MQTT / IFTTT) (Flashes one Time)


##### Night

* GREEN: OFF   = Everything is ok

* RED:  Light = Alarm
        OFF   = Everything is ok
        Flash = No connection to MQTT 1

* BLUE: OFF   = Everything is ok when no alarm / Alarm: You should check your library
        Flash = No connection to MQTT 2

(in the case of the Alarm the LED function is the same as the day!)
(The LEDs are a little complex, look at the Code for more details about the LED light/flash signs)




## Bugs

* In some cases you may have to unplug the ESP and then plug it back in for it to load the data.
* If MQTT 1 Connection is disabled the LED still flashes
* In some rare cases the ESP makes a soft restart after he switches the AP Mode off.


## Releases

### Version 1.1.0
* Implemented Automatic Updates via GitHub (testing)

### Version 1.0.0
* Fix Bugs
* MQTT 1 / 2 opinion to use Username/Password


## Future
* Option to use Home Assistant REST-API to create a sensor Entity's in Home Assistant that shows various states from the ESP8266
* Make Config Page more aesthetic



(This is/was a hobby project from 2023 that was published in 2024. I'm NOT a professional programmer. The Code works for me perfectly with high reliability)
