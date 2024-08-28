#include "sensors.h"

// Uncomment the type of sensor in use:
//#define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE DHT22  // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

DHT dhtSensor(DHTPIN, DHTTYPE); // definition of DHT-Object

//OneWire oneWire(OneWireBus);
//DallasTemperature sensors(&oneWire);

int groundHumidity = 0;
int groundHumidityDigital = 0;

void setupDHT() {
  dhtSensor.begin();
  //sensors.begin();
}

void readDHTSensor(float &temperature, float &humidity) {
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    temperature = dhtSensor.readTemperature();
    humidity = dhtSensor.readHumidity();
    
    if (isnan(temperature) || isnan(humidity)) {
        //Serial.println("Fehler beim Lesen des DHT-Sensors!");
    }
}

float readDHTTemperature() {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  // Read temperature as Celsius (the default)
  float temperature = dhtSensor.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  //float t = dhtSensor.readTemperature(true);
  if (isnan(temperature)) {  // Check if any reads failed and exit early (to try again).
    //Serial.println("Failed to read temperature from DHT sensor!");
    return -999.0;
  } else {
    //Serial.println(temperature);
    return temperature;
  }
}

float readDHTHumidity() {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float humidity = dhtSensor.readHumidity();
  if (isnan(humidity)) {  // Check if any reads failed and exit early (to try again).
    //Serial.println("Failed to read humidity from DHT sensor!");
    return -999.0;
  } else {
    //Serial.println(humidity);
    return humidity;
  }
}

//float readGroundTemperature() {
//  sensors.requestTemperatures();
//  float groundTemp = sensors.getTempCByIndex(0);
//  if (isnan(groundTemp)) {  // Check if any reads failed and exit early (to try again).
//    Serial.println("Failed to read humidity from DHT sensor!");
//    return -999.0;
//  } else {
//    //Serial.println(humidity);
//    return groundTemp;
//  }
//}

int readGroundHumidity() {
  groundHumidity = analogRead(groundHumidityPin);
  //groundHumidity = map(groundHumidity, 0, 4096, 0, 100);
  // Check if any reads failed
  if (isnan(groundHumidity)) {
    Serial.println("Failed to read ground humidity data sensor!");
    return -1;
  }
  groundHumidity = map(groundHumidity, 0, 4096, 0, 100);
  return groundHumidity;
}

/*
int readGroundHumidityDigital() {
  groundHumidityDigital = digitalRead(groundHumidityPinDigital);

  // Check if any reads failed
  if (isnan(groundHumidityDigital)) {
    Serial.println("Failed to read ground humidity data sensor!");
    return -1;
  }
  return groundHumidityDigital;
}
*/