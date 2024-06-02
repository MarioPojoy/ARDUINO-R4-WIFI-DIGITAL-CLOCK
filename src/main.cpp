#include <Arduino.h>
#include "RTC.h"
#include <WiFiS3.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "Arduino_LED_Matrix.h"
#include "credentials.h"

#define MODE_24 true
#define MODE_12 false

bool secondsON_OFF = true;
volatile bool irqFlag = false;
const int timeZoneOffsetHours = -6;
int wifiStatus = WL_IDLE_STATUS;
WiFiUDP Udp;
NTPClient timeClient(Udp);
ArduinoLEDMatrix matrix;
bool hourMode = MODE_12;

void periodicCallback() {
  irqFlag = true;
}

void printWifiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void connectToWiFi(){
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
    Serial.print("Current firmware: ");
    Serial.println(fv);
  }

  while (wifiStatus != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SSID);
    wifiStatus = WiFi.begin(SSID, PASSWORD);
    delay(5000);
  }

  Serial.println("Connected to WiFi");
  printWifiStatus();
}

void disconnectFromWiFi(){
  while (wifiStatus == WL_CONNECTED) {
    Serial.print("Disconnecting from SSID: ");
    Serial.println(SSID);
    wifiStatus = WiFi.disconnect();
    delay(5000);
  }
  WiFi.end();
}

byte Time[8][12] = {
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

byte Digits[5][30] = {                                                                 
  { 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
  { 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1 },
  { 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1 },
  { 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1 },
  { 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1 }
};

void displayDigit(int d, int s_x, int s_y) {
  for (int i=0;i<3;i++) {
    for (int j=0;j<5;j++) {
      Time[i+s_x][11-j-s_y] = Digits[j][i+d*3];
    }
  }
  matrix.renderBitmap(Time, 8, 12);
}

void displayDots(bool dots) {
  Time[0][2]=dots;
  Time[0][4]=dots;
  matrix.renderBitmap(Time, 8, 12);
}

void setup() {
  Serial.begin(115200);
  RTC.begin();
  RTCTime savedTime;
  RTC.getTime(savedTime);
  matrix.begin();
  matrix.loadSequence(LEDMATRIX_ANIMATION_LOAD);
  matrix.play(true);
  delay(3000);
  
  if (!RTC.isRunning()) {
    if (savedTime.getYear() == 2000) {
      Serial.println("RTC reset, configuring via WiFi...");
      matrix.loadSequence(LEDMATRIX_ANIMATION_WIFI_SEARCH);
      connectToWiFi();
      timeClient.begin();
      timeClient.update();
      unsigned long unixTime = timeClient.getEpochTime() + (timeZoneOffsetHours * 3600);
      timeClient.end();
      RTCTime startTime(unixTime);
      RTC.setTime(startTime);
      disconnectFromWiFi();
    } else {
      Serial.println("RTC does not need configuring, continuing...");
      RTC.setTime(savedTime);
    }
    matrix.loadSequence(LEDMATRIX_ANIMATION_CHECK);
    matrix.play(false);
    delay(2000);
  }
  
  if (RTC.setPeriodicCallback(periodicCallback, Period::N2_TIMES_EVERY_SEC)) {
    Serial.println("Periodic callback succesfully set");
  } else {
    Serial.println("ERROR: periodic callback not set");
  }
}

void loop() {
  RTCTime currentTime;
  RTC.getTime(currentTime);
  
  if (irqFlag){
    secondsON_OFF ?  secondsON_OFF = false : secondsON_OFF = true;
    
    if (hourMode == MODE_24) {
      displayDigit((int)(currentTime.getHour()/10),0,0 );
      displayDigit(currentTime.getHour()%10,4,0 ); 
    } else {
      if (currentTime.getHour() > 12) {
        displayDigit((int)((currentTime.getHour() - 12)/10),0,0 );
        displayDigit((currentTime.getHour() - 12)%10,4,0 ); 
      } else if (currentTime.getHour() == 0) {
          displayDigit((int)((currentTime.getHour() + 12)/10),0,0 );
          displayDigit((currentTime.getHour() + 12)%10,4,0 ); 
      } else {
          displayDigit((int)(currentTime.getHour()/10),0,0 );
          displayDigit(currentTime.getHour()%10,4,0 ); 
      }
    }
    
    displayDigit((int)(currentTime.getMinutes()/10),1,6 );
    displayDigit(currentTime.getMinutes()%10,5,6 );
    displayDots(secondsON_OFF);   
    irqFlag = false;
  }
}