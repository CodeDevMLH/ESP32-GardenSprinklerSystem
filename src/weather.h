#ifndef WEATHER_H
#define WEATHER_H

#include "Arduino.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "configuration.h"
#include "ntp.h"

extern int sunsetTime;
extern float rain1h;
extern float rain24h;
extern float rainForecast;
extern float calculatedRain24h;

extern struct tm timeinfo;

extern const String openWeatherUrl;
extern const String openWeatherForecastUrl;
extern const String meteomaticsUrl;

void getWeatherOpenWeather();
bool updateCalculatedRainDataWithDuration();
bool checkUpdateRainData();
void getWeatherForecastOpenWeather();
void getWeatherMeteomatics();
int convertSunsetTimeTtoInt(time_t time);

#endif // WEATHER_H