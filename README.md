# SomfyMQTT
A bridge between (RF only) Somfy sunshade and an MQTT server

The standard Telis 1 remote cannot be operated from a phone. Using an Arduido ESP D1 mini and a MQTT broker (I use Mosquitto installed in my NAS) it can be opearated thoruh the internet and is interoperable with homeautomation that can publish / subscribe MQTT topics.

# What does it look like
I now operate it from an mqtt client app on my phone. I tried 4, they all work, I liked this one the best:

<img src="https://user-images.githubusercontent.com/5008440/131183767-46c8a141-2b65-4d07-93be-a9f0270f46f7.jpeg" alt="drawing" width="200"/>

This allows to operate the sunshade through buttons, or by rotating the knob. The knob follows the actual position of the sunshade.

# How does it interface with the sunshade

The ESP D1 mini operates a small 433.31 Mhz transmitter, I used a CC1101 one. 

<img src="https://user-images.githubusercontent.com/5008440/131185271-522b3dfc-2b23-4d4f-8156-b0d0ea1be165.png" alt="drawing" width="200"/>

This transmitter has the advantage that you do not need to change the crystal to operate on the Somfy frequency, as its frequencies can be programmed over the whole 433Mhz band. I tried two different ones, they both worked. Note that because you operate it as a second transmitter, you need to "add" this transmitter following the manual of your sunshade. I had to "longpress" the PROG button on my original, and then press "PROG" on my newly built transdmitter. After this you can use both transmitters, allthough you cannot see the position of your sunshade on your app if you use the Somfy remote. Only if you use the ESP for moving the shade you will see the position on your phone.

# Configuration

For setting calculating the actual position, you need to configure the time it takes to open and close your sunshade. These parameters are in the config file, as well as all the other parameters, like for your wifi, your mqtt server and the topics yuo want to use. All parameters are set as #define 's in the SomfyMQTT.config.h file. I provide a "example"file called SomfyMQTT.config.EXAMPLE.h You need to rename this file and edit it to your needs.

# Uploading

The initial flash must be done through USB or a programmer. After that it supports over the air upgrades.

# The whole build

Requires this CC1101, the ESP D1 mini and the defined 8 wires between them and a 5V USB power source. Little S@tan describes how to hook this up with his library, you can see this here: https://github.com/LSatan/SmartRC-CC1101-Driver-Lib

# libraries used

- Mentioned CC1101 driver: https://github.com/LSatan/SmartRC-CC1101-Driver-Lib
- Somfy remote: https://github.com/Legion2/Somfy_Remote_Lib.git
- MQTT: https://github.com/knolleary/pubsubclient.git

and other standard Arduino libraries, like for the ESP8266, WiFi, OTA updates etc.


