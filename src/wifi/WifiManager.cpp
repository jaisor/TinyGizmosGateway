#if !( defined(ESP32) ) && !( defined(ESP8266) )
  #error This code is intended to run on ESP8266 or ESP32 platform!
#endif

#include <Arduino.h>
#include <WiFiClient.h>
#include <Time.h>
#include <ezTime.h>
#include <AsyncElegantOTA.h>
#include <StreamUtils.h>
#include <RF24.h>

#include "wifi/WifiManager.h"
#include "Configuration.h"
#include "RF24Message.h"

#define MAX_CONNECT_TIMEOUT_MS 15000 // 10 seconds to connect before creating its own AP
#define POST_UPDATE_INTERVAL 300000 // Every 5 min

const int RSSI_MAX =-50;// define maximum straighten of signal in dBm
const int RSSI_MIN =-100;// define minimum strength of signal in dBm

WiFiClient espClient;

int dBmtoPercentage(int dBm) {
  int quality;
  if(dBm <= RSSI_MIN) {
    quality = 0;
  } else if(dBm >= RSSI_MAX) {  
    quality = 100;
  } else {
    quality = 2 * (dBm + 100);
  }
  return quality;
}

const String htmlTop = F("<html>\
  <head>\
  <title>%s</title>\
  <style>\
    body { background-color: #303030; font-family: 'Anaheim',sans-serif; Color: #d8d8d8; }\
    input, select { margin-bottom: 0.4em; }\
  </style>\
  </head>\
  <body>\
  <h1>%s - Tiny Gizmos Radio Gateway</h1>");

const String htmlBottom = F("<p><b>%s</b><br>\
  Uptime: <b>%02d:%02d:%02d</b><br/>\
  WiFi signal strength: <b>%i%%</b><br/>\
  RF messages in queue: <b>%i</b><br/>\
  </p></body>\
</html>");

const String htmlWifiApConnectForm = F("<hr><h2>Connect to WiFi Access Point (AP)</h2>\
  <form method='POST' action='/connect' enctype='application/x-www-form-urlencoded'>\
    <label for='ssid'>SSID (AP Name):</label><br>\
    <input type='text' id='ssid' name='ssid'><br><br>\
    <label for='pass'>Password (WPA2):</label><br>\
    <input type='password' id='pass' name='password' minlength='8' autocomplete='off' required><br><br>\
    <input type='submit' value='Connect...'>\
  </form>");

const String htmlDeviceConfigs = F("<hr><h2>Configs</h2>\
  <form method='POST' action='/config' enctype='application/x-www-form-urlencoded'>\
    <label for='deviceName'>Device name:</label><br>\
    <input type='text' id='deviceName' name='deviceName' value='%s'><br>\
    <br>\
    <label for='mqttServer'>MQTT server:</label><br>\
    <input type='text' id='mqttServer' name='mqttServer' value='%s'><br>\
    <label for='mqttPort'>MQTT port:</label><br>\
    <input type='text' id='mqttPort' name='mqttPort' value='%u'><br>\
    <label for='mqttTopic'>MQTT topic:</label><br>\
    <input type='text' id='mqttTopic' name='mqttTopic' value='%s'><br>\
    <br>\
    <h3>Radio Settings</h3>\
    <label for='rf24_channel'>Channel:</label><br>\
    <input type='text' id='rf24_channel' name='rf24_channel' value='%u' size='3' maxlength='3'> <small>(0-125)</small><br>\
    <label for='rf24_data_rate'>Data rate:</label><br>\
    <select name='rf24_data_rate' id='rf24_data_rate'>\
    %s\
    </select><br>\
    <label for='rf24_pa_level'>Power amplifier level:</label><br>\
    <select name='rf24_pa_level' id='rf24_pa_level'>\
    %s\
    </select><br>\
    <label for='rf24_pipe_suffix'>Pipe name suffix:</label><br>\
    <input type='text' id='rf24_pipe_suffix' name='rf24_pipe_suffix' value='%s' minlength='4' maxlength='4' size='4'><br>\
    %s\
    <br>\
    <input type='submit' value='Set...'>\
  </form>");

const String htmlRF24MQTTTopicRow = F("<label for='ssid'>%i pipe MQTT topic:</label><br>\
    <input type='text' id='ssid' name='ssid'><br>");

CWifiManager::CWifiManager(IMessageQueue *messageQueue): 
rebootNeeded(false), wifiRetries(0), messageQueue(messageQueue) {  

  sensorJson["gw_name"] = configuration.name;

  strcpy(SSID, configuration.wifiSsid);
  server = new AsyncWebServer(WEB_SERVER_PORT);
  mqtt.setClient(espClient);
  connect();
}

void CWifiManager::connect() {

  status = WF_CONNECTING;
  strcpy(softAP_SSID, "");
  tMillis = millis();

  uint32_t deviceId = CONFIG_getDeviceId();
  sensorJson["gw_deviceId"] = deviceId;
  Log.infoln("Device ID: '%i'", deviceId);

  if (strlen(SSID)) {

    // Join AP from Config
    Log.infoln("Connecting to WiFi: '%s'", SSID);
    WiFi.begin(SSID, configuration.wifiPassword);
    wifiRetries = 0;

  } else {

    // Create AP using fallback and chip ID
    sprintf_P(softAP_SSID, "%s_%i", WIFI_FALLBACK_SSID, deviceId);
    Log.infoln("Creating WiFi: '%s' / '%s'", softAP_SSID, WIFI_FALLBACK_PASS);

    if (WiFi.softAP(softAP_SSID, WIFI_FALLBACK_PASS)) {
      wifiRetries = 0;
      Log.infoln("Wifi AP '%s' created, listening on '%s'", softAP_SSID, WiFi.softAPIP().toString().c_str());
    } else {
      Log.errorln("Wifi AP faliled");
    };

  }
  
}

void CWifiManager::listen() {

  status = WF_LISTENING;

  // Web
  server->on("/", std::bind(&CWifiManager::handleRoot, this, std::placeholders::_1));
  server->on("/connect", HTTP_POST, std::bind(&CWifiManager::handleConnect, this, std::placeholders::_1));
  server->on("/config", HTTP_POST, std::bind(&CWifiManager::handleConfig, this, std::placeholders::_1));
  server->on("/factory_reset", HTTP_POST, std::bind(&CWifiManager::handleFactoryReset, this, std::placeholders::_1));
  server->begin();
  Log.infoln("Web server listening on %s port %i", WiFi.localIP().toString().c_str(), WEB_SERVER_PORT);
  
  sensorJson["gw_ip"] = WiFi.localIP();

  // NTP
  Log.infoln("Configuring time from %s at %i (%i)", configuration.ntpServer, configuration.gmtOffset_sec, configuration.daylightOffset_sec);
  configTime(configuration.gmtOffset_sec, configuration.daylightOffset_sec, configuration.ntpServer);
  struct tm timeinfo;
  if(getLocalTime(&timeinfo)){
    Log.noticeln("The time is %i:%i", timeinfo.tm_hour,timeinfo.tm_min);
  }

  // OTA
  AsyncElegantOTA.begin(server);

  // MQTT
  mqtt.setServer(configuration.mqttServer, configuration.mqttPort);

  using std::placeholders::_1;
  using std::placeholders::_2;
  using std::placeholders::_3;
  mqtt.setCallback(std::bind( &CWifiManager::mqttCallback, this, _1,_2,_3));

  if (strlen(configuration.mqttServer) && strlen(configuration.mqttTopic) && !mqtt.connected()) {
    Log.noticeln("Attempting MQTT connection to '%s:%i' ...", configuration.mqttServer, configuration.mqttPort);
    if (mqtt.connect(String(CONFIG_getDeviceId()).c_str())) {
      Log.noticeln("MQTT connected");
      
      sprintf_P(mqttSubcribeTopicConfig, "%s/%u/config", configuration.mqttTopic, CONFIG_getDeviceId());
      bool r = mqtt.subscribe(mqttSubcribeTopicConfig);
      Log.noticeln("Subscribed for config changes to MQTT topic '%s' success = %T", mqttSubcribeTopicConfig, r);

      postSensorUpdate();
    } else {
      Log.warningln("MQTT connect failed, rc=%i", mqtt.state());
    }
  }
}

void CWifiManager::loop() {

  if (rebootNeeded && millis() - tMillis > 300) {
    Log.noticeln("Rebooting...");
  #ifdef ESP32
    ESP.restart();
  #elif ESP8266
    ESP.reset();
  #endif
  return;
  }

  if (WiFi.status() == WL_CONNECTED || isApMode() ) {
    // WiFi is connected

    if (status != WF_LISTENING) {  
      // Start listening for requests
      listen();
      return;
    }

    mqtt.loop();

    if (!isApMode() && millis() - tMillis > POST_UPDATE_INTERVAL &&
      strlen(configuration.mqttServer) && strlen(configuration.mqttTopic) && mqtt.connected()) {
      tMillis = millis();
      postSensorUpdate();
    }

    processQueue();

  } else if (WiFi.status() == WL_NO_SSID_AVAIL && !isApMode()) {
    // Can't find desired AP

    if (millis() - tMillis > MAX_CONNECT_TIMEOUT_MS) {
      tMillis = millis();
      if (++wifiRetries > 1) {
        Log.warningln("Failed to find previous AP (wifi status %i) after %l ms, create an AP instead", WiFi.status(), (millis() - tMillis));
        strcpy(SSID, "");
        WiFi.disconnect(false, true);
        connect();
      } else {
        Log.warningln("Can't find previous AP (wifi status %i) trying again attempt: %i", WiFi.status(), wifiRetries);
      }
      //Log.infoln("WifiMode == %i", WiFi.getMode());
    }
  } else {
  // WiFi is down

  switch (status) {
    case WF_LISTENING: {
    Log.infoln("Disconnecting %i", status);
    server->end();
    status = WF_CONNECTING;
    connect();
    } break;
    case WF_CONNECTING: {
      if (millis() - tMillis > MAX_CONNECT_TIMEOUT_MS) {
        tMillis = millis();
        if (++wifiRetries > 3) {
          Log.warningln("Connecting failed (wifi status %i) after %l ms, create an AP instead", WiFi.status(), (millis() - tMillis));
          strcpy(SSID, "");
        }
        connect();
      }
    } break;

  }

  }
  
}

void CWifiManager::handleRoot(AsyncWebServerRequest *request) {

  Log.infoln("handleRoot");

  AsyncResponseStream *response = request->beginResponseStream("text/html");
  printHTMLTop(response);

  if (isApMode()) {
    response->printf(htmlWifiApConnectForm.c_str());
  } else {
    response->printf("<p>Connected to '%s'</p>", SSID);
  }

  char rfDataRate[130];
  snprintf_P(rfDataRate, 130, PSTR("<option %s value='0'>1MBPS</option>\
    <option %s value='1'>2MBPS</option>\
    <option %s value='2'>250KBPS</option>"), 
    configuration.rf24_data_rate == RF24_1MBPS ? "selected" : "", 
    configuration.rf24_data_rate == RF24_2MBPS ? "selected" : "", 
    configuration.rf24_data_rate == RF24_250KBPS ? "selected" : "");

  char rfPALevel[210];
  snprintf_P(rfPALevel, 210, PSTR("<option %s value='0'>Min</option>\
    <option %s value='1'>Low</option>\
    <option %s value='2'>High</option>\
    <option %s value='3'>Max</option>"), 
    configuration.rf24_pa_level == RF24_PA_MIN ? "selected" : "", 
    configuration.rf24_pa_level == RF24_PA_LOW ? "selected" : "", 
    configuration.rf24_pa_level == RF24_PA_HIGH ? "selected" : "",
    configuration.rf24_pa_level == RF24_PA_MAX ? "selected" : "");

  String mqttTopicPipes = "";
  for (int i=0; i<6; i++) {
    char c[255];
    snprintf_P(c, 255, PSTR("<label for='pipe_%i_mqttTopic'>Pipe %i MQTT topic:</label><br>\
      <input type='text' id='pipe_%i_mqttTopic' name='pipe_%i_mqttTopic' value='%s'><br>"),
      i, i+1, i, i, configuration.rf24_pipe_mqttTopic[i]
      );
    mqttTopicPipes += String(c);
  }

  response->printf(htmlDeviceConfigs.c_str(), configuration.name,
    configuration.mqttServer, configuration.mqttPort, configuration.mqttTopic,
    configuration.rf24_channel, rfDataRate, rfPALevel, configuration.rf24_pipe_suffix, mqttTopicPipes.c_str()
  );

  printHTMLBottom(response);
  request->send(response);
}

void CWifiManager::handleConnect(AsyncWebServerRequest *request) {

  Log.infoln("handleConnect");

  String ssid = request->arg("ssid");
  String password = request->arg("password");
  
  AsyncResponseStream *response = request->beginResponseStream("text/html");
  
  printHTMLTop(response);
  response->printf("<p>Connecting to '%s' ... see you on the other side!</p>", ssid.c_str());
  printHTMLBottom(response);

  request->send(response);

  ssid.toCharArray(configuration.wifiSsid, sizeof(configuration.wifiSsid));
  password.toCharArray(configuration.wifiPassword, sizeof(configuration.wifiPassword));

  Log.noticeln("Saved config SSID: '%s'", configuration.wifiSsid);

  EEPROM_saveConfig();

  strcpy(SSID, configuration.wifiSsid);
  WiFi.disconnect(true, true);
  tMillis = millis();
  rebootNeeded = true;
}

void CWifiManager::handleConfig(AsyncWebServerRequest *request) {

  Log.infoln("handleConfig");

  String deviceName = request->arg("deviceName");
  deviceName.toCharArray(configuration.name, sizeof(configuration.name));
  Log.infoln("Device req name: %s", deviceName);
  Log.infoln("Device size %i name: %s", sizeof(configuration.name), configuration.name);

  String mqttServer = request->arg("mqttServer");
  mqttServer.toCharArray(configuration.mqttServer, sizeof(configuration.mqttServer));
  Log.infoln("MQTT Server: %s", mqttServer);

  uint16_t mqttPort = atoi(request->arg("mqttPort").c_str());
  configuration.mqttPort = mqttPort;
  Log.infoln("MQTT Port: %u", mqttPort);

  String mqttTopic = request->arg("mqttTopic");
  mqttTopic.toCharArray(configuration.mqttTopic, sizeof(configuration.mqttTopic));
  Log.infoln("MQTT Topic: %s", mqttTopic);

  uint8_t rf24_channel = atoi(request->arg("rf24_channel").c_str());
  configuration.rf24_channel = rf24_channel;
  Log.infoln("RF24 Channel: %u", rf24_channel);

  uint8_t rf24_data_rate = atoi(request->arg("rf24_data_rate").c_str());
  configuration.rf24_data_rate = rf24_data_rate;
  Log.infoln("RF24 Data Rate: %u", rf24_data_rate);

  uint8_t rf24_pa_level = atoi(request->arg("rf24_pa_level").c_str());
  configuration.rf24_pa_level = rf24_pa_level;
  Log.infoln("RF24 PA Level: %u", rf24_pa_level);

  String rf24_pipe_suffix = request->arg("rf24_pipe_suffix");
  rf24_pipe_suffix.toCharArray(configuration.rf24_pipe_suffix, sizeof(configuration.rf24_pipe_suffix));
  Log.infoln("RF24 Pipe Suffix: %s", rf24_pipe_suffix);

  for (int i=0; i<6; i++) {
    String r = request->arg("pipe_" + String(i) + "_mqttTopic");
    r.toCharArray(configuration.rf24_pipe_mqttTopic[i], sizeof(configuration.rf24_pipe_mqttTopic[i]));
  }

  EEPROM_saveConfig();
  
  request->redirect("/");
  tMillis = millis();
  rebootNeeded = true;
}

void CWifiManager::handleFactoryReset(AsyncWebServerRequest *request) {
  Log.infoln("handleFactoryReset");
  
  AsyncResponseStream *response = request->beginResponseStream("text/html");
  response->setCode(200);
  response->printf("OK");

  EEPROM_wipe();
  tMillis = millis();
  rebootNeeded = true;
  
  request->send(response);
}

void CWifiManager::postSensorUpdate() {

  if (!mqtt.connected()) {
    if (mqtt.state() < MQTT_CONNECTED 
      && strlen(configuration.mqttServer) && strlen(configuration.mqttTopic)) { // Reconnectable
      Log.noticeln("Attempting to reconnect from MQTT state %i at '%s:%i' ...", mqtt.state(), configuration.mqttServer, configuration.mqttPort);
      if (mqtt.connect(String(CONFIG_getDeviceId()).c_str())) {
        Log.noticeln("MQTT reconnected");
        sprintf_P(mqttSubcribeTopicConfig, "%s/%u/config", configuration.mqttTopic, CONFIG_getDeviceId());
        bool r = mqtt.subscribe(mqttSubcribeTopicConfig);
        Log.noticeln("Subscribed for config changes to MQTT topic '%s' success = %T", mqttSubcribeTopicConfig, r);
      } else {
        Log.warningln("MQTT reconnect failed, rc=%i", mqtt.state());
      }
    }
    if (!mqtt.connected()) {
      Log.noticeln("MQTT not connected %i", mqtt.state());
      return;
    }
  }

  if (!strlen(configuration.mqttTopic)) {
    Log.warningln("Blank MQTT topic");
    return;
  }

  char topic[255];
  bool current = false;
  float v; int iv;

  iv = dBmtoPercentage(WiFi.RSSI());
  sensorJson["gw_wifi_percent"] = iv;
  sensorJson["gw_wifi_rssi"] = WiFi.RSSI();

  time_t now; 
  time(&now);
  unsigned long uptimeMillis = CONFIG_getUpTime();

  sensorJson["gw_uptime_millis"] = uptimeMillis;
  // Convert to ISO8601 for JSON
  char buf[sizeof "2011-10-08T07:07:09Z"];
  strftime(buf, sizeof buf, "%FT%TZ", gmtime(&now));
  sensorJson["timestamp_iso8601"] = String(buf);

  sensorJson["mqttConfigTopic"] = mqttSubcribeTopicConfig;
  sensorJson["rf24_data_rate"] = configuration.rf24_data_rate;
  sensorJson["rf24_pa_level"] = configuration.rf24_pa_level;
  sensorJson["rf24_pipe_suffix"] = configuration.rf24_pipe_suffix;
  sensorJson["rf_msq_queue_size"] = messageQueue->getQueue().size();

  // sensor Json
  sprintf_P(topic, "%s/json", configuration.mqttTopic);
  mqtt.beginPublish(topic, measureJson(sensorJson), false);
  BufferingPrint bufferedClient(mqtt, 32);
  serializeJson(sensorJson, bufferedClient);
  bufferedClient.flush();
  mqtt.endPublish();

  String jsonStr;
  serializeJson(sensorJson, jsonStr);
  Log.noticeln("Sent '%s' json to MQTT topic '%s'", jsonStr.c_str(), topic);
}

bool CWifiManager::isApMode() { 
  return WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_MODE_APSTA; 
}

void CWifiManager::mqttCallback(char *topic, uint8_t *payload, unsigned int length) {

  if (length == 0) {
    return;
  }

  Log.noticeln("Received %u bytes message on MQTT topic '%s'", length, topic);
  if (!strcmp(topic, mqttSubcribeTopicConfig)) {
    deserializeJson(configJson, (const byte*)payload, length);

    String jsonStr;
    serializeJson(configJson, jsonStr);
    Log.noticeln("Received configuration over MQTT with json: '%s'", jsonStr.c_str());

    if (configJson.containsKey("name")) {
      strncpy(configuration.name, configJson["name"], 128);
    }

    if (configJson.containsKey("mqttTopic")) {
      strncpy(configuration.mqttTopic, configJson["mqttTopic"], 64);
    }

    // Delete the config message in case it was retained
    mqtt.publish(mqttSubcribeTopicConfig, NULL, 0, true);
    Log.noticeln("Deleted config message");

    EEPROM_saveConfig();
    postSensorUpdate();
  }
  
}

void CWifiManager::printHTMLTop(Print *p) {
  p->printf(htmlTop.c_str(), configuration.name, configuration.name);
}

void CWifiManager::printHTMLBottom(Print *p) {
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  p->printf(htmlBottom.c_str(), String(DEVICE_NAME), hr, min % 60, sec % 60, dBmtoPercentage(WiFi.RSSI()), messageQueue->getQueue());
}

void CWifiManager::processQueue() {
  std::queue<CBaseMessage*> q = messageQueue->getQueue();
  while(!q.empty()) {
    CRF24Message *msg = (CRF24Message *)q.front();

    if (!strlen(configuration.rf24_pipe_mqttTopic[msg->getPipe()])) {
      Log.warning("Message received on a pipe with blank MPTT topic: %s", msg->getString());
    }
    /*
    // sensor Json
    sprintf_P(topic, "%s/json", configuration.mqttTopic);
    mqtt.beginPublish(topic, measureJson(sensorJson), false);
    BufferingPrint bufferedClient(mqtt, 32);
    serializeJson(sensorJson, bufferedClient);
    bufferedClient.flush();
    mqtt.endPublish();
    */
    
    q.pop();
  }
}