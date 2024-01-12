#pragma once

#include <ArduinoJson.h>
#include <vector>

typedef struct r24_message_float_row_t { 
    char key;
    float value;
} S_r24_message_float_row_t;

typedef struct r24_message_uint16_row_t { 
    char key;
    uint16_t value;
} S_r24_message_uint16_row_t;

typedef struct r24_message_t { 
    r24_message_float_row_t voltage;
    r24_message_float_row_t temperature;
    r24_message_float_row_t humidity;
    r24_message_uint16_row_t uptime;
} S_r24_message_t;

class CRF24Message {
private:
  r24_message_t _msg;
public:
	CRF24Message();
  CRF24Message(float voltage, float temperature, float humidity, uint16_t uptime);

  static uint8_t getMessageLength() { return sizeof(_msg); }
  const void* getMessageBuffer(); 

  float getVoltage();
  void setVoltage(float value);

  float getTemperature();
  void setTemperature(float value);

  float getHumidity();
  void setHumidity(float value);

  uint16_t getUptime();
  void setUptime(uint16_t value);
};
