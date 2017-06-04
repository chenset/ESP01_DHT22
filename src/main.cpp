#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include "images.h"
#include "SH1106Brzo.h"
#include "SH1106Wire.h"
#include <ESP8266HTTPClient.h>
#include <DHT.h>

// WIFI
const char *host = "esp8266-oled";
const char *ssid = "deny-2.4G";
const char *password = "960902463";

// Chip name
String chipName = "KAWAII";

// static const uint8_t D0   = 16;
// static const uint8_t D1   = 5;
// static const uint8_t D2   = 4;
// static const uint8_t D3   = 0;
// static const uint8_t D4   = 2;
// static const uint8_t D5   = 14;
// static const uint8_t D6   = 12;
// static const uint8_t D7   = 13;
// static const uint8_t D8   = 15;
// static const uint8_t RX   = 3;
// static const uint8_t TX   = 1;
// Initialize the OLED display using brzo_i2c
// D1 -> SDA
// D2 -> SCL
// SSD1306Brzo display(0x3c, D1, D2);
SH1106Brzo display(0x3c, 0, 2);

// DHT WEB Server
WiFiServer DHTServer(80);
// DHT sensor settings
#define DHTPIN 5     // what digital pin we're connected to
#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);
// DHT variables
float humidity = 0.00;
float temperature = 0.00;
void DHTServerResponse();
String getSensorsJson();
void DHTSenserUpdate();
unsigned long timeSinceLastDHT = 0;

// Baud
const int baud = 115200;

void setup() {
  // Serial
  Serial.begin(baud);

  // init display
  display.init();
  // display.flipScreenVertically();
  display.setFont(Roboto_20);

  // init display2
  display.setFont(Roboto_20);

  display.clear();
  display.drawXbm(34, 14, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
  display.display();

  // connect wifi & OTA init
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    Serial.println("WiFi Ready");
  } else {
    Serial.println("WiFi Failed");
  }

  // OTA
  ArduinoOTA.onStart([]() { Serial.println("Start"); });
  ArduinoOTA.onEnd([]() { Serial.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)
      Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  Serial.println("");
  Serial.println("WiFi connected");

  // Print the IP address
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(Roboto_20);
  display.drawString(10, 20,  WiFi.localIP().toString());
  display.display();

  // DHT WEB Server begin
  DHTServer.begin();
  // DHT
  dht.begin();
}

void loop() {
  // OTA
  ArduinoOTA.handle();
  DHTServerResponse();
  delay(10);

  if (millis() - timeSinceLastDHT > 10000) {
    DHTSenserUpdate();
    timeSinceLastDHT = millis();
  }
}

void DHTServerResponse() {
  // Check if a client has connected
  WiFiClient client = DHTServer.available();
  if (!client) {
    return;
  }

  // Wait until the client sends some data
  Serial.println("new client");
  unsigned long start = millis();

  while (!client.available()) {
    delay(1);

    if (millis() - start > 1000) {
      Serial.println("time out");
      return;
    }
  }

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();

  // Prepare the response
  String s = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n";
  s += getSensorsJson();
  s += "\n";

  // Send the response to the client
  client.print(s);

  client.flush();
  client.stop();
  delay(1);
  Serial.println("Client disonnected");
}

void DHTSenserUpdate() {
  double localHumidity = dht.readHumidity();
  double localTemperature = dht.readTemperature();

  if (isnan(localHumidity) || isnan(localTemperature)) {
    return;
  }

  if (localHumidity != 0.00 || localTemperature != 0.00) {
    //偏差修正
    humidity = localHumidity - 5.00;
    temperature = localTemperature - 1.00;
  }
}

// sensors json
String getSensorsJson() {
  float h = humidity;
  float t = temperature;
  if (isnan(h)) {
    h = 0.00;
  }
  if (isnan(t)) {
    t = 0.00;
  }

  String res = "{\"temperature\": ";
  res += (String)t;
  res += ",\"humidity\": ";
  res += (String)h;
  res += ",\"chip\": \"";
  res += chipName;
  res += "\"}";

  return res;
}
