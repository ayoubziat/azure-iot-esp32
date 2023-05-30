#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

#include <Adafruit_Sensor.h>
#include <DHT.h>

#define DHTPIN 19
#define DHTTYPE DHT11

class DHTSensor{
    public:
        void setup();
        float measureTemperature();
        float measureHumidity();
};

#endif // DHT_SENSOR_H