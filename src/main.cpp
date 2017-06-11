// #include <ArduinoOTA.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266Ping.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <ESP8266HTTPClient.h>
#include <TimeLib.h>

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
// const char *ssid = ""; move to env.h
// const char *password = ""; move to env.h

// NTP time server
// const char *ntpServerName = "time.nist.gov";
// const char *ntpServerName = "cn.ntp.org.cn";
const char *ntpServerName = "1.asia.pool.ntp.org";

// init DHT WEB Server
WiFiServer DHTServer(80);

// UDP for ntp server
WiFiUDP Udp;
unsigned int localPort = 8888; // local port to listen for UDP packets

// Time Zone
const int timeZone = +8;



// DHT sensor settings
#define DHTPIN 2     // what digital pin we're connected to
#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

// Initialize the OLED display using brzo_i2c
// D1 -> SDA
// D2 -> SCL
// SSD1306Brzo display(0x3c, D1, D2);
// or
// SH1106Wire display2(0x3c, D3, D5);
//
// int counter = 1;

// Baud
const int baud = 115200;

// Host
char *nasHost = "10.0.0.2";
const int httpPort = 88;
const char *url = "/sensor/upload";

//  Time since
unsigned long timeSinceLastHttpRequest = 0;
unsigned long timeSinceLastClock = 0;
unsigned long timeSinceLastDHT = 0;

// DHT variables
float humidity = 0.00;
float temperature = 0.00;

// void OLEDDisplay2Ctl();
void DHTSenserPost();
void DHTSenserUpdate();
void DHTServerResponse();
void sendNTPpacket(IPAddress &address);
String getSensorsJson();
time_t getNtpTime();

// NTP var
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming & outgoing
                                    // packets
// return lastNtpTime if unable to get the time
time_t lastNtpTime = 0;
unsigned long lastNtpTimeFix = 0;

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

  // OTA
  // ArduinoOTA.onStart([]() { Serial.println("Start"); });
  // ArduinoOTA.onEnd([]() { Serial.println("\nEnd"); });
  // ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
  //   Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  // });
  // ArduinoOTA.onError([](ota_error_t error) {
  //   Serial.printf("Error[%u]: ", error);
  //   if (error == OTA_AUTH_ERROR)
  //     Serial.println("Auth Failed");
  //   else if (error == OTA_BEGIN_ERROR)
  //     Serial.println("Begin Failed");
  //   else if (error == OTA_CONNECT_ERROR)
  //     Serial.println("Connect Failed");
  //   else if (error == OTA_RECEIVE_ERROR)
  //     Serial.println("Receive Failed");
  //   else if (error == OTA_END_ERROR)
  //     Serial.println("End Failed");
  // });
  // ArduinoOTA.begin();

  Serial.println("");
  Serial.println("WiFi connected");

  // Print the IP address
  delay(500);

  // DHT WEB Server begin
  DHTServer.begin();
  // DHT
  // dht.begin();

  DHTSenserUpdate();
  timeSinceLastDHT = millis();

  DHTSenserPost();
  timeSinceLastHttpRequest = millis();
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

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011; // LI, Version, Mode
  packetBuffer[1] = 0;          // Stratum, or type of clock
  packetBuffer[2] = 6;          // Polling Interval
  packetBuffer[3] = 0xEC;       // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); // NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
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

time_t getNtpTime() {
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0)
    ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 5000) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE); // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      lastNtpTime = secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
      lastNtpTimeFix = millis();
      return lastNtpTime;
    }
  }
  Serial.println("No NTP Response :-(");
  return lastNtpTime + ((int)(millis() - lastNtpTimeFix) /
                        1000); // return lastNtpTime if unable to get the time
}
