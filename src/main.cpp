#include <Arduino.h>
//#include <functional>
#include <ArduinoLog.h>

#if !( defined(ESP32) ) && !( defined(ESP8266) )
  #error This code is intended to run on ESP8266 platform! Please check your Tools->Board setting.
#endif

#include "Configuration.h"
#include "wifi/WifiManager.h"
#include "RF24Manager.h"

#ifdef ESP32
#elif ESP8266
  ADC_MODE(ADC_TOUT);
#endif

CWifiManager *wifiManager;
CRF24Manager *rf24Manager;

unsigned long tsSmoothBoot;
bool smoothBoot;
unsigned long tsMillisBooted;

void setup() {
  Serial.begin(115200);  while (!Serial); delay(200);
  randomSeed(analogRead(0));

  //Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  Log.begin(LOG_LEVEL_NOTICE, &Serial);
  Log.noticeln("Initializing...");  

  pinMode(INTERNAL_LED_PIN, OUTPUT);
  intLEDOn();

  if (EEPROM_initAndCheckFactoryReset() >= 3) {
    Log.warningln("Factory reset conditions met!");
    EEPROM_wipe();  
  }

  tsSmoothBoot = millis();
  smoothBoot = false;

  EEPROM_loadConfig();

  rf24Manager = new CRF24Manager();
  wifiManager = new CWifiManager(rf24Manager);

  Log.infoln("Initialized");
}

void loop() {
  
  if (!smoothBoot && millis() - tsSmoothBoot > FACTORY_RESET_CLEAR_TIMER_MS) {
    smoothBoot = true;
    EEPROM_clearFactoryReset();
    tsMillisBooted = millis();
    intLEDOff();
    Log.noticeln("Device booted smoothly!");
  }

  rf24Manager->loop();
  wifiManager->loop();

  if (wifiManager->isRebootNeeded()) {
    return;
  }
 
  //delay(5);
  yield();
}