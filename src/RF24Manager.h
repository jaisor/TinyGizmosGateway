#pragma once

#include <ArduinoJson.h>
#include <RF24.h>
#include <vector>

#include "BaseManager.h"
#include "MessageQueue.h"

class CRF24Manager: public CBaseManager, public IMessageQueue {

private:
  unsigned long tMillis;
  StaticJsonDocument<2048> sensorJson;
  StaticJsonDocument<2048> configJson;

  RF24 *_radio;
  char _data[32];

  std::vector<CBaseMessage*> _queue;
    
public:
	CRF24Manager();
  ~CRF24Manager();

  // CBaseManager
  virtual void loop();

  // IMessageQueue
  virtual std::vector<CBaseMessage*> getQueue() { return _queue; };
};
