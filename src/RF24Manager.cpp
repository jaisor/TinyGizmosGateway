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

#include <BaseMessage.h>
#include <RF24Message.h>
#include <RF24Message_VED_INV.h>
#include <RF24Message_VED_MPPT.h>
#include <RF24Message_VED_BATT.h>
#include <RF24Message_VED_BATT_SUP.h>

#include "RF24Manager.h"
#include "Configuration.h"

CRF24Manager::CRF24Manager() {  
  error = false;
#ifdef RADIO_RF24
  radio = new RF24(CE_PIN, CSN_PIN);
  
  if (!radio->begin()) {
    Log.errorln("Failed to initialize RF24 radio");
    error = true;
    return;
  }

  uint8_t maxMessageSize = 32;
  Log.infoln("Message size: %u", maxMessageSize);
  
  radio->setAddressWidth(5);
  radio->setDataRate((rf24_datarate_e)configuration.rf24_data_rate);
  radio->setPALevel(configuration.rf24_pa_level);
  radio->setChannel(configuration.rf24_channel);
  radio->setPayloadSize(maxMessageSize);
  for (uint8_t i=0; i<6; i++) {
    char a[6];
    snprintf_P(a, 6, "%i%s", i, configuration.rf24_pipe_suffix);
    Log.noticeln("Opening reading pipe %i on address '%s'", i, a);
    radio->openReadingPipe(i, (uint8_t*)a);
  }
  radio->setRetries(15, 15);
  radio->setAutoAck(false);
  radio->startListening();

  Log.infoln("Radio initialized");
  if (Log.getLevel() >= LOG_LEVEL_NOTICE) {
    Log.noticeln("  Channel: %i", radio->getChannel());
    Log.noticeln("  DataRate: %i", radio->getDataRate());
    Log.noticeln("  PALevel: %i", radio->getPALevel());
    Log.noticeln("  PayloadSize: %i", radio->getPayloadSize());

    if (Log.getLevel() >= LOG_LEVEL_VERBOSE) {
      char buffer[870] = {'\0'};
      radio->sprintfPrettyDetails(buffer);
      Log.verboseln(buffer);
    }
  }
#endif
}

CRF24Manager::~CRF24Manager() { 
#ifdef RADIO_RF24
  delete radio;
#endif
  Log.noticeln("CRF24Manager destroyed");
}

void CRF24Manager::loop() {
#ifdef RADIO_RF24
  uint8_t pipe;
  if (radio->available(&pipe)) {
    intLEDOn();
    uint8_t bytes = radio->getPayloadSize();
    if (bytes == RF24_MESSAGE_LENGTH) {
      uint8_t *buf = new uint8_t[bytes];
      radio->read(buf, bytes);
      
      Log.infoln(F("Pipe %i received %i bytes, adding to queue of size %i"), pipe, bytes, queue.size());

      CBaseMessage *msg;
      switch (static_cast<uint8_t>(buf[0])) {
        case MSG_UVTHP_ID:
          msg = new CRF24Message(pipe, buf, bytes);
          break;
        case MSG_VED_INV_ID:
          msg = new CRF24Message_VED_INV(pipe, buf, bytes);
          break;
        case MSG_VED_MPPT_ID:
          msg = new CRF24Message_VED_MPPT(pipe, buf, bytes);
          break;
        case MSG_VED_BATT_ID:
          msg = new CRF24Message_VED_BATT(pipe, buf, bytes);
          break;
        case MSG_VED_BATT_SUP_ID:
          msg = new CRF24Message_VED_BATT_SUP(pipe, buf, bytes);
          break;
        default:
          Log.warningln("Unsupported message ID %i received on pipe %i", static_cast<uint8_t>(buf[0]), pipe);

      }
      queue.push(msg);
    } else {
      //Log.warningln(F("Received message length %u != expected %u, ignoring"), bytes, CRF24Message::getMessageLength());
    }
    intLEDOff();
  }
#endif
}