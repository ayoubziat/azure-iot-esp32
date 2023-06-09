#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiMulti.h>
#include <mqtt_client.h>
#include <az_core.h>
#include <az_iot.h>
#include <azure_ca.h>

#include "dht_sensor.h"
#include "config.h"
#include "credentials.h"

#include <cstdlib>
#include <string.h>
#include <time.h>

static esp_mqtt_client_handle_t mqtt_client;
static az_iot_hub_client iot_hub_client;
WiFiMulti wifiMulti;
DHTSensor dhtSensor;

#define SUBSCRIBE_TOPIC "devices/" DEVICE_ID "/messages/devicebound/#"

void connectToWiFi();
void initTime();
void initIoTHubClient();
int initMqttClient();
void sendTelemetryData(float , float );
void getCurrentTime();

void setup() { 
  Serial.begin(57600);
  Serial.println("### Setup ###");
  connectToWiFi();
  initTime();
  initIoTHubClient();
  initMqttClient();
  // (void) initMqttClient();
}

void loop() {
  // Check WiFi connection and reconnect if needed
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Wifi connection lost");
    while (wifiMulti.run() != WL_CONNECTED) {
      Serial.print(".");
      delay(100);
    }
  }

  long now = millis();
  if (now - next_time > TELEMETRY_FREQUENCY_MILLISECS) {
    next_time = now;
    float temperature = dhtSensor.measureTemperature();
    float humidity = dhtSensor.measureHumidity();
    // Check if read failed.
    if (isnan(temperature) || isnan(humidity)) {
      Serial.println(F("Failed to read from DHT sensor"));
    }
    else {
      // sendTelemetryData(temperature, humidity);
    }
  }
}

void connectToWiFi() {
  Serial.println("-----------------------------------------");
  Serial.print("### Connect to WiFi ###\n");
  Serial.print("Connecting to WiFi: ");
  Serial.print(WIFI_SSID);
  // Setup wifi connection
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  // Connect to WiFi
  while (wifiMulti.run() != WL_CONNECTED){
    Serial.print(".");
    delay(100);
  }
  Serial.println();
  char buf[16];
  IPAddress ip = WiFi.localIP();
  sprintf(buf, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  Serial.printf("\nConnected to WiFi! IP address: %s\n", buf);
  Serial.println("-----------------------------------------");
}

void getCurrentTime() {
  current_time = time(nullptr);
  while (current_time < 8 * 3600 * 2) {
    delay(500);
    current_time = time(nullptr);
  }
}

void initTime() {
  Serial.print("Setting time using SNTP");
  configTime(GMT_OFFSET_SECS, GMT_OFFSET_SECS_DST, NTP_SERVERS);
  time_t now = time(NULL);
  while (now < UNIX_TIME_NOV_13_2017) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("\nTime initialized!");
}

void receivedCallback(char* topic, byte* payload, unsigned int length) {
  Serial.println("Received [");
  Serial.println(topic);
  Serial.println("]: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println("");
}

static esp_err_t mqttEventHandler(esp_mqtt_event_handle_t event) {
  switch (event->event_id) {
    case MQTT_EVENT_ERROR:
      Serial.println("MQTT event MQTT_EVENT_ERROR");
      break;
    case MQTT_EVENT_CONNECTED:
      Serial.println("MQTT event MQTT_EVENT_CONNECTED");
      int result;
      result = esp_mqtt_client_subscribe(mqtt_client, SUBSCRIBE_TOPIC, MQTT_QOS1);
      if (result == -1) {
        Serial.println("Could not subscribe for cloud-to-device messages.");
      }
      else {
        Serial.println("Subscribed for cloud-to-device messages; message id:" + String(result));
      }
      break;
    case MQTT_EVENT_DISCONNECTED:
      Serial.println("MQTT event MQTT_EVENT_DISCONNECTED");
      break;
    case MQTT_EVENT_SUBSCRIBED:
      Serial.println("MQTT event MQTT_EVENT_SUBSCRIBED");
      break;
    case MQTT_EVENT_UNSUBSCRIBED:
      Serial.println("MQTT event MQTT_EVENT_UNSUBSCRIBED");
      break;
    case MQTT_EVENT_PUBLISHED:
      Serial.println("MQTT event MQTT_EVENT_PUBLISHED");
      break;
    case MQTT_EVENT_DATA:
      Serial.println("MQTT event MQTT_EVENT_DATA");
      int i;
      char topic[DATA_BUFFER_SIZE];
      for (i = 0; i < (DATA_BUFFER_SIZE - 1) && i < event->topic_len; i++) {
        topic[i] = event->topic[i];
      }
      topic[i] = '\0';
      Serial.print("Message arrived on topic: ");
      Serial.println(String(topic));
      char data[DATA_BUFFER_SIZE];
      for (i = 0; i < (DATA_BUFFER_SIZE - 1) && i < event->data_len; i++) {
        data[i] = event->data[i];
      }
      data[i] = '\0';
      Serial.print("Message Data: ");
      Serial.println(String(data));
      break;
    case MQTT_EVENT_BEFORE_CONNECT:
      Serial.println("MQTT event MQTT_EVENT_BEFORE_CONNECT");
      break;
    default:
      Serial.println("MQTT event UNKNOWN");
      break;
  }

  return ESP_OK;
}

void initIoTHubClient() {
  Serial.println("Init IoT Hub Client");
  az_iot_hub_client_options options = az_iot_hub_client_options_default();
  options.user_agent = AZ_SPAN_FROM_STR(AZURE_SDK_CLIENT_USER_AGENT);
  if (az_result_failed(az_iot_hub_client_init(
          &iot_hub_client,
          az_span_create((uint8_t*) IOT_HUB_FQDN, strlen(IOT_HUB_FQDN)),
          az_span_create((uint8_t*) DEVICE_ID, strlen(DEVICE_ID)),
          &options)))
  {
    Serial.println("Failed initializing Azure IoT Hub client");
    return;
  }

  size_t client_id_length;
  if (az_result_failed(az_iot_hub_client_get_client_id(
          &iot_hub_client, mqtt_client_id, sizeof(mqtt_client_id) - 1, &client_id_length)))
  {
    Serial.println("Failed getting Client ID");
    return;
  }

  if (az_result_failed(az_iot_hub_client_get_user_name(&iot_hub_client, mqtt_username, sizeofarray(mqtt_username), NULL))) {
    Serial.println("Failed to get MQTT Username, return code");
    return;
  }

  Serial.printf("Client ID: ");
  Serial.println(String(mqtt_client_id));
  Serial.printf("Username: ");
  Serial.println(String(mqtt_username));
}

int initMqttClient() {
  Serial.println("Init IoT Hub MQTT Client");
  esp_mqtt_client_config_t mqtt_config;
  memset(&mqtt_config, 0, sizeof(mqtt_config));
  mqtt_config.uri = MQTT_BROKER;
  mqtt_config.port = AZ_IOT_DEFAULT_MQTT_CONNECT_PORT;
  mqtt_config.client_id = mqtt_client_id;
  mqtt_config.username = mqtt_username;
  mqtt_config.password = (const char*) az_span_ptr(AZ_SPAN_FROM_BUFFER(SAS_TOKEN));
  mqtt_config.keepalive = 30;
  mqtt_config.disable_clean_session = 0;
  mqtt_config.disable_auto_reconnect = false;
  mqtt_config.event_handle = mqttEventHandler;
  mqtt_config.user_context = NULL;
  mqtt_config.cert_pem = (const char*) ca_pem;

  mqtt_client = esp_mqtt_client_init(&mqtt_config);
  if (mqtt_client == NULL) {
    Serial.println("Failed creating mqtt client");
    return 1;
  }

  esp_err_t start_result = esp_mqtt_client_start(mqtt_client);
  if (start_result != ESP_OK){
    Serial.printf("Could not start mqtt client; error code: %s \n", start_result);
    return 1;
  } else {
    Serial.println("MQTT client started");
    return 0;
  }
}

void sendTelemetryData(float temperature, float humidity) {
  Serial.println("-----------------------------------------");
  Serial.println("## Sending telemetry data ##");
  if (az_result_failed(az_iot_hub_client_telemetry_get_publish_topic(
          &iot_hub_client, NULL, mqtt_publish_topic, sizeof(mqtt_publish_topic), NULL)))
  {
    Serial.println("Failed az_iot_hub_client_telemetry_get_publish_topic");
    return;
  }

  StaticJsonDocument<DATA_BUFFER_SIZE> jsonDoc;
  jsonDoc["MessageId"] = message_count;
  jsonDoc["DeviceId"] = DEVICE_ID;
  char time_buffer[26];
  getCurrentTime();
  strftime(time_buffer, 25, "%a %b %d %T %G", localtime(&current_time));
  jsonDoc["Time"] = time_buffer;

  // Temeletry data
  jsonDoc["Temperature"] = temperature;
  jsonDoc["Humidity"] = humidity;

  // convert JSON document to char array
  u_int16_t messageSize = measureJson(jsonDoc);
  char messagePayload[messageSize + 1] = "";
  messagePayload[messageSize + 1] = '\n';
  serializeJson(jsonDoc, messagePayload, messageSize);

  if (esp_mqtt_client_publish(
          mqtt_client,
          mqtt_publish_topic,
          (const char*) messagePayload,
          messageSize,
          MQTT_QOS1,
          DO_NOT_RETAIN_MSG)
      == 0)
  {
    Serial.println("Failed publishing the measurements");
  }
  else {
    Serial.println("DHT measurements: published successfully");
    Serial.println(messagePayload);
    message_count++;
  }
  Serial.println("-----------------------------------------");
}
