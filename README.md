# SomfyMQTT
A bridge between (RF only) Somfy sunshade and an MQTT server

The standard Telis 1 remote cannot be operated from a phone. Using an Arduido ESP D1 mini and a MQTT broker (I use Mosquitto installed in my NAS) it can be opearated thoruh the internet and is interoperable with homeautomation that can publish / subscribe MQTT topics.

# What does it look like
I now operate it from an mqtt client app on my phone. I tried 4, they all work, I liked this one the best:

![signal-2021-08-27-220122](https://user-images.githubusercontent.com/5008440/131183767-46c8a141-2b65-4d07-93be-a9f0270f46f7.jpeg)

This allows to operate the sunshade through buttons, or by rotating the knob. The knob follows the actual position of the sunshade.

# How does it interface with the sunshade

The ESP D1 mini operates a small 433.31 Mhz transmitter, I used a C1101 one. This transmitter has the advantage that you do not need to change the crystal to operate on the Somfy frequency, as its frequencies can be programmed over the whole 433Mhz band. I tried two different ones, they both worked. Note that because you operate it as a second transmitter, you need to "add" this transmitter following the manual of your sunshade. I had to "longpress" the PROG button on my original, and then press "PROG" on my newly built transdmitter. After this you can use both transmitters, allthough you cannot see the position of your sunshade like the ESP can.

# Configuration

For setting calculating the actual position, you need to configure the time it takes to open and close your sunshade. These parameters are in the config file, as well as all the other parameters, like for your wifi, your mqtt server and the topics yuo want to use. All parameters are set as #define 's in the SomfyMQTT.h file. I provide a "example"file called SomfyMQTT.EXAMPLE.h You need to rename this file and edit it to your needs.

# Uploading

The initial flash must be done through USB or a programmer. After that it supports over the air upgrades.
