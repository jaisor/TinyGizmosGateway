#pragma once

typedef struct r24_message_uvthp_t {
  uint8_t id; // message id
  uint32_t uptime;
  float voltage;
  float temperature;
  float humidity;
  float baro_pressure;
} _r24_message_uvtha_t;

class CRF24Message {
private:
public:
	CRF24Message();
  CRF24Message(float voltage, float temperature, float humidity, uint16_t uptime);

  static uint8_t getMessageLength() { return sizeof(r24_message_uvthp_t); }
  const void* getMessageBuffer(); 

  uint32_t getUptime();
  void setUptime(uint32_t value);

  float getVoltage();
  void setVoltage(float value);

  float getTemperature();
  void setTemperature(float value);

  float getHumidity();
  void setHumidity(float value);

  float getBaroPressure();
  void setBaroPressure(float value);
};
