#include <ArduinoLog.h>
#include <Arduino.h>

#include "RF24Message.h"

r24_message_uvthp_t _msg;

// Message Uptime-Voltage-Temperature-Humidity-BarometricPressure
#define MSG_UVTHP_ID 1

CRF24Message::CRF24Message(const u_int8_t pipe, float voltage, float temperature, float humidity, uint16_t uptime)
: CBaseMessage(pipe) {  
  
  setVoltage(voltage);
  setTemperature(temperature);
  setHumidity(humidity);
  setUptime(uptime);
  _msg.id = MSG_UVTHP_ID;
}

CRF24Message::CRF24Message(const u_int8_t pipe, const void* buf, uint8_t length)
: CBaseMessage(pipe) { 
  // TODO: Validate message id
  memcpy(&_msg, buf, length);
}

const void* CRF24Message::getMessageBuffer() {
  return &_msg;
}

uint32_t CRF24Message::getUptime() {
  return _msg.uptime;
}

void CRF24Message::setUptime(uint32_t value) {
  _msg.uptime = value;
}

float CRF24Message::getVoltage() {
  return _msg.voltage;
}

void CRF24Message::setVoltage(float value) {
  _msg.voltage = value;
}

float CRF24Message::getTemperature() {
  return _msg.temperature;
}

void CRF24Message::setTemperature(float value) {
  _msg.temperature = value;
}

float CRF24Message::getHumidity() {
  return _msg.humidity;
}

void CRF24Message::setHumidity(float value) {
  _msg.humidity = value;
}

float CRF24Message::getBaroPressure() {
  return _msg.baro_pressure;
}

void CRF24Message::setBaroPressure(float value) {
  _msg.baro_pressure = value;
}

const String CRF24Message::getString() {
  char c[255];
  snprintf(c, 255, "[%i] (V=%DV, T=%DC, H=%D%%, BP=%DKPa U=%isec)", pipe, 
        _msg.voltage, _msg.temperature, _msg.humidity, _msg.baro_pressure/1000.0, _msg.uptime/1000.0);
  Log.noticeln("--%s", c);
  return String(c);
}