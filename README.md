# ESP Remote Captive Portal
This is a simple program made for ESP boards. It allows you to create your own captive portal server without the need to hard-code it into your ESP board.
# Getting started
There are two ways to install this program:
## Installation
### 1. Use prebuilt binaries
* Download the correct binary file for your ESP board [here](https://google.com).
* Use an software to upload the firmware into your ESP board. Here are some of my recommendations [esp.huhn.me](https://esp.huhn.me), [NodeMCU Flasher](https://github.com/nodemcu/nodemcu-flasher), [esptool](https://github.com/espressif/esptool).
### 2. Compile the program by yourself
* Download Arduino IDE or any IDE that have the ability compile/upload if you haven't.
* Ensure that you have all of these dependencies installed: [ArduinoJson](https://github.com/bblanchon/ArduinoJson), [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer), [StreamUtils](https://github.com/bblanchon/ArduinoStreamUtils).
* Select and download the correct source code for your ESP board from [here](https://e) and open it in your IDE
* Connect the ESP board to your device.
* Compile then upload the firmware into your ESP board.
## Setting up
Here are the steps to setup this program after installation:
* Connect to the Wi-Fi access point named "ESPRCP" (There are no password).
* Open your browser and head to this url: [http://192.168.4.1:8000](http://192.168.4.1:8000)
* Login using the following the username "manager" and password "12345678".
* Configure your access point and captive portal address, remember to change the username and password of manager account as well.
* Click the "Save" button.
* Use any software or programming languages that you familiar with and start creating a local web server at the address you set earlier, all devices connect to your access point will be redirect to your server.
