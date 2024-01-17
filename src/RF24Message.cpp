#include <ArduinoLog.h>
#include <Arduino.h>

#include "RF24Message.h"

// Message Uptime-Voltage-Temperature-Humidity-BarometricPressure
#define MSG_UVTHP_ID 1

CRF24Message::CRF24Message(const u_int8_t pipe, const uint16_t uptime, const float voltage, const float temperature, const float humidity, const float baro_pressure)
: CBaseMessage(pipe) {  
  
  _msg.id = MSG_UVTHP_ID;
  setUptime(uptime);
  setVoltage(voltage);
  setTemperature(temperature);
  setHumidity(humidity);
  setBaroPressure(baro_pressure);
}

CRF24Message::CRF24Message(const u_int8_t pipe, const void* buf, const uint8_t length)
: CBaseMessage(pipe) { 
  // TODO: Validate message id
  if (length < getMessageLength()) {
    Log.warningln(F("Received message with shorter (%i) length than expected (%i)"), length, getMessageLength());
  } else if (length > getMessageLength()) {
    Log.errorln(F("Received message with longer (%i) length than expected (%i), will trim to prevent crash"), length, getMessageLength());
  }
  memcpy(&_msg, buf, length <= getMessageLength() ? length : getMessageLength());
  if (_msg.id != MSG_UVTHP_ID) {
    Log.errorln(F("Message ID mismatch %i is not MSG_UVTHP_ID"), _msg.id);
    memset(&_msg, 0, getMessageLength());
  }
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
  snprintf_P(c, 255, PSTR("[%u] (V=%0.2fV, T=%0.2fC, H=%0.2f%%, BP=%0.2fKPa U=%0.2fsec)"), pipe, 
        _msg.voltage, _msg.temperature, _msg.humidity, _msg.baro_pressure/1000.0, (float)(_msg.uptime)/1000.0);
  Log.verboseln(F("CRF24Message::getString() : %s"), c);
  return String(c);
}