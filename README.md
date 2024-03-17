# RF24 to Wifi (MQTT) Gateway for STUS devices

# Main purpose

STUS devices transmit data with nrf24l01 modules using pre-defined 32byte messages.
This project is designed to listen for these radio messages, convert them to JSON and post them to an MQTT topic over WiFi.

The message format is defined in https://github.com/jaisor/stus-rf24-commons

# Auxillary functions

This device is meant to be constantly on. As such, there is good opportunity for including a temperature and voltage sensors.
The device can be installed in an RV or Travel Trailer, for example, as telemetry bridge for Solar or other STUS sensors.
In that scenario the temperature sensor can be used to measure indoor temperature and humidity.
The voltage sensor can be used to measure state of charge for an RV battery, starter, auxillary or any other not already monitored by the solar installation.

# Hardware

Code build supports ESP32 and ESP8266 devices. 
TODO: Schematic

# Web configuration
TODO

