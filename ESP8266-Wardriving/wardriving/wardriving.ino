// raw sd card logging
#include <Adafruit_GFX.h>
#include <ESP8266WiFi.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <Time.h>

#define UTC_offset -7  // PDT
#define SD_CS      D8

String logFileName = "";
int networks = 0;

#define LOG_RATE 500
char currentTime[5];

SoftwareSerial ss(D3, D4); // RX, TX
TinyGPSPlus tinyGPS;

void setup() {
  Serial.begin(115200);

  //  pinMode(D3,/
  ss.begin(9600);

  WiFi.mode(WIFI_STA); WiFi.disconnect();

  /* initialize SD card */
  initializeSD();

  /* initialize GPS */
  delay(500);
  if (ss.available() > 0) {
    Serial.println("GPS: found");
    Serial.println("Waiting on fix...");
  }
  else {
    Serial.println("GPS: not found");
    Serial.println("Check wiring & reset.");
  }
  while (!tinyGPS.location.isValid()) {
    Serial.println(tinyGPS.location.isValid());
    delay(0);
    smartDelay(500);
  }

}
void lookForNetworks() {
  sprintf_P(currentTime, PSTR("%02d:%02d"), hour(), minute());
  int n = WiFi.scanNetworks();
  if (n == 0) {
    Serial.println("no networks found");
  }
  else {
    for (int i = 0; i < n; ++i) {
      if ((WiFi.channel(i) > 0) && (WiFi.channel(i) < 15)) {
        sprintf_P(currentTime, PSTR("%02d:%02d"), hour(), minute());
        networks++;
        File logFile = SD.open(logFileName, FILE_WRITE);
        logFile.print(WiFi.BSSIDstr(i));  logFile.print(',');
        logFile.print(WiFi.SSID(i)); logFile.print(',');

        String bssid = WiFi.BSSIDstr(i);
        bssid.replace(":", "");
        logFile.print(getEncryption(i, "")); logFile.print(',');

        logFile.print(year());   logFile.print('-');
        logFile.print(month());  logFile.print('-');
        logFile.print(day());    logFile.print(' ');
        logFile.print(hour());   logFile.print(':');
        logFile.print(minute()); logFile.print(':');
        logFile.print(second()); logFile.print(',');
        logFile.print(WiFi.channel(i)); logFile.print(',');
        logFile.print(WiFi.RSSI(i)); logFile.print(',');
        logFile.print(tinyGPS.location.lat(), 6); logFile.print(',');
        logFile.print(tinyGPS.location.lng(), 6); logFile.print(',');

        logFile.print(tinyGPS.altitude.meters(), 1); logFile.print(',');
        logFile.print(tinyGPS.hdop.value(), 1); logFile.print(',');
        logFile.println("WIFI");
        logFile.close();
      }
    }
  }
}
void loop() {
  if (tinyGPS.location.isValid()) {
    setTime(tinyGPS.time.hour(), tinyGPS.time.minute(), tinyGPS.time.second(), tinyGPS.date.day(), tinyGPS.date.month(), tinyGPS.date.year());
    adjustTime(UTC_offset * SECS_PER_HOUR);
    lookForNetworks();
  }
  smartDelay(LOG_RATE);
  if (millis() > 5000 && tinyGPS.charsProcessed() < 10)
    Serial.println("No GPS data received: check wiring");
}

static void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (ss.available())
      tinyGPS.encode(ss.read());
  } while (millis() - start < ms);
}

int isOnFile(String mac) {
  File netFile = SD.open(logFileName);
  String currentNetwork;
  if (netFile) {
    while (netFile.available()) {
      currentNetwork = netFile.readStringUntil('\n');
      if (currentNetwork.indexOf(mac) != -1) {
        netFile.close();
        return currentNetwork.indexOf(mac);
      }
    }
    netFile.close();
    return currentNetwork.indexOf(mac);
  }
}

void initializeSD() { // create new CSV file and add WiGLE headers
  int i = 0; logFileName = "log0.csv";
  while (SD.exists(logFileName)) {
    i++; logFileName = "log" + String(i) + ".csv";
  }
  File logFile = SD.open(logFileName, FILE_WRITE);
  if (logFile) {
    logFile.println("MAC,SSID,AuthMode,FirstSeen,Channel,RSSI,CurrentLatitude,CurrentLongitude,AltitudeMeters,AccuracyMeters,Type");
  }
  logFile.close();
}

String getEncryption(uint8_t network, String src) { // return encryption for WiGLE or print
  byte encryption = WiFi.encryptionType(network);
  switch (encryption) {
    case 2:
      if (src == "screen") {
        return "WPA";
      }
      return "[WPA-PSK-CCMP+TKIP][ESS]";
    case 5:
      if (src == "screen") {
        return "WEP";
      }
      return "[WEP][ESS]";
    case 4:
      if (src == "screen") {
        return "WPA2";
      }
      return "[WPA2-PSK-CCMP+TKIP][ESS]";
    case 7:
      if (src == "screen") {
        return "NONE" ;
      }
      return "[ESS]";
    case 8:
      if (src == "screen") {
        return "AUTO";
      }
      return "[WPA-PSK-CCMP+TKIP][WPA2-PSK-CCMP+TKIP][ESS]";
  }
}
