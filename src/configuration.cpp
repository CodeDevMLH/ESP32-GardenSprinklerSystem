#include "configuration.h"

Config config;
const char *configFilePath = "/config.json";

bool saveConfig(const char *path) {
  JsonDocument doc;

  doc["network"]["SSID"] = config.network.SSID;
  doc["network"]["PASSWORD"] = config.network.PASSWORD;
  doc["network"]["HOSTNAME"] = config.network.HOSTNAME;
  doc["network"]["NTP_SERVER"] = config.network.NTP_SERVER;

  doc["weather"]["weather_channel"] = config.weather.weatherChannel;
  doc["weather"]["API_KEY"] = config.weather.API_KEY;
  doc["weather"]["METHEO_USERNAME"] = config.weather.METHEO_USERNAME;
  doc["weather"]["METHEO_PASSWORD"] = config.weather.METHEO_PASSWORD;
  doc["weather"]["LATITUDE"] = config.weather.LATITUDE;
  doc["weather"]["LONGITUDE"] = config.weather.LONGITUDE;
  doc["weather"]["DURATION_PAST"] = config.weather.DURATION_PAST;
  doc["weather"]["DURATION_FORECAST"] = config.weather.DURATION_FORECAST;

  doc["timer"]["DURATION"] = config.TIMER.DURATION;
  doc["timer"]["EN"] = config.TIMER.Time_EN;
  doc["timer"]["TIME"] = config.TIMER.TIME;
  doc["timer"]["ALL"] = config.TIMER.ALL;

  doc["AUTOMATIC_MODE"] = config.AUTOMATIC_MODE;
  doc["PRECIPITATION_MODE"] = config.PRECIPITATION_MODE;
  doc["PRECIPITATION_MODE_FORECAST"] = config.PRECIPITATION_MODE_FORECAST;
  doc["PRECIPITATION_AMOUNT"] = config.PRECIPITATION_AMOUNT;
  doc["PRECIPITATION_AMOUNT_FORECAST"] = config.PRECIPITATION_AMOUNT_FORECAST;
  doc["MENTION_SUNSET"] = config.MENTION_SUNSET;

  doc["MAX_PUMP_TIME"] = config.MAX_PUMP_TIME;
  doc["MAX_VENT1_TIME"] = config.MAX_VENT1_TIME;
  doc["MAX_VENT2_TIME"] = config.MAX_VENT2_TIME;

  doc["MON"]["ON"] = config.MON.ON;
  doc["MON"]["TIME"]["ON_TIME"] = config.MON.TIME.ON_TIME;
  doc["MON"]["TIME"]["TIMER_1"] = config.MON.TIME.TIMER_1;
  doc["MON"]["TIME"]["TIMER_2"] = config.MON.TIME.TIMER_2;

  doc["TUE"]["ON"] = config.TUE.ON;
  doc["TUE"]["TIME"]["ON_TIME"] = config.TUE.TIME.ON_TIME;
  doc["TUE"]["TIME"]["TIMER_1"] = config.TUE.TIME.TIMER_1;
  doc["TUE"]["TIME"]["TIMER_2"] = config.TUE.TIME.TIMER_2;

  doc["WED"]["ON"] = config.WED.ON;
  doc["WED"]["TIME"]["ON_TIME"] = config.WED.TIME.ON_TIME;
  doc["WED"]["TIME"]["TIMER_1"] = config.WED.TIME.TIMER_1;
  doc["WED"]["TIME"]["TIMER_2"] = config.WED.TIME.TIMER_2;

  doc["THU"]["ON"] = config.THU.ON;
  doc["THU"]["TIME"]["ON_TIME"] = config.THU.TIME.ON_TIME;
  doc["THU"]["TIME"]["TIMER_1"] = config.THU.TIME.TIMER_1;
  doc["THU"]["TIME"]["TIMER_2"] = config.THU.TIME.TIMER_2;

  doc["FRI"]["ON"] = config.FRI.ON;
  doc["FRI"]["TIME"]["ON_TIME"] = config.FRI.TIME.ON_TIME;
  doc["FRI"]["TIME"]["TIMER_1"] = config.FRI.TIME.TIMER_1;
  doc["FRI"]["TIME"]["TIMER_2"] = config.FRI.TIME.TIMER_2;

  doc["SAT"]["ON"] = config.SAT.ON;
  doc["SAT"]["TIME"]["ON_TIME"] = config.SAT.TIME.ON_TIME;
  doc["SAT"]["TIME"]["TIMER_1"] = config.SAT.TIME.TIMER_1;
  doc["SAT"]["TIME"]["TIMER_2"] = config.SAT.TIME.TIMER_2;

  doc["SUN"]["ON"] = config.SUN.ON;
  doc["SUN"]["TIME"]["ON_TIME"] = config.SUN.TIME.ON_TIME;
  doc["SUN"]["TIME"]["TIMER_1"] = config.SUN.TIME.TIMER_1;
  doc["SUN"]["TIME"]["TIMER_2"] = config.SUN.TIME.TIMER_2;

  doc["mqtt"]["SERVER"] = config.mqtt.SERVER;
  doc["mqtt"]["PORT"] = config.mqtt.PORT;
  doc["mqtt"]["USER"] = config.mqtt.USER;
  doc["mqtt"]["PASSWORD"] = config.mqtt.PASSWORD;
  doc["mqtt"]["TOPIC"] = config.mqtt.TOPIC;

  // File file = LittleFS.open(configFileName, "r");
  File file = LittleFS.open(path, "w");
  if (!file) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  if (serializeJson(doc, file) == 0) {
    Serial.println("Failed to write config to file");
    file.close();
    return false;
  }
  file.close();

  Serial.println("Config saved");
  return true;
}

bool loadConfig(const char *path) {
  // File file = LittleFS.open(configFileName, "r");
  File file = LittleFS.open(path, "r");
  if (!file) {
    Serial.println("Failed to open config file");
    return false;
  }

  // size_t size = file.size();
  // if (size > 4096)
  //{
  //     Serial.println("Config file size is too large");
  //     file.close();
  //     return false;
  // }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);

  // char *content = new char[size + 1];
  // file.readBytes(content, size);
  // content[size] = '\0'; // Null-terminating the buffer to make it a valid
  // C-string file.close();         // close the file

  // auto error = deserializeJson(doc, content);

  // delete[] content; // Free the allocated memory from new statement

  if (error) {
    Serial.println("Failed to parse config file");
    Serial.println(error.c_str());
    return false;
  }

  Serial.println("Loading config...");

  config.network.SSID = doc["network"]["SSID"].as<String>();
  config.network.PASSWORD = doc["network"]["PASSWORD"].as<String>();
  config.network.HOSTNAME = doc["network"]["HOSTNAME"].as<String>();
  config.network.NTP_SERVER = doc["network"]["NTP_SERVER"].as<String>();

  config.weather.weatherChannel = doc["weather"]["weather_channel"].as<String>();
  config.weather.API_KEY = doc["weather"]["API_KEY"].as<String>();
  config.weather.METHEO_USERNAME = doc["weather"]["METHEO_USERNAME"].as<String>();
  config.weather.METHEO_PASSWORD = doc["weather"]["METHEO_PASSWORD"].as<String>();
  config.weather.LATITUDE = doc["weather"]["LATITUDE"].as<float>();
  config.weather.LONGITUDE = doc["weather"]["LONGITUDE"].as<float>();
  config.weather.DURATION_PAST = doc["weather"]["DURATION_PAST"].as<int>();
  config.weather.DURATION_FORECAST = doc["weather"]["DURATION_FORECAST"].as<int>();

  config.TIMER.DURATION = doc["timer"]["DURATION"].as<int>();
  config.TIMER.Time_EN = doc["timer"]["EN"].as<bool>();
  config.TIMER.TIME = doc["timer"]["TIME"].as<int>();
  config.TIMER.ALL = doc["timer"]["ALL"].as<bool>();

  config.AUTOMATIC_MODE = doc["AUTOMATIC_MODE"].as<bool>();
  config.PRECIPITATION_MODE = doc["PRECIPITATION_MODE"].as<bool>();
  config.PRECIPITATION_MODE_FORECAST = doc["PRECIPITATION_MODE_FORECAST"].as<bool>();
  config.PRECIPITATION_AMOUNT = doc["PRECIPITATION_AMOUNT"].as<float>();
  config.PRECIPITATION_AMOUNT_FORECAST = doc["PRECIPITATION_AMOUNT_FORECAST"].as<float>();
  config.MENTION_SUNSET = doc["MENTION_SUNSET"].as<bool>();

  config.MAX_PUMP_TIME = doc["MAX_PUMP_TIME"].as<int>();
  config.MAX_VENT1_TIME = doc["MAX_VENT1_TIME"].as<int>();
  config.MAX_VENT2_TIME = doc["MAX_VENT2_TIME"].as<int>();

  config.MON.ON = doc["MON"]["ON"].as<bool>();
  config.MON.TIME.ON_TIME = doc["MON"]["TIME"]["ON_TIME"].as<int>();
  config.MON.TIME.TIMER_1 = doc["MON"]["TIME"]["TIMER_1"].as<int>();
  config.MON.TIME.TIMER_2 = doc["MON"]["TIME"]["TIMER_2"].as<int>();

  config.TUE.ON = doc["TUE"]["ON"].as<bool>();
  config.TUE.TIME.ON_TIME = doc["TUE"]["TIME"]["ON_TIME"].as<int>();
  config.TUE.TIME.TIMER_1 = doc["TUE"]["TIME"]["TIMER_1"].as<int>();
  config.TUE.TIME.TIMER_2 = doc["TUE"]["TIME"]["TIMER_2"].as<int>();

  config.WED.ON = doc["WED"]["ON"].as<bool>();
  config.WED.TIME.ON_TIME = doc["WED"]["TIME"]["ON_TIME"].as<int>();
  config.WED.TIME.TIMER_1 = doc["WED"]["TIME"]["TIMER_1"].as<int>();
  config.WED.TIME.TIMER_2 = doc["WED"]["TIME"]["TIMER_2"].as<int>();

  config.THU.ON = doc["THU"]["ON"].as<bool>();
  config.THU.TIME.ON_TIME = doc["THU"]["TIME"]["ON_TIME"].as<int>();
  config.THU.TIME.TIMER_1 = doc["THU"]["TIME"]["TIMER_1"].as<int>();
  config.THU.TIME.TIMER_2 = doc["THU"]["TIME"]["TIMER_2"].as<int>();

  config.FRI.ON = doc["FRI"]["ON"].as<bool>();
  config.FRI.TIME.ON_TIME = doc["FRI"]["TIME"]["ON_TIME"].as<int>();
  config.FRI.TIME.TIMER_1 = doc["FRI"]["TIME"]["TIMER_1"].as<int>();
  config.FRI.TIME.TIMER_2 = doc["FRI"]["TIME"]["TIMER_2"].as<int>();

  config.SAT.ON = doc["SAT"]["ON"].as<bool>();
  config.SAT.TIME.ON_TIME = doc["SAT"]["TIME"]["ON_TIME"].as<int>();
  config.SAT.TIME.TIMER_1 = doc["SAT"]["TIME"]["TIMER_1"].as<int>();
  config.SAT.TIME.TIMER_2 = doc["SAT"]["TIME"]["TIMER_2"].as<int>();

  config.SUN.ON = doc["SUN"]["ON"].as<bool>();
  config.SUN.TIME.ON_TIME = doc["SUN"]["TIME"]["ON_TIME"].as<int>();
  config.SUN.TIME.TIMER_1 = doc["SUN"]["TIME"]["TIMER_1"].as<int>();
  config.SUN.TIME.TIMER_2 = doc["SUN"]["TIME"]["TIMER_2"].as<int>();

  config.mqtt.SERVER = doc["mqtt"]["SERVER"].as<String>();
  config.mqtt.PORT = doc["mqtt"]["PORT"].as<String>();
  config.mqtt.USER = doc["mqtt"]["USER"].as<String>();
  config.mqtt.PASSWORD = doc["mqtt"]["PASSWORD"].as<String>();
  config.mqtt.TOPIC = doc["mqtt"]["TOPIC"].as<String>();

  Serial.println("config loaded");

  return true;
}

bool resetConfig() {
  // Network Config
  config.network.SSID = "WIFISSID";
  config.network.PASSWORD = "WifiPassword";
  config.network.HOSTNAME = "DeviceHostname";
  config.network.NTP_SERVER = "ptbtime1.ptb.de";

  // Weather Config
  config.weather.weatherChannel = "openweather";
  config.weather.API_KEY = "api_key";
  config.weather.METHEO_USERNAME = "username";
  config.weather.METHEO_PASSWORD = "password";
  config.weather.LATITUDE = 52.3759;
  config.weather.LONGITUDE = 9.7320;
  config.weather.DURATION_PAST = 24;
  config.weather.DURATION_FORECAST = 24;

  // Timer Config
  config.TIMER.DURATION = 30;
  config.TIMER.TIME = -1;
  config.TIMER.Time_EN = false; // Nicht in JSON spezifiziert, auf false gesetzt
  config.TIMER.ALL = false;

  // Other settings
  config.AUTOMATIC_MODE = false;
  config.PRECIPITATION_MODE = false;
  config.PRECIPITATION_MODE_FORECAST = false;
  config.PRECIPITATION_AMOUNT = 20.0;
  config.PRECIPITATION_AMOUNT_FORECAST = 20.0;
  config.MENTION_SUNSET = false;

  config.MAX_PUMP_TIME = 180;
  config.MAX_VENT1_TIME = 180;
  config.MAX_VENT2_TIME = 180;

  // Day configs
  DayConfig *dayConfigs[] = {&config.MON, &config.TUE, &config.WED, &config.THU,
                             &config.FRI, &config.SAT, &config.SUN};
  for (int i = 0; i < 7; i++) {
    dayConfigs[i]->ON = false;
    dayConfigs[i]->TIME.ON_TIME = -1;
    dayConfigs[i]->TIME.TIMER_1 = 0;
    dayConfigs[i]->TIME.TIMER_2 = 0;
  }

  // MQTT Config
  config.mqtt.SERVER = "mqtt.example.com";
  config.mqtt.PORT = "1883";
  config.mqtt.USER = "username";
  config.mqtt.PASSWORD = "password";
  config.mqtt.TOPIC = "device/status";

  saveConfig(configFilePath);

  Serial.println("and resetted");
  return true;
}