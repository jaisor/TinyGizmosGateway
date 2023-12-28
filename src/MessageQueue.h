#pragma once

#include <vector>
#include "Configuration.h"
#include "BaseMessage.h"

class IMessageQueue {
public:
  virtual std::vector<CBaseMessage*> getQueue();
};
