#include <ArduinoLog.h>
#include <Arduino.h>

#include "RF24Message.h"

CRF24Message::CRF24Message() {  
  memset(&_msg, 0, sizeof(_msg));
}

CRF24Message::CRF24Message(float voltage, float temperature, float humidity, uint16_t uptime) {  
  setVoltage(voltage);
  setTemperature(temperature);
  setHumidity(humidity);
  setUptime(uptime);
}

const void* CRF24Message::getMessageBuffer() {
  return &_msg;
}

float CRF24Message::getVoltage() {
  return _msg.voltage.value;
}

void CRF24Message::setVoltage(float value) {
  _msg.voltage.key = 'V';
  _msg.voltage.value = value;
}

float CRF24Message::getTemperature() {
  return _msg.temperature.value;
}

void CRF24Message::setTemperature(float value) {
  _msg.temperature.key = 'T';
  _msg.temperature.value = value;
}

float CRF24Message::getHumidity() {
  return _msg.humidity.value;
}

void CRF24Message::setHumidity(float value) {
  _msg.humidity.key = 'H';
  _msg.humidity.value = value;
}

uint16_t CRF24Message::getUptime() {
  return _msg.uptime.value;
}

void CRF24Message::setUptime(uint16_t value) {
  _msg.uptime.key = 'U';
  _msg.uptime.value = value;
}
