#pragma once

#include <ArduinoJson.h>
#include <RF24.h>
#include <queue>

#include "BaseManager.h"
#include "MessageQueue.h"

class CRF24Manager: public CBaseManager, public IMessageQueue {

private:
  unsigned long tMillis;
  StaticJsonDocument<2048> sensorJson;
  StaticJsonDocument<2048> configJson;

  RF24 *_radio;

  std::queue<CBaseMessage*> _queue;
    
public:
	CRF24Manager();
  ~CRF24Manager();

  // CBaseManager
  virtual void loop();

  // IMessageQueue
  virtual std::queue<CBaseMessage*>* getQueue() { return &_queue; };
};
