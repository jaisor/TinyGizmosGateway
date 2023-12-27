#pragma once

#include <Arduino.h>
#include <functional>
#include <ArduinoLog.h>

#define WIFI    // 2.4Ghz wifi access point

#define EEPROM_FACTORY_RESET 0       // Byte to be used for factory reset device fails to start or is rebooted within 1 sec 3 consequitive times
#define EEPROM_CONFIGURATION_START 1   // First EEPROM byte to be used for storing the configuration

#define FACTORY_RESET_CLEAR_TIMER_MS 2000   // Clear factory reset counter when elapsed, considered smooth boot

#ifdef ESP32
  #define DEVICE_NAME "ESP32RFGW"
#elif ESP8266
  #define DEVICE_NAME "ESP8266RFGW"
#endif

#ifdef WIFI
  #define WIFI_SSID DEVICE_NAME
  #define WIFI_PASS "password123"

  // If unable to connect, it will create a soft accesspoint
  #define WIFI_FALLBACK_SSID DEVICE_NAME // device chip id will be suffixed
  #define WIFI_FALLBACK_PASS "password123"

  #define NTP_SERVER "pool.ntp.org"
  #define NTP_GMT_OFFSET_SEC -25200
  #define NTP_DAYLIGHT_OFFSET_SEC 0

  // Web server
  #define WEB_SERVER_PORT 80
#endif

#define BATTERY_SENSOR  // ADC A0 using 0-3.3v voltage divider
#ifdef BATTERY_SENSOR
  #define BATTERY_SENSOR_ADC_PIN  A0
#endif

#define INTERNAL_LED_PIN LED_BUILTIN

struct configuration_t {

  #ifdef WIFI
    char wifiSsid[32];
    char wifiPassword[63];
    
    // ntp
    char ntpServer[128];
    long gmtOffset_sec;
    int daylightOffset_sec;

    // mqtt
    char mqttServer[128];
    uint16_t mqttPort;
    char mqttTopic[128];
  #endif

  char name[128];

  char _loaded[7]; // used to check if EEPROM was correctly set
};

extern configuration_t configuration;

uint8_t EEPROM_initAndCheckFactoryReset();
void EEPROM_clearFactoryReset();

void EEPROM_saveConfig();
void EEPROM_loadConfig();
void EEPROM_wipe();

uint32_t CONFIG_getDeviceId();
unsigned long CONFIG_getUpTime();