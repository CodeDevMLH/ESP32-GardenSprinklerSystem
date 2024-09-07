#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

struct NetworkConfig {
  String SSID;
  String PASSWORD;
  String HOSTNAME;
  String NTP_SERVER;
};

struct WeatherConfig {
  String weatherChannel;
  String API_KEY;
  String METHEO_USERNAME;
  String METHEO_PASSWORD;
  float LATITUDE;
  float LONGITUDE;
  int DURATION_PAST;
  int  DURATION_FORECAST;
};

struct TimerConfig {
  int DURATION;
  int TIME;
  bool Time_EN;
  bool ALL;
};

struct TimeConfig {
  int ON_TIME;
  int TIMER_1;
  int TIMER_2;
};

struct DayConfig {
  bool ON;
  TimeConfig TIME;
};

struct MQTTConfig {
  String SERVER;
  String PORT;
  String USER;
  String PASSWORD;
  String TOPIC;
};

struct Config {
  const char *VERSION = "1.0.1";

  NetworkConfig network;
  WeatherConfig weather;
  TimerConfig TIMER;

  bool AUTOMATIC_MODE;
  bool PRECIPITATION_MODE;
  bool PRECIPITATION_MODE_FORECAST;
  float PRECIPITATION_AMOUNT;
  float PRECIPITATION_AMOUNT_FORECAST;
  bool MENTION_SUNSET;

  int MAX_PUMP_TIME;
  int MAX_VALVE1_TIME;
  int MAX_VALVE2_TIME;

  DayConfig MON;
  DayConfig TUE;
  DayConfig WED;
  DayConfig THU;
  DayConfig FRI;
  DayConfig SAT;
  DayConfig SUN;

  MQTTConfig mqtt;
};

#endif  // CONFIG_H