#if !( defined(ESP32) ) && !( defined(ESP8266) )
  #error This code is intended to run on ESP8266 or ESP32 platform!
#endif

#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <ArduinoLog.h>

#include "Configuration.h"

#define SERIAL_DEBUG
#ifdef ESP32
  #define CE_PIN  GPIO_NUM_22
  #define CSN_PIN GPIO_NUM_21
#elif ARDUINO_AVR_UNO
  #define CE_PIN  9
  #define CSN_PIN 10
#elif SEEED_XIAO_M0
  #define CE_PIN  D2
  #define CSN_PIN D3
#endif

#include <Arduino.h>
#include <Time.h>
#include <printf.h>

#include "RF24Manager.h"
#include "Configuration.h"
#include "BaseMessage.h"
#include "RF24Message.h"

const uint8_t addresses[][6] = {"JAIGW", "TSIAJ", "FSIAJ", "FSIAJ"};

CRF24Manager::CRF24Manager() {  
  _radio = new RF24(CE_PIN, CSN_PIN);
  
  if (!_radio->begin()) {
    Log.errorln("Failed to initialize RF24 radio");
    return;
  }
  
  _radio->setAddressWidth(5);
  _radio->setDataRate((rf24_datarate_e)configuration.rf24_data_rate);
  _radio->setPALevel(configuration.rf24_pa_level);
  _radio->setChannel(configuration.rf24_channel);
  _radio->setPayloadSize(CRF24Message::getMessageLength());
  _radio->openReadingPipe(1, addresses[1]);
  _radio->setRetries(15, 15);
  _radio->startListening();

  Log.infoln("Radio initialized...");
  Log.noticeln("  Channel: %i", _radio->getChannel());
  Log.noticeln("  DataRate: %i", _radio->getDataRate());
  Log.noticeln("  PALevel: %i", _radio->getPALevel());
  Log.noticeln("  PayloadSize: %i", _radio->getPayloadSize());

  char buffer[870] = {'\0'};
  uint16_t used_chars = _radio->sprintfPrettyDetails(buffer);
  Log.noticeln(buffer);

  _queue.push_back(new CBaseMessage(String("test")));
}

CRF24Manager::~CRF24Manager() { 
  delete _radio;
  Log.noticeln("CRF24Manager destroyed");
}

void CRF24Manager::loop() {
  uint8_t pipe;
  if (_radio->available(&pipe)) {
    intLEDOn();
    uint8_t bytes = _radio->getPayloadSize();
    if (bytes == CRF24Message::getMessageLength()) {
      CRF24Message msg;
      _radio->read(&msg, bytes);
      Log.infoln("Received %i bytes on pipe %i (V=%D, T=%D, H=%D, U=%i)", bytes, pipe, 
        msg.getVoltage(), msg.getTemperature(), msg.getHumidity(), msg.getUptime());
      //_queue.push_back(new CBaseMessage(String(_data)));
    } else {
      Log.warningln("Received message length %u != expected %u, ignoring", CRF24Message::getMessageLength(), bytes);
    }
    intLEDOff();
  }
}