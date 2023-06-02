// Create a file named "credentials.h" define your credentials/private data

// WiFi AP SSID
#define WIFI_SSID ""
// WiFi password
#define WIFI_PASSWORD ""

// Azure IoT

// When developing for your own Arduino-based platform,
// please follow the format '(ard;<platform>)'.
#define AZURE_SDK_CLIENT_USER_AGENT "c%2F" AZ_SDK_VERSION_STRING

// Primary connection string of the device
#define IOT_DEVICE_PRIMARY_CONNECTION_STRING ""

#define IOT_HUB_NAME ""
#define IOT_HUB_FQDN IOT_HUB_NAME ".azure-devices.net"

#define MQTT_BROKER "mqtts://" IOT_HUB_FQDN

#define DEVICE_ID ""

// Shared Access Signature
#define SAS_TOKEN ""

