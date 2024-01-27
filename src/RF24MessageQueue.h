#pragma once

#include <queue>
#include "Configuration.h"
#include "BaseMessage.h"

#define RF24_MESSAGE_LENGTH 32

class IMessageQueue {
public:
  virtual std::queue<const void*>* getQueue();
};
