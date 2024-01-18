#if !( defined(ESP32) ) && !( defined(ESP8266) )
  #error This code is intended to run on ESP8266 or ESP32 platform!
#endif

#include <Arduino.h>
#include <SPI.h>
#ifdef RADIO_RF24
  #include <nRF24L01.h>
#endif
#include <ArduinoLog.h>

#include "Configuration.h"

#define SERIAL_DEBUG
#if defined(ESP32)
  #define CE_PIN  GPIO_NUM_22
  #define CSN_PIN GPIO_NUM_21
#elif defined(ESP8266)
  #define CE_PIN  D4
  #define CSN_PIN D8
#elif defined(ARDUINO_AVR_UNO)
  #define CE_PIN  9
  #define CSN_PIN 10
#elif defined(SEEED_XIAO_M0)
  #define CE_PIN  D2
  #define CSN_PIN D3
#endif

#include <Arduino.h>
#include <Time.h>

#include "RF24Manager.h"
#include "Configuration.h"
#include "BaseMessage.h"
#include "RF24Message.h"

CRF24Manager::CRF24Manager() {  
  error = false;
#ifdef RADIO_RF24
  _radio = new RF24(CE_PIN, CSN_PIN);
  
  if (!_radio->begin()) {
    Log.errorln("Failed to initialize RF24 radio");
    error = true;
    return;
  }
  
  _radio->setAddressWidth(5);
  _radio->setDataRate((rf24_datarate_e)configuration.rf24_data_rate);
  _radio->setPALevel(configuration.rf24_pa_level);
  _radio->setChannel(configuration.rf24_channel);
  _radio->setPayloadSize(CRF24Message::getMessageLength());
  for (uint8_t i=0; i<6; i++) {
    char a[6];
    snprintf_P(a, 6, "%i%s", i, configuration.rf24_pipe_suffix);
    Log.noticeln("Opening reading pipe %i on address '%s'", i, a);
    _radio->openReadingPipe(i, (uint8_t*)a);
  }
  _radio->setRetries(15, 15);
  _radio->startListening();

  Log.infoln("Radio initialized");
  if (Log.getLevel() >= LOG_LEVEL_NOTICE) {
    Log.noticeln("  Channel: %i", _radio->getChannel());
    Log.noticeln("  DataRate: %i", _radio->getDataRate());
    Log.noticeln("  PALevel: %i", _radio->getPALevel());
    Log.noticeln("  PayloadSize: %i", _radio->getPayloadSize());

    if (Log.getLevel() >= LOG_LEVEL_VERBOSE) {
      char buffer[870] = {'\0'};
      uint16_t used_chars = _radio->sprintfPrettyDetails(buffer);
      Log.verboseln(buffer);
    }
  }
#endif
}

CRF24Manager::~CRF24Manager() { 
#ifdef RADIO_RF24
  delete _radio;
#endif
  Log.noticeln("CRF24Manager destroyed");
}

void CRF24Manager::loop() {
#ifdef RADIO_RF24
  uint8_t pipe;
  if (_radio->available(&pipe)) {
    intLEDOn();
    uint8_t bytes = _radio->getPayloadSize();
    if (bytes == CRF24Message::getMessageLength()) {
      uint8_t buf[bytes];
      _radio->read(&buf, bytes);
      CRF24Message *msg = new CRF24Message(pipe, &buf, bytes);
      if (!msg->isError()) {
        Log.infoln(F("Received %i bytes message: %s adding to queue of size %i"), bytes, msg->getString().c_str(), _queue.size());
        _queue.push(msg);
      }
    } else {
      //Log.warningln(F("Received message length %u != expected %u, ignoring"), bytes, CRF24Message::getMessageLength());
    }
    intLEDOff();
  }
#endif
}