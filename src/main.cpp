#include <Arduino.h>
#include <AzureIotHub.h>
#include <Esp32MQTTClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "dht_sensor.h"
#include "credentials.h"

DHTSensor dhtSensor;

void setupWifi();
void callback(IOTHUB_CLIENT_CONFIRMATION_RESULT);
void sendDHTSensorData(float , float );
void getCurrentTime();

static bool isConnected = false;
int messageCount {1};
int64_t nextTime {0};
const u_int16_t bufferSize = 2048;
time_t currentTime;

void setup() {
  Serial.begin(57600);
  Serial.println("### Setup ###");
  dhtSensor.setup();
  delay(500);
  setupWifi();
  if (!isConnected){
    return;
  }

  Esp32MQTTClient_Init((const uint8_t*) AZURE_IOT_CONNECTION_STRING);
  Esp32MQTTClient_SetSendConfirmationCallback(callback);
}

void loop() {
  if (isConnected) {
    long current_time = millis();
    if (current_time - nextTime > TIME_INTERVAL) {
      nextTime = current_time;
      float temperature = dhtSensor.measureTemperature();
      float humidity = dhtSensor.measureHumidity();
      Serial.println(temperature);
      Serial.println(humidity);
      // Check if read failed.
      if (isnan(temperature) || isnan(humidity)) {
        Serial.println(F("Failed to read from DHT sensor"));
      }
      else {
        sendDHTSensorData(temperature, humidity);
      }
    }
  }
  delay(10);
}

void setupWifi() {
  Serial.println("-----------------------------------------");
  Serial.print("### Setup WiFi ###\n");
  Serial.print("Connecting to WiFi: ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  char buf[16];
  IPAddress ip = WiFi.localIP();
  sprintf(buf, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  Serial.printf("WiFi connected! IP address: %s\n", buf);
  Serial.println("-----------------------------------------");
  isConnected = true;
}

void callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result) {
  if (result == IOTHUB_CLIENT_CONFIRMATION_OK){
    Serial.println("Send Confirmation Callback finished.");
  }
}

void getCurrentTime() {
  currentTime = time(nullptr);
  while (currentTime < 8 * 3600 * 2) {
    delay(500);
    currentTime = time(nullptr);
  }
}

void sendDHTSensorData(float temperature, float humidity) {
  Serial.println("--------------------");
  Serial.println("## Send Data ##");
  Serial.println(temperature);
  Serial.println(humidity);

  StaticJsonDocument<bufferSize> jsonDoc;
  jsonDoc["DeviceId"] = DEVICE_NAME;
  // jsonDoc["MessageId"] = messageCount;
  char time_buffer[26];
  getCurrentTime();
  strftime(time_buffer, 25, "%a %b %d %T %G", localtime(&currentTime));
  jsonDoc["Time"] = time_buffer;

  // DHT11 sensor data
  jsonDoc["Temperature"] = temperature;
  jsonDoc["Humidity"] = humidity;

  // convert JSON document to char array
  u_int16_t jsonSize = measureJson(jsonDoc);
  char messagePayload[jsonSize + 1] = "";
  messagePayload[jsonSize + 1] = '\n';
  serializeJson(jsonDoc, messagePayload, jsonSize);

  Serial.print("DHT measurements: ");
  Serial.println(messagePayload);

  EVENT_INSTANCE* message = Esp32MQTTClient_Event_Generate(messagePayload, MESSAGE);
  Esp32MQTTClient_SendEventInstance(message);
  Serial.println("--------------------");
}
