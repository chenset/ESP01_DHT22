#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include "images.h"
#include "SH1106Brzo.h"
#include "SH1106Wire.h"
#include <ESP8266HTTPClient.h>

// WIFI
const char *host = "esp8266-webupdate";
const char *ssid = "deny-2.4G";
const char *password = "960902463";

static const uint8_t D0   = 16;
static const uint8_t D1   = 5;
static const uint8_t D2   = 4;
static const uint8_t D3   = 0;
static const uint8_t D4   = 2;
static const uint8_t D5   = 14;
static const uint8_t D6   = 12;
static const uint8_t D7   = 13;
static const uint8_t D8   = 15;
static const uint8_t RX   = 3;
static const uint8_t TX   = 1;
// Initialize the OLED display using brzo_i2c
// D1 -> SDA
// D2 -> SCL
// SSD1306Brzo display(0x3c, D1, D2);
SH1106Brzo display(0x3c, D3, D4);

// Baud
const int baud = 115200;

void setup() {
  // Serial
  Serial.begin(baud);

  // init display
  display.init();
  display.flipScreenVertically();
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
}

void loop() {
  // OTA
  ArduinoOTA.handle();
  delay(10);
}
