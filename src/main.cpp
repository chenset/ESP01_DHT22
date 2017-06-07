#include <ArduinoOTA.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include "SSD1306Brzo.h"
#include "images.h"
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
String chipName = "KAWAII";

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

//web benchkmark
const char* webBenchmarkUrl = envWebBenchmarkUrl;
const char* webBenchmarkFingerprint = envWebBenchmarkFingerprint;
String webBenchmarkHTTPCodeStr = "-";
String webBenchmarkStr = "WEB";
void webBenchmark();

// DHT sensor settings
// #define DHTPIN 5     // what digital pin we're connected to
// #define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321
// DHT dht(DHTPIN, DHTTYPE);

// Initialize the OLED display using brzo_i2c
// D1 -> SDA
// D2 -> SCL
// SSD1306Brzo display(0x3c, D1, D2);
// or
SSD1306Brzo display(0x3c, 0, 2);
// SH1106Wire display2(0x3c, D3, D5);
//
// int counter = 1;

// weather api
int temperatureOnline = 0;
int humidityOnline = 0;
int pm25Online = 0;

// weather img
int weatherImg = 0; // default 晴
// img array
// "晴",
// "多云",
// "阴",
// "阵雨",
// "雷阵雨",
// "雷阵雨伴有冰雹",
// "雨夹雪",
// "小雨",
// "中雨",
// "大雨",
// "暴雨",
// "大暴雨",
// "特大暴雨",
// "阵雪",
// "小雪",
// "中雪",
// "大雪",
// "暴雪",
// "雾",
// "冻雨",
// "沙尘暴",
// "小雨-中雨",
// "中雨-大雨",
// "大雨-暴雨",
// "暴雨-大暴雨",
// "大暴雨-特大暴雨",
// "小雪-中雪",
// "中雪-大雪",
// "大雪-暴雪",
// "浮尘",
// "扬沙",
// "强沙尘暴晴",
// "霾",
String weatherImgMapping[] = {
    "1", "3", "5",  "7",  "7",  "7", "7", "7", "7", "8", "8",
    "8", "8", "\"", "\"", "\"", "#", "#", "M", "7", "F", "7",
    "8", "8", "8",  "8",  "\"", "#", "#", "J", "E", "F", "E",
};

String weekDay[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat",
};

String Months[] = {
    "Jan",  "Feb", "Mar",  "Apr", "May", "June",
    "July", "Aug", "Sept", "Oct", "Nov", "Dec",
};

// Baud
const int baud = 115200;

// Time to sleep (in seconds):
const int sleepTimeS = 590;

// Host
char *nasHost = "10.0.0.2";
const int httpPort = 88;
const char *url = "/sensor/upload";

//  Time since
unsigned long timeSinceLastHttpRequest = 0;
unsigned long timeSinceLastClock = 0;
// unsigned long timeDisplay2LastClock = 0;
unsigned long timeSinceLastDHT = 0;
unsigned long timeSinceLastWEB = 0;
unsigned long timeSinceLastWeatherAPI = 0;

// DHT variables
float humidity = 0.00;
float temperature = 0.00;

void OLEDDisplayCtl();
// void OLEDDisplay2Ctl();
void DHTSenserPost();
// void DHTSenserUpdate();
void weatherUpdate();
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

  // init display
  display.init();
  // display.flipScreenVertically();
  display.setFont(Roboto_20);

  // init display2
  // display2.init();
  display.setFont(Roboto_20);

  display.clear();
  display.drawXbm(34, 14, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
  display.display();

  // display2.clear();
  // display2.setTextAlignment(TEXT_ALIGN_LEFT);
  // display2.setFont(Roboto_20);
  // display2.drawString(28, 20, "Weather");
  // display2.display();

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
  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display.setFont(ArialMT_Plain_16);
  display.drawString(64, 32,  WiFi.localIP().toString());
  display.display();
  delay(500);

  // init NTP UDP
  Udp.begin(localPort);
  // Serial.println(Udp.localPort());
  // Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(600);

  // DHT WEB Server begin
  DHTServer.begin();
  // DHT
  // dht.begin();

  // weather update
  weatherUpdate();
}

void loop() {
  // OTA
  ArduinoOTA.handle();
  DHTServerResponse();

  // OLED refresh
  if (millis() - timeSinceLastClock >= 1000) {
  // delay(1000);
    OLEDDisplayCtl();
    timeSinceLastClock = millis();
  }

  // juhe weather API
  if (millis() - timeSinceLastWeatherAPI >= 3600000) {
    weatherUpdate();
    timeSinceLastWeatherAPI = millis();
  }

  if (millis() - timeSinceLastWEB >= 10000) {
    webBenchmark();
    timeSinceLastWEB = millis();
  }

  // if (millis() - timeSinceLastDHT > 10000) {
    // DHTSenserUpdate();
    // timeSinceLastDHT = millis();
  // }

  // Use WiFiClient class to create TCP connections
  // if (millis() - timeSinceLastHttpRequest > 900000) {
    // DHTSenserPost();
    // timeSinceLastHttpRequest = millis();
  // }

  // if (millis() - timeDisplay2LastClock > 10000) {
    // OLEDDisplay2Ctl();
    // timeDisplay2LastClock = millis();
  // }
}

// void OLEDDisplay2Ctl() {
//   display2.clear();
//   // weather API weather
//   display2.setTextAlignment(TEXT_ALIGN_LEFT);
//   display2.setFont(Roboto_14);
//   display2.drawString(26, 3, (String)temperatureOnline + " / " +
//                                  (String)humidityOnline);
//
//   display2.setTextAlignment(TEXT_ALIGN_RIGHT);
//   display2.setFont(Roboto_14);
//   display2.drawString(128, 3, " P: " + (String)pm25Online);
//
//   display2.setTextAlignment(TEXT_ALIGN_LEFT);
//   display2.setFont(Meteocons_Plain_21);
//   display2.drawString(0, 0, weatherImgMapping[weatherImg]);
//
//   // temperature & humidity
//   int floorTemperature = floor(temperature);
//   int remainFloat = round(temperature * 10) - floorTemperature * 10;
//   String displayTemperature =
//       (String)floorTemperature + "." + (String)remainFloat;
//
//   display2.setTextAlignment(TEXT_ALIGN_LEFT);
//   display2.setFont(Roboto_Black_48);
//   display2.drawString(0, 19, displayTemperature);
//
//   display2.setTextAlignment(TEXT_ALIGN_RIGHT);
//   display2.setFont(Roboto_20);
//   display2.drawString(128, 35, ((String)round(humidity)));
//
//   display2.display();
// }

// void icmpPing(){
//       bool ret = Ping.ping(icmpPingDomain, 1);
//       if(ret){
//          pingTimeStr = (String)Ping.averageTime();
//       }else{
//          pingTimeStr = "FAILED";
//       }
// }

void OLEDDisplayCtl() {

  display.clear();

  // weather & temperature & humidity
  // display.setTextAlignment(TEXT_ALIGN_LEFT);
  // display.setFont(Meteocons_Plain_21);
  // display.drawString(0, 0, weatherImgMapping[weatherImg]);

  // display.setTextAlignment(TEXT_ALIGN_LEFT);
  // display.setFont(Roboto_14);
  // display.drawString(0, 3, Months[month() - 1] + ". " + (String)day() + ", " +
  //                              (String)year());

  // date
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.setFont(Roboto_14);
  display.drawString(128, 3, weekDay[weekday() - 1]);

  // weather API weather
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(Roboto_14);
  display.drawString(26, 3, (String)temperatureOnline + "-" + (String)humidityOnline + "-" + (String)pm25Online);

  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(Meteocons_Plain_21);
  display.drawString(0, 0, weatherImgMapping[weatherImg]);

  // center area
  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display.setFont(Roboto_Black_32);
  display.drawString(64, 34, webBenchmarkStr);

  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.setFont(Roboto_10);
  display.drawString(128, 34, "ms");

  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(Roboto_10);
  display.drawString(0, 34, webBenchmarkHTTPCodeStr);

  // clock
  // display.setTextAlignment(TEXT_ALIGN_LEFT);
  // display.setFont(Roboto_Black_48);
  // time_t hourInt = hour();
  // String hourStr;
  // if (10 > hourInt) {
  //   hourStr = "0" + (String)hourInt;
  // } else {
  //   hourStr = (String)hourInt;
  // }
  // display.drawString(0, 19, hourStr);
  //
  // display.setTextAlignment(TEXT_ALIGN_CENTER);
  // display.setFont(Roboto_Black_18);
  // display.drawString(64, 27, ":");
  //
  // display.setTextAlignment(TEXT_ALIGN_CENTER);
  // display.setFont(Roboto_14);
  // display.drawString(64, 47, (String)second());
  //
  // display.setTextAlignment(TEXT_ALIGN_RIGHT);
  // display.setFont(Roboto_Black_48);
  // time_t minuteInt = minute();
  // String minuteStr;
  // if (10 > minuteInt) {
  //   minuteStr = "0" + (String)minuteInt;
  // } else {
  //   minuteStr = (String)minuteInt;
  // }
  // display.drawString(128, 19, minuteStr);

  //hour
  time_t hourInt = hour();
  String hourStr;
  if (10 > hourInt) {
    hourStr = "0" + (String)hourInt;
  } else {
    hourStr = (String)hourInt;
  }
  //minute
  time_t minuteInt = minute();
  String minuteStr;
  if (10 > minuteInt) {
    minuteStr = "0" + (String)minuteInt;
  } else {
    minuteStr = (String)minuteInt;
  }
  //second
  time_t secondInt = second();
  String secondStr;
  if (10 > secondInt) {
    secondStr = "0" + (String)secondInt;
  } else {
    secondStr = (String)secondInt;
  }

  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 48,Months[month() - 1] + ". " + (String)day() );
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.setFont(ArialMT_Plain_16);
  display.drawString(128, 48, hourStr + ":" + minuteStr + ":" + secondStr);

  display.display();
}


void webBenchmark() {
  HTTPClient http;
  if(webBenchmarkFingerprint == ""){
      http.begin(webBenchmarkUrl); // HTTP
  }else{
      http.begin(webBenchmarkUrl, webBenchmarkFingerprint); // HTTPS
  }
  http.setUserAgent("WEB benchmarks from " + chipName);
  http.addHeader("Accept-Encoding", "gzip, deflate, sdch");

  // start connection and send HTTP header
  unsigned long start =  millis();
  int httpCode = http.GET();

  if(httpCode > 0) {
    webBenchmarkHTTPCodeStr = (String)httpCode;
  } else {
    webBenchmarkHTTPCodeStr = "FAILED";
  }
  webBenchmarkStr = (String) (millis() - start);
  http.end();
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

void weatherUpdate() {
  HTTPClient http;

  http.begin("http://op.juhe.cn/onebox/weather/"
             "query?cityname=深圳&key="
             "c43f962eb0631105e8598b26c6de9659"); // HTTP

  // start connection and send HTTP header
  int httpCode = http.GET();

  // httpCode will be negative on error
  if (httpCode == 200) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      int imgIndex = payload.indexOf("img");
      // imgIndex = -1 ; error handle
      if (imgIndex != -1) {
        weatherImg = payload.substring(imgIndex + 6, imgIndex + 8).toInt();
        Serial.println("weatherImg: ");
        Serial.println(weatherImg);
      }

      int temperatureIndex = payload.indexOf("temperature");
      if (temperatureIndex != -1) {
        temperatureOnline =
            payload.substring(temperatureIndex + 14, temperatureIndex + 14 + 2)
                .toInt();
        Serial.println("temperature: ");
        Serial.println(temperatureOnline);
      }

      int humidityIndex = payload.indexOf("humidity");
      if (humidityIndex != -1) {
        humidityOnline =
            payload.substring(humidityIndex + 11, humidityIndex + 11 + 2)
                .toInt();
        Serial.println("humidity: ");
        Serial.println(humidityOnline);
      }

      int pm25Index = payload.indexOf("pm25\":\"");
      if (pm25Index != -1) {
        pm25Online =
            payload.substring(pm25Index + 7, pm25Index + 7 + 3).toInt();
        Serial.println("pm25: ");
        Serial.println(pm25Online);
      }
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n",
                  http.errorToString(httpCode).c_str());
  }

  http.end();
}

// void DHTSenserUpdate() {
//   double localHumidity = dht.readHumidity();
//   double localTemperature = dht.readTemperature();
//
//   if (isnan(localHumidity) || isnan(localTemperature)) {
//     return;
//   }
//
//   if (localHumidity != 0.00 || localTemperature != 0.00) {
//     //偏差修正
//     humidity = localHumidity - 9.0;
//     temperature = localTemperature - 0.5;
//   }
// }

void DHTSenserPost() {
  WiFiClient client;

  if (client.connect(nasHost, httpPort)) {

    String jsonStr = getSensorsJson();

    String httpBody = String("POST ") + String(url) + " HTTP/1.1\r\n" +
                      "Host: " + nasHost + "\r\n" + "Content-Length: " +
                      jsonStr.length() + "\r\n\r\n" + jsonStr;

    Serial.println("--" + httpBody + "--");
    client.print(httpBody);
    delay(10);

    // Read all the lines of the reply from server and print them to Serial
    while (client.available()) {
      String line = client.readStringUntil('\r');
      Serial.print(line);
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
