#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include "WebSerialUtils.h"

#include <DHT.h>
//#include <Adafruit_Sensor.h>
#include <OneWire.h>
#include <DallasTemperature.h>

extern const int DHTPIN;
extern const int OneWireBus;
extern const int groundHumidityPin;
extern const int groundHumidityPinDigital;

extern DHT dhtSensor; // Externe Deklaration des DHT-Objekts
extern OneWire oneWire;
extern DallasTemperature sensors;

void setupDHT();
void readDHTSensor(float &temperature, float &humidity);
float readDHTTemperature();
float readDHTHumidity();
float readGroundTemperature();
int readGroundHumidity();
//int readGroundHumidityDigital();


#endif // SENSORS_H
