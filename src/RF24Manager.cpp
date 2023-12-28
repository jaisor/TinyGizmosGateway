#if !( defined(ESP32) ) && !( defined(ESP8266) )
  #error This code is intended to run on ESP8266 or ESP32 platform!
#endif

#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <ArduinoLog.h>

#define CE_PIN  22
#define CSN_PIN 21

#include <Arduino.h>
#include <Time.h>

#include "RF24Manager.h"
#include "Configuration.h"
#include "BaseMessage.h"

const byte thisSlaveAddress[5] = {'R', 'x', 'A', 'A', 'A'};

CRF24Manager::CRF24Manager() {  
  _radio = new RF24(CE_PIN, CSN_PIN);
  
  _radio->begin();
  _radio->setDataRate( RF24_250KBPS );
  _radio->openReadingPipe(1, thisSlaveAddress);
  _radio->startListening();

  Log.infoln("Radio listening...");
  Log.noticeln("  Channel: %i", _radio->getChannel());
  Log.noticeln("  PayloadSize: %i", _radio->getPayloadSize());
  Log.noticeln("  DataRate: %i", _radio->getDataRate());
  
  _queue.push_back(new CBaseMessage(String("test")));
}

CRF24Manager::~CRF24Manager() { 
  delete _radio;
  Log.noticeln("CRF24Manager destroyed");
}

void CRF24Manager::loop() {
  if (_radio->available()) {
    _radio->read( &_data, sizeof(_data) );
    Log.infoln("Received: '%s'", _data);
    _queue.push_back(new CBaseMessage(String(_data)));
  }
}