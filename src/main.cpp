#include <ArduinoOTA.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

//only define const char *ssid & const char *password in env.h
#include "env.h"

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

// Chip name
String chipName = "two";

// WIFI
const char *host = chipName.c_str();


// DHT sensor settings
#define DHTPIN 2     // what digital pin we're connected to
#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);


// Baud
const int baud = 115200;

// Host
char *nasHost = "10.0.0.2";
const int httpPort = 88;
const char *url = "/sensor/upload";

// DHT variables
float humidity = 0.00;
float temperature = 0.00;

// void OLEDDisplay2Ctl();
void DHTSenserPost();
void DHTSenserUpdate();
String getSensorsJson();


void setup() {
  // Serial
  Serial.begin(baud);

  // connect wifi & OTA init
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    Serial.println("WiFi Ready");
  } else {
    Serial.println("WiFi Failed");
  }

  Serial.println("");
  Serial.println("WiFi connected");

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

  // Print the IP address
  delay(500);
  for(int whileCount = 0;whileCount < 10;++whileCount){
      ArduinoOTA.handle();
      delay(1000);
  }

  DHTSenserUpdate();
  DHTSenserPost();
}

void loop() {
    // Serial.println("loop");
    // delay(1000);
  // OTA
  // ArduinoOTA.handle();
  // DHTServerResponse();
  //
  // // Use WiFiClient class to create TCP connections
  // if (millis() - timeSinceLastHttpRequest >= 599990) {
  delay(599990);
  DHTSenserUpdate();
  DHTSenserPost();

  //   timeSinceLastHttpRequest = millis();
  // }
  //
  // // if(millis() - timeSinceLastDHT >= 10000){
    // DHTSenserUpdate();
    // timeSinceLastDHT = millis();
  // // }
  // Serial.println("test");
  // delay(10000);
  // Serial.println("ESP8266 in sleep mode");
  // ESP.deepSleep(10000000);
}

void DHTSenserUpdate() {
  double localHumidity = dht.readHumidity();
  double localTemperature = dht.readTemperature();

  if (isnan(localHumidity) || isnan(localTemperature)) {
    return;
  }

  if (localHumidity != 0.00 || localTemperature != 0.00) {
    //偏差修正
    humidity = localHumidity - 5.0;
    temperature = localTemperature;
  }
}

void DHTSenserPost() {
  WiFiClient client;

  if (client.connect(nasHost, httpPort)) {

    String jsonStr = getSensorsJson();

    String httpBody = String("POST ") + String(url) + " HTTP/1.1\r\n" +
                      "Host: " + nasHost + "\r\n" + "Content-Length: " +
                      jsonStr.length() + "\r\n\r\n" + jsonStr;

    client.print(httpBody);
    delay(10);

    // Read all the lines of the reply from server and print them to Serial
    while (client.available()) {
      String line = client.readStringUntil('\r');
    }
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
