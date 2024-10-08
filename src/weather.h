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
#include "WebSerialUtils.h"

extern int sunsetTime;
extern float rain1h;
extern float rain24h;
extern float rainForecast;
extern float calculatedRain;

extern struct tm timeinfo;

//extern const String openWeatherUrl;
//extern const String openWeatherForecastUrl;
//extern const String meteomaticsUrl;

void getWeatherOpenWeather();
bool getWeatherMeteo24H();
bool updateCalculatedRainDataWithDuration();
bool checkUpdateRainData();
bool getCalculatedWeatherUpdateNow();
void getWeatherUpdateNow();
void getWeatherForecastOpenWeather();
void getWeatherMeteomatics();
int convertSunsetTimeTtoInt(time_t time);

#endif // WEATHER_H