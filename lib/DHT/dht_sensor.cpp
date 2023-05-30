#include "dht_sensor.h"

DHT dht(DHTPIN, DHTTYPE);

void DHTSensor::setup(){
    dht.begin();
}

float DHTSensor::measureTemperature(){
    delay(250);
    return dht.readTemperature();
}

float DHTSensor::measureHumidity(){
    delay(250);
    return dht.readHumidity();
}