#pragma once

#include <Arduino.h>

#include "Configuration.h"

class CBaseMessage {

protected:
    unsigned long tMillis;
    const String raw;

public:
	CBaseMessage(const String raw);

    virtual void something() {};

    const String getString() { return raw; }
};
