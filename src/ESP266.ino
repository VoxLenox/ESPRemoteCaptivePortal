#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h> 
#include <ESP8266WebServer.h>

const uint64_t CURRENT_DATA_VERSION = 1;

typedef struct {
  uint64_t dataVersion;

  char* apSSID; // 32
  char* apPassword; // 63
  bool auth;
  bool hidden;
  int channel;
  int maxConnections;
  int beaconInterval;
  uint8_t apLocalIP[4];
  uint8_t apGateway[4];
  uint8_t apSubnet[4];

  char* managerPassword; // 128
  uint16_t managerPort;

  uint8_t captivePortalIP[4];
  uint16_t captivePortalPort;
} settings_t;

settings_t settings;

void saveSettings() {
  EEPROM.put(0, settings);
  EEPROM.commit();
}

void setLedBrightness(const float &brightness = 0) {
  digitalWrite(LED_BUILTIN, 1 - brightness);
}

void setup() {
  EEPROM.begin(SPI_FLASH_SEC_SIZE);

  pinMode(LED_BUILTIN, OUTPUT);
  setLedBrightness(0);

  EEPROM.get(0, settings);
  if (settings.dataVersion != CURRENT_DATA_VERSION) {
    settings.dataVersion = CURRENT_DATA_VERSION;

    settings.apSSID = "ESP8266";
    settings.apPassword = "12345678";
    settings.auth = false;
    settings.hidden = false;
    settings.channel = 1;
    settings.maxConnections = 4;
    settings.beaconInterval = 100;

    settings.apLocalIP[0] = 192;
    settings.apLocalIP[1] = 168;
    settings.apLocalIP[2] = 4;
    settings.apLocalIP[3] = 1;

    settings.apGateway[0] = 192;
    settings.apGateway[1] = 168;
    settings.apGateway[2] = 4;
    settings.apGateway[3] = 1;

    settings.apSubnet[0] = 255;
    settings.apSubnet[1] = 255;
    settings.apSubnet[2] = 255;
    settings.apSubnet[3] = 0;

    settings.managerPassword = "12345678";
    settings.managerPort = 8000;

    settings.captivePortalIP[0] = 192;
    settings.captivePortalIP[1] = 168;
    settings.captivePortalIP[2] = 4;
    settings.captivePortalIP[3] = 100;

    settings.captivePortalPort = 8000;

    saveSettings();
  }

  const IPAddress apLocalIP(
    settings.apLocalIP[0],
    settings.apLocalIP[1],
    settings.apLocalIP[2],
    settings.apLocalIP[3]
  );

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(
    apLocalIP,
    IPAddress(settings.apGateway[0], settings.apGateway[1], settings.apGateway[2], settings.apGateway[3]),
    IPAddress(settings.apSubnet[0], settings.apSubnet[1], settings.apSubnet[2], settings.apSubnet[3]
  ));
  WiFi.softAP(
    settings.apSSID,
    settings.auth ? settings.apPassword : 0,
    settings.channel,
    settings.hidden,
    settings.maxConnections,
    settings.beaconInterval
  );

  DNSServer dnsServer;
  ESP8266WebServer redirectWebServer(apLocalIP, 80);
  ESP8266WebServer managerWebServer(apLocalIP, settings.managerPort);

  redirectWebServer.onNotFound([&redirectWebServer]() {
    redirectWebServer.sendHeader("Location", "http://" + IPAddress(
      settings.captivePortalIP[0],
      settings.captivePortalIP[1],
      settings.captivePortalIP[2],
      settings.captivePortalIP[3]
    ).toString() + ":" + settings.captivePortalPort);
    redirectWebServer.send(302);

    setLedBrightness(1);
    delay(10);
    setLedBrightness(0);
  });

  managerWebServer.onNotFound([&managerWebServer]() {
      if (managerWebServer.uri() == String(settings.managerPassword)) {
        managerWebServer.send(200, "application/json", "");
      } else {
        managerWebServer.send(401, "text/html", "Hello, world!");
      }
  });

  dnsServer.start(53, "*", apLocalIP);
  redirectWebServer.begin();
  managerWebServer.begin();

  while (true) {
    dnsServer.processNextRequest();
    redirectWebServer.handleClient();
    managerWebServer.handleClient();
  }
}

void loop() {}