/*
    This sketch reads the output of a digital thermometer (DS18B20), 
    then connects to a TCP server through a WiFi receiver (ESP8266) 
    and sends a json-like response with the temperature value.
*/

// Libraries to use:
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.hpp>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Wifi network name and password:
#ifndef STASSID
#define STASSID "XXX"
#define STAPSK  "YYY"
#endif
const char* ssid     = STASSID;
const char* password = STAPSK;

// Server where to send the data:
const char* host = "A.B.C.D";
const uint16_t port = 0;

// Temperature sensor input pin:
const int oneWireBus = 2;

// Offset in seconds corresponding to UTC+1:
const long utcOffsetInSeconds = 3600;

// Number of samples to output its average:
const int numSamples = 30;

// Define objects:
OneWire oneWire(oneWireBus);
DallasTemperature sensors (&oneWire);
ESP8266WiFiMulti WiFiMulti;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// ESP8266's WiFi initialization function:
void initWifi(){
  // We are setting the ESP8266 to behave as a WiFi-client:
  WiFi.mode(WIFI_STA);
  // We try to connect to the given WiFi SSID and password:
  WiFiMulti.addAP(ssid, password);
  Serial.println("Waiting for WiFi connection... ");
  // We wait for the ESP8266 to connect to the desired WiFi network:
  while (WiFiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  // Once we are connected to the WiFi network we will show the local IP:
  Serial.println("WiFi connected! IP Address: ");
  Serial.println(WiFi.localIP());
  delay(500);
}

// Function that returns the mean of a given array:
float getMean(float * val, int arrayCount) {
  float total = 0.0;
  for (int i = 0; i < arrayCount; i++) {
    total = total + val[i];
  }
  float avg = total/(float)arrayCount;
  return avg;
}

// Function that returns the maximum value of a given array:
float getMax(float * val, int arrayCount) {
  float auxMax = -50.0;
  for (int i = 0; i < arrayCount; i++) {
    if (val[i] > auxMax) auxMax = val[i];
  }
  return auxMax;
}

// Function that returns the minimum value of a given array:
float getMin(float * val, int arrayCount) {
  float auxMin = 200.0;
  for (int i = 0; i < arrayCount; i++) {
    if (val[i] < auxMin) auxMin = val[i];
  }
  return auxMin;
}

// Function that returns the standard deviation of a given array:
float getStd(float * val, int arrayCount) {
  float avg = getMean(val, arrayCount);
  long total = 0;
  for (int i = 0; i < arrayCount; i++) {
    total = total + (val[i] - avg) * (val[i] - avg);
  }

  float variance = total/(float)arrayCount;
  float stdDev = sqrt(variance);
  return stdDev;
}

// Function that rounds a given value to 2 decimals:
double roundNumber2(float value) {
  return (int)(value * 100 + 0.5) / 100.0;
}


// Initialization block:
void setup() {
  Serial.println();
  // Setting up the baud rate Serial communication:
  Serial.begin(115200);
  Serial.println("[1/4] Serial communication set up");
  // Initializing the DS18B20 temperature sensor:
  sensors.begin();
  Serial.println("[2/4] DS18B20 temperature sensor initialized");
  // Initializing the WiFi connection:
  initWifi();
  Serial.println("[3/4] WiFi connection initialized");
  // Initializing the NTP client that will help us to obtain the UNIX timestamp:
  timeClient.begin();
  Serial.println("[4/4] NTP client initialized");
  }


void loop() {
  
  // Getting DS18B20 readings:
  float readings[numSamples] = {};
  float sensorValue;
  for (int i = 0; i < numSamples; i++){
    sensors.requestTemperatures();
    sensorValue = sensors.getTempCByIndex(0);
    readings[i] = sensorValue;
    delay(60 / numSamples);
  }
  
  // Computing reading statistics:
  float tempMean = getMean(readings, numSamples);
  float tempMax = getMax(readings, numSamples);
  float tempMin = getMin(readings, numSamples);
  float tempStd = getStd(readings, numSamples);
  Serial.println((String) "Mean temp.: " + tempMean + "°C");
  
   // Getting current time:
  timeClient.update();
  
  // Build JSON:
  String json;
  StaticJsonDocument<300> doc;
  doc["timestamp"] = timeClient.getEpochTime();
  doc["sensorId"] = 1;
  doc["sensorType"] = "temperature";
  doc["meanValue"] = roundNumber2(tempMean);
  doc["maxValue"] = roundNumber2(tempMax);
  doc["minValue"] = roundNumber2(tempMin);
  doc["stdValue"] = roundNumber2(tempStd);
  doc["unit"] = "celsius";
  serializeJson(doc, json);

  // Use WiFiClient class to create a TCP connection:
  WiFiClient client;
    Serial.println((String) "Connecting to " + host + ":" + port + "...");
  // If connection is not possible wait to try again:
  if (!client.connect(host, port)) {
    Serial.println("*** Connection failed! Waiting 10 seconds to try reconnecting again...");
    delay(10000);
    return;
  }

  // Sending json to server:
    Serial.println("Sending data to server...");
  if (client.connected()) {
    client.println(json);
  }

  // Wait for data to be available
  unsigned long timeout = millis();
  while (client.available() == 0) {
    // If we don't get a response 5 seconds after we sent data, client has timeout and we'll try to reconnect again:
    if (millis() - timeout > 5000) {
      Serial.println("*** Client timeout! Waiting 10 seconds to try reconnecting again...");
      client.stop();
      delay(10000);
      return;
    }
  }
  
  // Expecting acknowledgement from server:
  Serial.println((String) "Response from " + host + ":" + port + "-> ");
  while (client.available()) {
    char ch = static_cast<char>(client.read());
    Serial.print(ch);
  }

  // Close connection and wait for the next reading:
  client.stop();
  Serial.println();
  Serial.println("Connection closed. Requesting temperature again in 10 seconds...");
  delay(10000);
}
