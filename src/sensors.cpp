#include "sensors.h"

// Uncomment the type of sensor in use:
// #define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE DHT22 // DHT 22 (AM2302)
// #define DHTTYPE    DHT21     // DHT 21 (AM2301)

DHT dhtSensor(DHTPIN, DHTTYPE); // definition of DHT-Object

OneWire oneWire(OneWireBus);
DallasTemperature sensors(&oneWire);

unsigned long lastTempReadTime = 55000;
unsigned long lastHumReadTime = 550000;
unsigned long lastGroundTempReadTime = 55000;
unsigned long lastGroundHumReadTime = 55000;
const unsigned long interval = 60000;

float temperature = 0.0;
float humidity = 0.0;
float groundTemp = 0.0;
int groundHumidity = 0;
int groundHumidityDigital = 0;

void setupDHT() {
    dhtSensor.begin();
    sensors.begin();
}

void readDHTSensor(float &temperature, float &humidity) {
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    temperature = dhtSensor.readTemperature();
    humidity = dhtSensor.readHumidity();

    if (isnan(temperature) || isnan(humidity)) {
        println("error reading dht sensor");
    }
}

float readDHTTemperature() {
    if (millis() - lastTempReadTime >= interval) {
        lastTempReadTime = millis();
        // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
        // Read temperature as Celsius (the default)
        temperature = dhtSensor.readTemperature();
        // Read temperature as Fahrenheit (isFahrenheit = true)
        // float t = dhtSensor.readTemperature(true);
        if (isnan(temperature)) { // Check if any reads failed and exit early (to try again).
            println("Failed to read temperature from DHT sensor! Check connection and reset ESP!");
            //temperature = -99.0;
            return -99.0;
        } else {
            printFormatted("Temperature: %f\n", temperature);
        }
    }
    return temperature;
}

float readDHTHumidity() {
    if (millis() - lastHumReadTime >= interval) {
        lastHumReadTime = millis();
        // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
        humidity = dhtSensor.readHumidity();
        if (isnan(humidity)) { // Check if any reads failed and exit early (to try again).
            println("Failed to read humidity from DHT sensor! Check connection and reset ESP!");
            //humidity = -999.0;
            return -1.0;
        } else {
            printFormatted("Humidity: %f\n", humidity);
        }
    }
    return humidity;
}

float readGroundTemperature() {
    if (millis() - lastGroundTempReadTime >= interval) {
        lastGroundTempReadTime = millis();
        sensors.requestTemperatures();
        delay(150);
        groundTemp = sensors.getTempCByIndex(0);
        if (groundTemp == -127.00) {
            println("Failed to read temperature from DS18B20 sensor!");
            return -999.0;
        } else {
            printFormatted("Ground temperature: %f°C\n", groundTemp);
        }
    }
    return groundTemp;
}

int readGroundHumidity() {
    if (millis() - lastGroundHumReadTime >= interval) {
        lastGroundHumReadTime = millis();
        groundHumidity = analogRead(groundHumidityPin);
        // println(String(groundHumidity));
        //  Check if any reads failed
        if (isnan(groundHumidity)) {
            println("Failed to read/connect ground humidity data sensor! Check connection!");
            return -1;
        }
        groundHumidity = map(groundHumidity, 0, 4096, 0, 100);
        printFormatted("Ground humidity: %d%\n", groundHumidity);
    }
    return groundHumidity;
}

/*
int readGroundHumidity() {
    if (millis() - lastGroundHumReadTime >= interval) {
        lastGroundHumReadTime = millis();
        groundHumidity = analogRead(groundHumidityPin);
        //println(String(groundHumidity));
        //  Check if any reads failed
        if (groundHumidity < 500) {
            println("Failed to read ground humidity data sensor!");
            return -1;
        }
        groundHumidity = map(groundHumidity, 10, 4096, 0, 100);
        printFormatted("Ground humidity: %f%\n", groundHumidity);
    }
    return groundHumidity;
}




int readGroundHumidityDigital() {
    if (millis() - lastGroundHumDigitReadTime >= interval) {
        lastGroundHumDigitReadTime = millis();
        groundHumidityDigital = digitalRead(groundHumidityPinDigital);

        // Check if any reads failed
        if (isnan(groundHumidityDigital)) {
            println("Failed to read ground humidity data sensor!");
            return -1;
        }
    }
    return groundHumidityDigital;
}
*/