#pragma once

#include <ArduinoJson.h>
#ifdef RADIO_RF24
  #include <RF24.h>
#endif
#include <queue>

#include "BaseManager.h"
#include "RF24MessageQueue.h"

class CRF24Manager: public CBaseManager, public IMessageQueue {

private:
  unsigned long tMillis;
  StaticJsonDocument<2048> sensorJson;
  StaticJsonDocument<2048> configJson;
  bool error;

#ifdef RADIO_RF24
  RF24 *radio;
#endif

  std::queue<const void*> queue;
    
public:
	CRF24Manager();
  ~CRF24Manager();

  // CBaseManager
  virtual void loop();

  // IMessageQueue
  virtual std::queue<const void*>* getQueue() { return &queue; };
  virtual const bool isError() { return error; }
};
