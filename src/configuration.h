#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>

#include "config.h"

extern Config config;
extern const char *configFilePath;

bool saveConfig(const char *path);
bool loadConfig(const char *path);
bool resetConfig();

#endif  // CONFIGURATION_H
