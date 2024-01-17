#pragma once

#include <queue>
#include "Configuration.h"
#include "BaseMessage.h"

class IMessageQueue {
public:
  virtual std::queue<CBaseMessage*> getQueue();
};
