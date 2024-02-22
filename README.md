# alarmbox                                                                                    v1.0.0

This is a small code for an `ESP8266` to integrate analog signals into smart homes.
I use it to connect my pager `Airbus P8GR` to Home Assistant.
Since I couldn't find any comparable code, I thought I'd just make it available to everyone.
He's certainly not perfect.


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

* The input port wroks as pushdown-resistor. The esp gets triggert, if the voltage from pin 2 fell down.

* ! If you wont to youse it also for a Aribus P8GR you must connect Pin 2 to the left relay pin (on front view to the homestation) and the right relay pin to ground on the ESP !


## Configuration

When you plug in the ESP8266, if you have installed LEDs, all LEDs should light up and the display should say "Start" for about 10 seconds. He then opened the Access Point and the display should switch between the IP address of the ESP8266 and a generated key. If no configuration parameters have been set, which should be the case when you first start, only the red LED should light up.

* Connect a device with the WiFi: Its default `Alarmbox` with the Password `12345678`.
* The blue LED on the device should now light up (a Client is connected).
* Type in your browser the ip that is shown on the dispplay. You should now see a Login-Page.
* Type in the genereted Key that is shown on your display and press the login button: You should now see an ugly page with only a hamburger menu on it. (There are plans to revise this but feel free to create a pull request).
* Press on the first link on the menu (The only one that functions).
* You should now see an Config-Page with the minutes remaining of the Access Point is available.
* Type in your configurations and press "Save" or "Speichern".
* If the save process was successfull you should be redirected to a overwiev Page. Yes the data is clearly saved. You should not forward the port of the ESP8266 and use it only in your own private network.
* If the data is correctly you can disconnect from the Wifi of the ESP and were done!

* In some cases you may have to unplug the ESP and then plug it back in for it to load the data.

* But the Nighttime can be set on the go.


## Features

* NTP
* 2 MQTT Broker
* IFTTT Integration
* Nighttime Mode (LED are handled different)
* (ESP Logging via Discord) Of some reasons the Discord Webhook Libery is actually not compatible to the U8g2lib
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
        OFF   = Everything is ok when no alarm / Alarm: You should check your liberys
        Flash = Alarm Message send succesfull (MQTT / IFTTT) (Flashes one Time)


##### Night

* GREEN: OFF   = Everything is ok

* RED:  Light = Alarm
        OFF   = Everything is ok
        Flash = No connection to MQTT 1

* BLUE: OFF   = Everything is ok when no alarm / Alarm: You should check your liberys
        Flash = No connection to MQTT 2

(in case of Alarm the LED function is the same of the day!)
(The LED's are a little complex, look at the Code for more details about the LED light/flash signs)




## Bugs

* In some cases you may have to unplug the ESP and then plug it back in for it to load the data.
* If MQTT 1 Connection is disabled the LED still flashes
* In some rare cases the ESP makes a soft restart after he switch the AP Mode off.


## Future

* Fix Bugs
* Make Config Page more asthetic
* MQTT 1 / 2 opinion to use Username/Password


(This is/was a hobby project from 2023 what is published 2024. Im NOT a professioal programmer. The Code works for me perfect with high reliability)
