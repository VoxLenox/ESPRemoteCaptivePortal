#include <ESP8266WiFi.h>
#include <DNSServer.h> 
#include <ESP8266WebServer.h>

const char* SSID = "Free WiFi";
const char* PORTAL_URL = "http://69.69.69.100:8000";
IPAddress NETWORK_IP(69, 69, 69, 1);

DNSServer dnsServer;
ESP8266WebServer webServer(80);

void setup() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(NETWORK_IP, NETWORK_IP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(SSID);

  dnsServer.start(53, "*", NETWORK_IP);
  d
  webServer.onNotFound([]() {
    webServer.sendHeader("Location", PORTAL_URL);
    webServer.send(302, "text/plain", "");
  });
  webServer.begin();
  
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, LOW);
  delay(100);
  digitalWrite(BUILTIN_LED, HIGH);
  delay(50);
  digitalWrite(BUILTIN_LED, LOW);
  delay(100);
  digitalWrite(BUILTIN_LED, HIGH);
}


void loop() { 
  dnsServer.processNextRequest();
  webServer.handleClient();
}