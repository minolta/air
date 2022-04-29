/*
This is the code for the AirGradient DIY Air Quality Sensor with an ESP8266 Microcontroller.

It is a high quality sensor showing PM2.5, CO2, Temperature and Humidity on a small display and can send data over Wifi.

For build instructions please visit https://www.airgradient.com/diy/

Compatible with the following sensors:
Plantower PMS5003 (Fine Particle Sensor)
SenseAir S8 (CO2 Sensor)
SHT30/31 (Temperature/Humidity Sensor)

Please install ESP8266 board manager (tested with version 3.0.0)

The codes needs the following libraries installed:
"WifiManager by tzapu, tablatronix" tested with Version 2.0.3-alpha
"ESP8266 and ESP32 OLED driver for SSD1306 displays by ThingPulse, Fabrice Weinberg" tested with Version 4.1.0

If you have any questions please visit our forum at https://forum.airgradient.com/

Configuration:
Please set in the code below which sensor you are using and if you want to connect it to WiFi.
You can also switch PM2.5 from ug/m3 to US AQI and Celcius to Fahrenheit

If you are a school or university contact us for a free trial on the AirGradient platform.
https://www.airgradient.com/schools/

Kits with all required components are available at https://www.airgradient.com/diyshop/

MIT License
*/
#include <Arduino.h>
#include <AirGradient.h>
#include <WiFiManager.h>

#include <ESP8266WiFi.h>

#include <ESP8266HTTPClient.h>

#include <Wire.h>
#include <NTPClient.h>
#include "SH1106Wire.h"
// #include "SSD1306Wire.h"
// #include "Configfile.h"
#include "LittleFS.h"
// #define s8 1

#ifdef s8
#include "s8_uart.h"
#endif

void connectToWifi();
void showTextRectangle(String ln1, String ln2, boolean small);
int PM_TO_AQI_US(int pm02);
void displaytime();
#include <Ticker.h>
long uptime = 0;
String timenow;
Ticker flipper;
long updatedatatime = 0;
long showdisplay = 0;
long showpm = 0;
long showco = 0;
long showsht = 0;
long showdate = 0;
boolean senddatanow = false;
AirGradient ag = AirGradient();
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
SH1106Wire display(0x3c, SDA, SCL);
WiFiClient client;
HTTPClient http;
// set sensors that you do not use to false
// boolean hasPM = true;
boolean hasPM = true;
boolean hasCO2 = true;
boolean hasSHT = true;

// set to true to switch PM2.5 from ug/m3 to US AQI
boolean inUSaqi = false;

// set to true to switch from Celcius to Fahrenheit
boolean inF = false;

// set to true if you want to connect to wifi. The display will show values only when the sensor has wifi connection
boolean connectWIFI = true;

// change if you want to send the data to another server
String APIROOT = "http://192.168.88.21:3333/";
// String APIROOT = "http://192.168.88.225:8080/";

#ifdef s8

#define S8_BAUDRATE 9600
#define DEBUG_BAUDRATE 115200

#if (defined USE_SOFTWARE_SERIAL || defined ARDUINO_ARCH_RP2040)
#define S8_RX_PIN D4 // Rx pin which the S8 Tx pin is attached to (change if it is needed)
#define S8_TX_PIN D3 // Tx pin which the S8 Rx pin is attached to (change if it is needed)
#else
#define S8_UART_PORT 1 // Change UART port if it is needed
#endif
/* END CONFIGURATION */

#ifdef USE_SOFTWARE_SERIAL
SoftwareSerial S8_serial(S8_RX_PIN, S8_TX_PIN);
#else
#if defined(ARDUINO_ARCH_RP2040)
REDIRECT_STDOUT_TO(Serial) // to use printf (Serial.printf not supported)
UART S8_serial(S8_TX_PIN, S8_RX_PIN, NC, NC);
#else
HardwareSerial S8_serial(S8_UART_PORT);
#endif
#endif

S8_UART *sensor_S8;
S8_sensor sensor;
void setS8()
{
  S8_serial.begin(S8_BAUDRATE);
  sensor_S8 = new S8_UART(S8_serial);

  // Check if S8 is available
  sensor_S8->get_firmware_version(sensor.firm_version);
  int len = strlen(sensor.firm_version);
  if (len == 0)
  {
    Serial.println("SenseAir S8 CO2 sensor not found!");
    while (1)
    {
      delay(1);
    };
  }

  // Show basic S8 sensor info
  Serial.println(">>> SenseAir S8 NDIR CO2 sensor <<<");
  printf("Firmware version: %s\n", sensor.firm_version);
  sensor.sensor_id = sensor_S8->get_sensor_ID();
  Serial.print("Sensor ID: 0x");
  printIntToHex(sensor.sensor_id, 4);
  Serial.println("");
}
#endif
void scani2c()
{
  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for (address = 1; address < 127; address++)
  {

    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");

      nDevices++;
    }
    else if (error == 4)
    {
      Serial.print("Unknow error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
}

void savefile(String data, long fn)
{
  Serial.println("Save file");
  File file = LittleFS.open("/data/" + fn, "w");
  file.print(data);
  delay(1);
  file.close();
}
void listdata()
{

  Dir dirs = LittleFS.openDir("/data");
  while (dirs.next())
  {
    String fn = dirs.fileName();
    Serial.printf("FileName:%s\n", fn);
  }

  Serial.println("End list");
}
void timecheck()
{
  timenow = timeClient.getFormattedTime();
  updatedatatime++;
  uptime++;
  if (!senddatanow)
  {
    showdisplay++;
  }
  showdate++;
  // displaytime();
  // Serial.println(timenow);
}
void setup()
{
  Serial.begin(9600);

  display.init();
  // display
  displaytime();
  showTextRectangle("Init", String(ESP.getChipId(), HEX), true);
  if (LittleFS.begin())
  {
    Serial.println("LittleFs Open file");
    listdata();
  }
  if (hasPM)
    ag.PMS_Init();
#ifndef s8
  if (hasCO2)
    ag.CO2_Init();
#endif
  if (hasSHT)
    ag.TMP_RH_Init(0x44);
    // ag.TMP_RH_Init(0x58);
#ifdef s8
  setS8();
#endif

  if (connectWIFI)
  {
    connectToWifi();
    timeClient.begin();
    timeClient.setTimeOffset(25200);
    timeClient.forceUpdate();
  }
  flipper.attach(1.0, timecheck);
  displaytime();
  delay(2000);
  display.clear();
}

void loop()
{

  // create payload
  if (Serial.available())
  {
    char key = Serial.read();
    if (key == 's')
      scani2c();
  }
  String payload = "{\"wifi\":" + String(WiFi.RSSI()) + ",";

  if (hasPM)
  {
    int PM2 = ag.getPM2_Raw();
    payload = payload + "\"pm02\":" + String(PM2);

    if (showdisplay % 10 == 0)
    {
      if (inUSaqi)
      {
        showTextRectangle("AQI", String(PM_TO_AQI_US(PM2)), false);
      }
      else
      {
        showTextRectangle("PM2", String(PM2), false);
      }
    }
  }

  if (hasCO2)
  {
    if (hasPM)
      payload = payload + ",";

#ifndef s8

    int CO2 = ag.getCO2_Raw();

    payload = payload + "\"rco2\":" + String(CO2);
    if (showdisplay % 20 == 0)
    {

      showTextRectangle("CO2", String(CO2), false);

#endif

#ifdef s8

      int co2 = 0;
      int i = 0;
      while (i < 10)
      {

        co2 = sensor_S8->get_co2();
        sensor.co2 = co2;
        if (sensor.co2 > 0)
        {
          break;
          // sensor.co2 = sensor_S8->get_co2();
        }
        else
        {
          Serial.printf("CO2 eRROR %d\n", co2);
          delay(15000);
        }

        i++;
      }

      // printf("CO2 value = %d ppm\n", sensor.co2);
      payload = payload + "\"rco2\":" + String(sensor.co2);
      showTextRectangle("CO2", String(sensor.co2), false);
#endif
    }
  }

  if (hasSHT)
  {
    if (hasCO2 || hasPM)
      payload = payload + ",";
    TMP_RH result = ag.periodicFetchData();
    payload = payload + "\"atmp\":" + String(result.t) + ",\"rhum\":" + String(result.rh);

    if (showdisplay % 30 == 0)
    {
      showdisplay = 0;
      if (inF)
      {
        showTextRectangle(String((result.t * 9 / 5) + 32), String(result.rh) + "%", false);
      }
      else
      {
        showTextRectangle(String(result.t), String(result.rh) + "%", false);
      }
    }
  }

  timeClient.forceUpdate();

  long fn = timeClient.getEpochTime();
  payload = payload + ",\"t\":" + fn;
  payload = payload + "}";

  displaytime();
  // Serial.println(payload);
  // send payload
  if (connectWIFI && updatedatatime >= 32)
  {
    senddatanow = true;
    updatedatatime = 0;
    // Serial.println(payload);
    String POSTURL = APIROOT + "sensors/airgradient:" + String(ESP.getChipId(), HEX) + "/measures";
    // Serial.println(POSTURL);

    http.begin(client, POSTURL);
    http.addHeader("content-type", "application/json");
    int httpCode = http.POST(payload);
    // Serial.println(httpCode);
    if (httpCode == -1)
    {
      // can not connect
      savefile(payload, fn);
      listdata();
    }
    String response = http.getString();
    http.end();
    senddatanow = false;
  }
}
String lasttimedisplay;
void displaytime()
{
  if (showdate > 0)
  {
    showdate = 0;
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.setColor(BLACK);
    display.drawStringf(5, 2, "%s", lasttimedisplay);
    display.display();
    display.setColor(WHITE);
    lasttimedisplay = timenow;
    display.drawString(5, 2, lasttimedisplay);
    display.display();
  }
}
// DISPLAY
String ln1lastdisplay;
String ln2lastdisplay;
void showTextRectangle(String ln1, String ln2, boolean small)
{
  // display.clear();

  display.setTextAlignment(TEXT_ALIGN_LEFT);
  if (small)
  {
    display.setFont(ArialMT_Plain_16);
  }
  else
  {
    display.setFont(ArialMT_Plain_24);
  }

  display.setColor(BLACK);
  display.drawString(5, 16, ln1lastdisplay);
  display.drawString(5, 36, ln2lastdisplay);
  display.display();

  display.setColor(WHITE);
  ln1lastdisplay = ln1;
  ln2lastdisplay = ln2;
  display.drawString(5, 16, ln1lastdisplay);
  display.drawString(5, 36, ln2lastdisplay);
  display.display();
}

// Wifi Manager
void connectToWifi()
{
  WiFiManager wifiManager;
  // WiFi.disconnect(); //to delete previous saved hotspot
  String HOTSPOT = "AIRGRADIENT-" + String(ESP.getChipId(), HEX);
  wifiManager.setTimeout(120);
  if (!wifiManager.autoConnect((const char *)HOTSPOT.c_str()))
  {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    ESP.restart();
    delay(5000);
  }
}

// Calculate PM2.5 US AQI
int PM_TO_AQI_US(int pm02)
{
  if (pm02 <= 12.0)
    return ((50 - 0) / (12.0 - .0) * (pm02 - .0) + 0);
  else if (pm02 <= 35.4)
    return ((100 - 50) / (35.4 - 12.0) * (pm02 - 12.0) + 50);
  else if (pm02 <= 55.4)
    return ((150 - 100) / (55.4 - 35.4) * (pm02 - 35.4) + 100);
  else if (pm02 <= 150.4)
    return ((200 - 150) / (150.4 - 55.4) * (pm02 - 55.4) + 150);
  else if (pm02 <= 250.4)
    return ((300 - 200) / (250.4 - 150.4) * (pm02 - 150.4) + 200);
  else if (pm02 <= 350.4)
    return ((400 - 300) / (350.4 - 250.4) * (pm02 - 250.4) + 300);
  else if (pm02 <= 500.4)
    return ((500 - 400) / (500.4 - 350.4) * (pm02 - 350.4) + 400);
  else
    return 500;
};