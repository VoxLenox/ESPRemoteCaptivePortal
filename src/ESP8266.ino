#include <EEPROM.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <StreamUtils.h>

const uint64_t CURRENT_DATA_VERSION = 2;
const int SETTING_DATA_ADDRESS = 0;

EepromStream eepromStream(0, SPI_FLASH_SEC_SIZE);
DynamicJsonDocument settings(SPI_FLASH_SEC_SIZE);

String getSerializedSettingData() {
  String serializedSettingData;
  serializeJson(settings, serializedSettingData);
  return serializedSettingData;
}

void saveSettings() {
  serializeJson(settings, eepromStream);
  EEPROM.commit();
}

void resetSettings() {
    settings["dataVersion"]        = CURRENT_DATA_VERSION;

    settings["apSSID"]             = "ESP8266"; // 32
    settings["apPassword"]         = "12345678"; // 63
    settings["auth"]               = false;
    settings["hidden"]             = false;
    settings["channel"]            = 1;
    settings["maxConnections"]     = 4;
    settings["beaconInterval"]     = 100;

    settings["apLocalIP"][0]       = 192;
    settings["apLocalIP"][1]       = 168;
    settings["apLocalIP"][2]       = 4;
    settings["apLocalIP"][3]       = 1;

    settings["apGateway"][0]       = 192;
    settings["apGateway"][1]       = 168;
    settings["apGateway"][2]       = 4;
    settings["apGateway"][3]       = 1;

    settings["apSubnet"][0]        = 255;
    settings["apSubnet"][1]        = 255;
    settings["apSubnet"][2]        = 255;
    settings["apSubnet"][3]        = 0;

    settings["managerUser"]        = "manager"; // 20
    settings["managerPassword"]    = "12345678"; // 128
    settings["managerPort"]        = 8000;

    settings["captivePortalIP"][0] = 192;
    settings["captivePortalIP"][1] = 168;
    settings["captivePortalIP"][2] = 4;
    settings["captivePortalIP"][3] = 100;

    settings["captivePortalPort"]  = 8000;
    saveSettings();
}

void setLedBrightness(const float &brightness = 0) {
  digitalWrite(LED_BUILTIN, 1 - brightness);
}

ArRequestHandlerFunction createHandler(ArRequestHandlerFunction handler) {
  return [&handler](AsyncWebServerRequest *request) {
    if (request->authenticate(settings["managerUser"].as<const char*>(), settings["managerPassword"].as<const char*>()))
      handler(request);
    else
      request->requestAuthentication();
  };
}

void setup() {
  Serial.begin(9600);
  Serial.println("Starting...");
  Serial.println("Current data version: " + CURRENT_DATA_VERSION);
  Serial.println("Setting data address: " + SETTING_DATA_ADDRESS);
  Serial.println("Setting data size & EEPROM size: " + SPI_FLASH_SEC_SIZE);
  EEPROM.begin(SPI_FLASH_SEC_SIZE);
  pinMode(LED_BUILTIN, OUTPUT);
  setLedBrightness(LOW);

  DeserializationError deserializationError = deserializeJson(settings, eepromStream);
  Serial.println("Deserialized data from EEPROM: " + getSerializedSettingData());
  Serial.println("Validating setting data...");
  if (deserializationError != DeserializationError::Ok) {
    Serial.println("Resetting setting data due to error: " + String(deserializationError.c_str()));
    resetSettings();
  } else if (settings["dataVersion"].as<uint64_t>() != CURRENT_DATA_VERSION) {
    Serial.println("Resetting setting data due to data version not matched");
    resetSettings();
  } else
    Serial.println("Setting data is valid");

  Serial.println("Current setting data: " + getSerializedSettingData());

  const IPAddress apLocalIP(
    settings["apLocalIP"][0].as<uint8_t>(),
    settings["apLocalIP"][1].as<uint8_t>(),
    settings["apLocalIP"][2].as<uint8_t>(),
    settings["apLocalIP"][3].as<uint8_t>()
  );

  Serial.println("Setting up access point...");
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(
    apLocalIP,
    IPAddress(settings["apGateway"][0].as<uint8_t>(), settings["apGateway"][1].as<uint8_t>(), settings["apGateway"][2].as<uint8_t>(), settings["apGateway"][3].as<uint8_t>()),
    IPAddress(settings["apSubnet"][0].as<uint8_t>(), settings["apSubnet"][1].as<uint8_t>(), settings["apSubnet"][2].as<uint8_t>(), settings["apSubnet"][3].as<uint8_t>())
  );
  WiFi.softAP(
    settings["apSSID"].as<const char*>(),
    settings["auth"].as<bool>() ? settings["apPassword"].as<const char*>() : 0,
    settings["channel"].as<int>(),
    settings["hidden"].as<bool>(),
    settings["maxConnections"].as<int>(),
    settings["beaconInterval"].as<int>()
  );
  Serial.println("Access point is now ready for connections");

  Serial.println("Setting up manager web server...");
  AsyncWebServer managerWebServer(settings["managerPort"].as<uint16_t>());

  managerWebServer.on("/api", HTTP_ANY, createHandler([](AsyncWebServerRequest *request) {
    request->send(403);
  }));

  managerWebServer.on("/api/settings", HTTP_ANY, createHandler([](AsyncWebServerRequest *request) {
    switch (request->method()) {
      case HTTP_GET:
        request->send(200, "application/json", getSerializedSettingData());
        break;
      case HTTP_POST:
        request->send(500); // Change this
        break;
      case HTTP_DELETE:
        resetSettings();
        request->send(204);
        break;
      default:
        request->send(405);
    }
  }));

  managerWebServer.on("/api/reboot", HTTP_ANY, createHandler([](AsyncWebServerRequest *request) {
    if (request->method() == HTTP_POST) {
      request->send(204);
      ESP.restart();
    } else
      request->send(405);
  }));

  managerWebServer.onNotFound(createHandler([](AsyncWebServerRequest *request) {
    if (request->method() == HTTP_GET) {
      request->send(200, "text/html", "Hello, world"); // Change this
    } else
      request->send(405);
  }));

  managerWebServer.begin();
  Serial.println("Manager web server is now ready for requests");

  Serial.println("Setting up a redirection server...");
  AsyncWebServer redirectionWebServer(80);

  redirectionWebServer.onNotFound([](AsyncWebServerRequest *request) {
    request->redirect("http://" + IPAddress(
      settings["captivePortalIP"][0].as<uint8_t>(),
      settings["captivePortalIP"][1].as<uint8_t>(),
      settings["captivePortalIP"][2].as<uint8_t>(),
      settings["captivePortalIP"][3].as<uint8_t>()
    ).toString() + ":" + settings["captivePortalPort"].as<uint16_t>());
    setLedBrightness(HIGH);
    delay(10);
    setLedBrightness(LOW);
  });

  redirectionWebServer.begin();
  Serial.println("Redirection server is now ready for requests");

  Serial.println("Setting up DNS server...");
  DNSServer dnsServer;
  dnsServer.start(53, "*", apLocalIP);
  Serial.println("DNS server is now ready");
  Serial.println("Everything ready");

  while (true) {
    dnsServer.processNextRequest();
  }
}

void loop() {}