#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <FS.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#include <WiFi.h>

#include <ElegantOTA.h>
#include <WebSerial.h>

#include "configuration.h"
#include "ntp.h"
#include "sensors.h"
#include "weather.h"

#if !(defined(ESP32))
#error This code is intended to run on the ESP32 platform! Please check your Tools->Board setting.
#endif

//  You only need to format LittleFS the first time you run a
//  test or else use the LITTLEFS plugin to create a partition
//  https://github.com/lorol/arduino-esp32littlefs-plugin
#define FORMAT_LITTLEFS_IF_FAILED true

const unsigned long RESTART_INTERVAL = 30UL * 24UL * 60UL * 60UL * 1000UL; // 30 Tage in Millisekunden

// configFilePath is at /config.json, declared in configuration.cpp

// MARK: PINOUT
const int vent_1_button = 36; // the number of the pushbutton pin for vent 1
const int vent_1_led = 27;    // the number of the LED pin for light indicator of vent 1
const int vent_2_button = 39; // the number of the pushbutton pin for vent 2
const int vent_2_led = 32;    // the number of the LED pin for light indicator of vent 2
const int pump_button = 34;   // the number of the pushbutton pin for pump
const int pump_led = 23;      // the number of the LED pin for light indicator of pump
const int action_button = 35; // the number of the pushbutton pin for action
const int action_led = 13;    // the number of the LED pin for light indicator of action
const int vent_1_relay = 19;  // the number of the relay pin for vent 1
const int vent_2_relay = 18;  // the number of the relay pin for vent 2
const int pump_relay = 17;    // the number of the relay pin for pump
const int action_relay = 16;  // the number of the relay pin for action

const int groundHumidityPin = 33;
// const int groundHumidityPinDigital = 25;
const int OneWireBus = 26;

// const int data1 = 14;
// const int data2 = 15;
// const int data3 = 5;

const int DHTPIN = 4; // Digital pin connected to the DHT sensor

const char *ntpServer1 = "ptbtime1.ptb.de";
const char *ntpServer2 = "pool.ntp.org";
const char *Timezone = "CET-1CEST,M3.5.0/2,M10.5.0/3";
// Example time zones
// const char* Timezone = "GMT0BST,M3.5.0/01,M10.5.0/02";     // UK
// const char* Timezone = "MET-2METDST,M3.5.0/01,M10.5.0/02"; // Most of Europe
// const char* Timezone = "CET-1CEST,M3.5.0/2,M10.5.0/3";     // Central Europe
// const char* Timezone = "EST5EDT,M3.2.0,M11.1.0";           // EST USA
// const char* Timezone = "CST6CDT,M3.2.0,M11.1.0";           // CST USA
// const char* Timezone = "MST7MDT,M4.1.0,M10.5.0";           // MST USA
// const char* Timezone = "NZST-12NZDT,M9.5.0,M4.1.0/3";      // Auckland
// const char* Timezone = "EET-2EEST,M3.5.5/0,M10.5.5/0";     // Asia
// const char* Timezone = "ACST-9:30ACDT,M10.1.0,M4.1.0/3":   // Australia

// MARK: Variables
// deckare variables
int pumpEndTime = -1;
int vent1EndTime = -1;
int vent2EndTime = -1;
int actionEndTime = -1;

int pumpStartTime = -1;
int vent1StartTime = -1;
int vent2StartTime = -1;
int actionStartTime = -1;

int currentTime = -1;

bool pumpState = false;
bool vent1State = false;
bool vent2State = false;
bool actionState = false;

bool setTimer1Active = false;
bool setTimer2Active = false;

bool timer1Active = false;
bool timer2Active = false;

bool TimerLogicActive = false;

int timerTyp = 0;

const unsigned long debounceDelay = 20; // the debounce time to avoid flickers
int lastVent1State = LOW;
int lastVent2State = LOW;
int lastPumpState = LOW;
int lastActionState = LOW;
int Vent1ButtonState;
int Vent2ButtonState;
int PumpButtonState;
int ActionButtonState;

unsigned long lastVent1DebounceTime = 0;
unsigned long lastVent2DebounceTime = 0;
unsigned long lastPumpDebounceTime = 0;
unsigned long lastActionDebounceTime = 0;

bool alreadyPrinted1 = false; // for serial output for timer 1 with enable time
bool alreadyPrinted2 = false; // for serial output for timer 2 with enable time

// create objects
AsyncWebServer server(80); // Create AsyncWebServer object on port 80
// AsyncWebSocket ws("/ws");   // Create WebSockets object
DNSServer dns; // Wifi Manager DNS Server for manager website

void reboot() {
    // reboots the esp
    delay(1000);
    ESP.restart();
    delay(5000);
}

void checkReboot() {
    // check if the esp should be rebooted
    // updateLocaleTime();
    // check reboot at 3:00
    if (timeinfo.tm_hour == 3 && (timeinfo.tm_min >= 0 && timeinfo.tm_min <= 1)) {
        if (millis() > RESTART_INTERVAL) {
            Serial.println("rebooting due to reboot interval");
            delay(400); // delay to allow serial to print
            reboot();
        }
    }
}

// MARK: Wifi
boolean setNewWiFi(String ssid, String password) {
    // MARK: setNewWiFi
    WiFi.disconnect();
    WiFi.begin(ssid.c_str(), password.c_str());

    Serial.print("New SSID: ");
    Serial.println(ssid);
    Serial.print("New password: ");
    Serial.println(password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to new network!");
    return true;
}

void getWiFiCredentials() {
    // Get ssid and password
    String ssid = WiFi.SSID();
    String password = WiFi.psk();
}

// MARK: Time calculations
//  convert time input from HH:MM to intger HHMM
int convertTimeToInt(const String &timeStr) {
    if (timeStr.length() == 0) {
        return -1; // Return -1 if string is empty
    }

    int hour = timeStr.substring(0, 2).toInt();
    int minute = timeStr.substring(3, 5).toInt();

    return hour * 100 + minute;
}

String convertTimeToString(int timeInt) {
    if (timeInt == -1) {
        return ""; // Return empty string if time is -1
    }
    int hour = timeInt / 100;
    int minute = timeInt % 100;

    char buffer[6];
    snprintf(buffer, sizeof(buffer), "%02d:%02d", hour, minute);

    return String(buffer);
}

// calculate time in format HHMM
int calculateTime(int time, int duration) {
    int hour = time / 100;
    int minute = time % 100;

    minute += duration;
    while (minute >= 60) {
        hour += 1;
        minute -= 60;
    }
    hour = hour % 24;

    return hour * 100 + minute;
}

// returns true if the current time is not in the time slot
bool calculateSecurityCheck(int startTime, int duration) {
    int maxEndTime = calculateTime(startTime, duration);
    currentTime = getCurrentTimeInt();
    if (maxEndTime < startTime) {
        return currentTime < startTime && currentTime > maxEndTime;
    }
    return currentTime > maxEndTime;
}

// MARK: init LittleFS
//  Initialize LitlleFS
void initLittleFS() {
    if (!LittleFS.begin()) {
        Serial.println("An Error has occurred while mounting LittleFS");
        return;
    }
    Serial.println("LittleFS initialized.");

    /*
    if(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
      Serial.println("An Error has occurred while mounting LittleFS");
      return;
    */
}

// MARK: initPins
void initPins() {
    pinMode(vent_1_button, INPUT);
    pinMode(vent_2_button, INPUT);
    pinMode(pump_button, INPUT);
    pinMode(action_button, INPUT);

    pinMode(vent_1_led, OUTPUT);
    pinMode(vent_2_led, OUTPUT);
    pinMode(pump_led, OUTPUT);
    pinMode(action_led, OUTPUT);

    pinMode(vent_1_relay, OUTPUT);
    pinMode(vent_2_relay, OUTPUT);
    pinMode(pump_relay, OUTPUT);
    pinMode(action_relay, OUTPUT);

    pinMode(groundHumidityPin, INPUT); // not necessary due to ADC pin

    // INPUT and OUTPUT, digital and analog reading seems to be possible
    // pinMode(groundHumidityPinDigital, INPUT);
    // pinMode(OneWireBus, INPUT); // not necessary due to OneWire Library
    // pinMode(data1, INPUT);
    // pinMode(data2, INPUT);
    // pinMode(data3, INPUT);
}

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index,
                  uint8_t *data, size_t len, bool final) {
    // Check if the file is a .json file
    if (!filename.endsWith(".json")) {
        request->send(400, "text/plain", "Only .json files are allowed");
        return;
    }

    // Use fixed filename "config.json" or keep the uploaded filename
    // String filePath = "/" + filename;
    // For fixed filename:
    // const char *filePath = "/config_new.json";
    File file = LittleFS.open(configFilePath, "w");
    if (!index) {
        // Datei zum Schreiben öffnen
        Serial.printf("UploadStart: %s\n", filename.c_str());
        if (!file) {
            request->send(500, "text/plain", "Could not open file for writing");
            return;
        }
    }

    // Daten in die Datei schreiben
    if (len) {
        file.write(data, len);
    }
    file.close();

    if (final) {
        Serial.printf("UploadEnd: %s, %u B\n", filename.c_str(), index + len);
        request->send(200, "text/plain", "File upload completed");
    }
    if (!loadConfig(configFilePath)) {
        Serial.println("Failed to load config file");
    }
}

// MARK: Emergency
bool handleEmergency() {
    digitalWrite(vent_1_relay, LOW);
    digitalWrite(vent_2_relay, LOW);
    digitalWrite(pump_relay, LOW);
    digitalWrite(action_relay, LOW);
    digitalWrite(vent_1_led, LOW);
    digitalWrite(vent_2_led, LOW);
    digitalWrite(pump_led, LOW);
    digitalWrite(action_led, LOW);

    vent1State = false;
    vent2State = false;
    pumpState = false;
    actionState = false;

    delay(10);

    bool allLow = (vent1State == false) &&
                  (vent2State == false) &&
                  (pumpState == false) &&
                  (actionState == false) &&
                  (digitalRead(vent_1_relay) == LOW) &&
                  (digitalRead(vent_2_relay) == LOW) &&
                  (digitalRead(pump_relay) == LOW) &&
                  (digitalRead(action_relay) == LOW) &&
                  (digitalRead(vent_1_led) == LOW) &&
                  (digitalRead(vent_2_led) == LOW) &&
                  (digitalRead(pump_led) == LOW) &&
                  (digitalRead(action_led) == LOW);

    return allLow;
}

// MARK: notFound
//  Handler for the 404 not found page
void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "404 - Not found");
}

// MARK: Setup begin
void setup() {
    // Serial port for debugging purposes
    Serial.begin(115200);

    // MARK: WiFiManager
    // Local intialization. Once its business is done, there is no need to keep it
    // around
    AsyncWiFiManager wifiManager(&server, &dns);

    // reset settings - for testing only
    // wifiManager.resetSettings();

    // sets timeout until configuration portal gets turned off, useful to make it
    // all retry or go to sleep
    wifiManager.setTimeout(180); // in seconds

    // set custom ip for portal
    wifiManager.setAPStaticIPConfig(IPAddress(10, 0, 1, 4), IPAddress(10, 0, 1, 1), IPAddress(255, 255, 255, 0));

    // set minimu quality of signal so it ignores AP's under that quality
    // defaults to 8%
    // wifiManager.setMinimumSignalQuality();

    // wifiManager.setRemoveDuplicateAPs(false);

    // wifiManager.setTryConnectDuringConfigPortal(true);

    // tries to connect to last known settings
    // if it does not connect it starts an access point with the specified name,
    // here  "Pumpenwifi" with password "password" and goes into a blocking loop
    // awaiting configuration
    if (!wifiManager.autoConnect("Pumpenwifi", "Garten24")) {
        Serial.println("failed to connect --- hit timeout --- rebooting ESP...");
        delay(3000);
        ESP.restart();
        delay(5000);
    }
    Serial.printf("Connected to %s\n", WiFi.SSID().c_str());
    Serial.println("");

    initLittleFS();
    initPins();
    setupDHT();

    loadConfig(configFilePath);

    // Print ESP Local IP Address
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    // sets a custom hostname for website
    WiFi.setHostname(config.network.HOSTNAME.c_str());
    Serial.print("Hostname: ");
    Serial.println(WiFi.getHostname());

    config.network.SSID = WiFi.SSID().c_str();
    saveConfig(configFilePath);

    // Handle the config download button
    // server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request)
    //          { request->send(LittleFS, "/config.json", String(), true); });
    server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request) {
        Serial.println("Download config file");
        request->send(LittleFS, configFilePath, String(), true);
    });

    // define Upload-Handler
    server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) { request->send(200); }, handleUpload);

    // define Emergency-Handler
    server.on("/emergency", HTTP_GET, [](AsyncWebServerRequest *request) {
        bool result = handleEmergency();
        Serial.println("Emergency shutoff initiated");
        if (result) {
            Serial.println("Emergency shutoff successful");
        } else {
            Serial.println("Emergency shutoff failed, retrying...");
            handleEmergency();
        }
        request->send(200, "text/plain", "emergency initiated! Success: " + result);
    });

    server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request) {
        Serial.println("rebooting initiated");
        request->send(200, "application/json", "{\"status\":\"true\"}");
        reboot();
    });

    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
        bool result = resetConfig();
        Serial.println("Reset initiated: Success: " + String(result));
        request->send(200, "text/plain", "Reset initiated! Success: " + String(result));
    });

    // Handle POST request to set timer
    // MARK: timer
    server.on("/set-timer", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, data);
            if (error) {
                request->send(400, "application/json", "{\"status\":\"error\", \"message\":\"Invalid JSON\"}");
                return;
            }

            config.TIMER.DURATION = doc["value"];
            config.TIMER.ALL = doc["all"];
            config.TIMER.Time_EN = doc["onTimeEN"].as<bool>();
            config.TIMER.TIME = convertTimeToInt(doc["onTime"]);

            // Timer-Logik
            timerTyp = doc["timer"];
            switch (timerTyp) {
            case 1:
                // Timer 1
                setTimer1Active = true;
                break;
            case 2:
                // Timer 2
                setTimer2Active = true;
                break;
            case 3:
                // Timer all
                setTimer1Active = true;
                setTimer2Active = true;
                break;
            default:
                // default case
                break;
            }

            Serial.println("Timer data received");

            request->send(200, "application/json", "{\"status\":\"true\"}"); });

    // Handle GET request to get timer
    server.on("/get-timer", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["value"] = config.TIMER.DURATION;
        doc["all"] = config.TIMER.ALL;
        doc["onTimeEN"] = config.TIMER.Time_EN;
        doc["onTime"] = convertTimeToString(config.TIMER.TIME);

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
        Serial.println("Timer data send");
    });

    // Handle stop timers
    server.on("/stop-timer", HTTP_GET, [](AsyncWebServerRequest *request) {
        timer1Active = false;
        timer2Active = false;
        setTimer1Active = false;
        setTimer2Active = false;

        pumpState = false;
        vent1State = false;
        vent2State = false;
        actionState = false;

        request->send(200, "application/json", "{\"status\":\"true\"}");
        Serial.println("All timers stopped");
    });

    // Handle POST request to set timeclock
    // MARK: timetable
    server.on("/set-timeclock", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, data);
            if (error) {
                request->send(400, "application/json", "{\"status\":\"error\", \"message\":\"Invalid JSON\"}");
                return;
            }

            // Update settings for each day
            config.MON.ON = doc["monday"]["onTimeSwitch"];
            config.MON.TIME.ON_TIME = convertTimeToInt(doc["monday"]["onTime"]);
            config.MON.TIME.TIMER_1 = doc["monday"]["timer1"];
            config.MON.TIME.TIMER_2 = doc["monday"]["timer2"];

            config.TUE.ON = doc["tuesday"]["onTimeSwitch"];
            config.TUE.TIME.ON_TIME = convertTimeToInt(doc["tuesday"]["onTime"]);
            config.TUE.TIME.TIMER_1 = doc["tuesday"]["timer1"];
            config.TUE.TIME.TIMER_2 = doc["tuesday"]["timer2"];

            config.WED.ON = doc["wednesday"]["onTimeSwitch"];
            config.WED.TIME.ON_TIME = convertTimeToInt(doc["wednesday"]["onTime"]);
            config.WED.TIME.TIMER_1 = doc["wednesday"]["timer1"];
            config.WED.TIME.TIMER_2 = doc["wednesday"]["timer2"];

            config.THU.ON = doc["thursday"]["onTimeSwitch"];
            config.THU.TIME.ON_TIME = convertTimeToInt(doc["thursday"]["onTime"]);
            config.THU.TIME.TIMER_1 = doc["thursday"]["timer1"];
            config.THU.TIME.TIMER_2 = doc["thursday"]["timer2"];

            config.FRI.ON = doc["friday"]["onTimeSwitch"];
            config.FRI.TIME.ON_TIME = convertTimeToInt(doc["friday"]["onTime"]);
            config.FRI.TIME.TIMER_1 = doc["friday"]["timer1"];
            config.FRI.TIME.TIMER_2 = doc["friday"]["timer2"];

            config.SAT.ON = doc["saturday"]["onTimeSwitch"];
            config.SAT.TIME.ON_TIME = convertTimeToInt(doc["saturday"]["onTime"]);
            config.SAT.TIME.TIMER_1 = doc["saturday"]["timer1"];
            config.SAT.TIME.TIMER_2 = doc["saturday"]["timer2"];

            config.SUN.ON = doc["sunday"]["onTimeSwitch"];
            config.SUN.TIME.ON_TIME = convertTimeToInt(doc["sunday"]["onTime"]);
            config.SUN.TIME.TIMER_1 = doc["sunday"]["timer1"];
            config.SUN.TIME.TIMER_2 = doc["sunday"]["timer2"];

            request->send(200, "application/json", "{\"status\":\"true\"}");
            Serial.println("Timetable data received"); });

    // Handle GET request to get timeclock settings
    server.on("/get-timeclock", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;

        doc["monday"]["onTimeSwitch"] = config.MON.ON;
        doc["monday"]["onTime"] = convertTimeToString(config.MON.TIME.ON_TIME);
        doc["monday"]["timer1"] = config.MON.TIME.TIMER_1;
        doc["monday"]["timer2"] = config.MON.TIME.TIMER_2;

        doc["tuesday"]["onTimeSwitch"] = config.TUE.ON;
        doc["tuesday"]["onTime"] = convertTimeToString(config.TUE.TIME.ON_TIME);
        doc["tuesday"]["timer1"] = config.TUE.TIME.TIMER_1;
        doc["tuesday"]["timer2"] = config.TUE.TIME.TIMER_2;

        doc["wednesday"]["onTimeSwitch"] = config.WED.ON;
        doc["wednesday"]["onTime"] = convertTimeToString(config.WED.TIME.ON_TIME);
        doc["wednesday"]["timer1"] = config.WED.TIME.TIMER_1;
        doc["wednesday"]["timer2"] = config.WED.TIME.TIMER_2;

        doc["thursday"]["onTimeSwitch"] = config.THU.ON;
        doc["thursday"]["onTime"] = convertTimeToString(config.THU.TIME.ON_TIME);
        doc["thursday"]["timer1"] = config.THU.TIME.TIMER_1;
        doc["thursday"]["timer2"] = config.THU.TIME.TIMER_2;

        doc["friday"]["onTimeSwitch"] = config.FRI.ON;
        doc["friday"]["onTime"] = convertTimeToString(config.FRI.TIME.ON_TIME);
        doc["friday"]["timer1"] = config.FRI.TIME.TIMER_1;
        doc["friday"]["timer2"] = config.FRI.TIME.TIMER_2;

        doc["saturday"]["onTimeSwitch"] = config.SAT.ON;
        doc["saturday"]["onTime"] = convertTimeToString(config.SAT.TIME.ON_TIME);
        doc["saturday"]["timer1"] = config.SAT.TIME.TIMER_1;
        doc["saturday"]["timer2"] = config.SAT.TIME.TIMER_2;

        doc["sunday"]["onTimeSwitch"] = config.SUN.ON;
        doc["sunday"]["onTime"] = convertTimeToString(config.SUN.TIME.ON_TIME);
        doc["sunday"]["timer1"] = config.SUN.TIME.TIMER_1;
        doc["sunday"]["timer2"] = config.SUN.TIME.TIMER_2;

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
        Serial.println("Timetable data send");
    });

    // Main page handlers
    // MARK: main-page
    server.on("/home-status", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json;
        JsonDocument doc;

        doc["temperature"] = readDHTTemperature();
        doc["humidity"] = readDHTHumidity();
        // doc["ground_temp"] = readGroundTemperature();
        doc["ground_temp"] = 0;
        doc["ground_humidity"] = readGroundHumidity();
        doc["nds24h"] = rain24h;
        doc["nds24h_text"] = config.weather.DURATION_PAST;
        doc["nds24h_next"] = rain24h;
        doc["nds24h_next_text"] = config.weather.DURATION_FORECAST;

        // implement which circle due to which circle is active
        if (timer1Active) {
            doc["kreis1_timer"] =
                "Aktiv: " + String(config.TIMER.DURATION) +
                " min bis: " + convertTimeToString(vent1EndTime);
        } else if (setTimer1Active && config.TIMER.Time_EN && (timerTyp == 1 || timerTyp == 3)) {
            doc["kreis1_timer"] =
                "Aktiv ab: " + convertTimeToString(config.TIMER.TIME) +
                " für " + String(config.TIMER.DURATION) +
                " min bis: " + convertTimeToString(vent1EndTime);
        } else {
            doc["kreis1_timer"] = "Timer nicht aktiv";
        }
        if (timer2Active) {
            doc["kreis2_timer"] =
                "Aktiv: " + String(config.TIMER.DURATION) +
                " min bis: " + convertTimeToString(vent2EndTime);
        } else if (setTimer2Active && config.TIMER.Time_EN && (timerTyp == 2 || timerTyp == 3)) {
            if (timerTyp == 2) {
                doc["kreis2_timer"] =
                    "Aktiv ab: " + convertTimeToString(config.TIMER.TIME) +
                    " für " + String(config.TIMER.DURATION) +
                    " min bis: " + convertTimeToString(vent2EndTime);
            } else {
                doc["kreis2_timer"] =
                    "Aktiv ab: " + convertTimeToString(calculateTime(config.TIMER.TIME, config.TIMER.DURATION)) +
                    " für " + String(config.TIMER.DURATION) +
                    " min bis: " + convertTimeToString(calculateTime(config.TIMER.TIME, (2 * config.TIMER.DURATION)));
            }
        } else {
            doc["kreis2_timer"] = "Timer nicht aktiv";
        }

        doc["vent1_button"] = vent1State;
        doc["vent2_button"] = vent2State;
        doc["pump_button"] = pumpState;

        doc["time_control"] = config.AUTOMATIC_MODE;
        doc["nds_active"] = config.PRECIPITATION_MODE;
        doc["nds_active_forecast"] = config.PRECIPITATION_MODE_FORECAST;
        doc["mentions_sunset"] = config.MENTION_SUNSET;

        serializeJson(doc, json);
        request->send(200, "application/json", json);
    });
    server.on("/home-timetable", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json;
        JsonDocument doc;

        const char *days[] = {"monday", "tuesday", "wednesday", "thursday",
                              "friday", "saturday", "sunday"};
        DayConfig *dayConfigs[] = {&config.MON, &config.TUE, &config.WED,
                                   &config.THU, &config.FRI, &config.SAT,
                                   &config.SUN};

        for (int i = 0; i < 7; i++) {
            // JsonObject day = doc.createNestedObject(days[i]); //OLD
            JsonObject day = doc[days[i]].to<JsonObject>();
            day["on"] = dayConfigs[i]->ON;
            day["on_time"] = convertTimeToString(dayConfigs[i]->TIME.ON_TIME);
            day["timer1"] = dayConfigs[i]->TIME.TIMER_1;
            day["timer2"] = dayConfigs[i]->TIME.TIMER_2;
        }

        serializeJson(doc, json);
        request->send(200, "application/json", json);
    });

    server.on("/activateAutoMode", HTTP_GET, [](AsyncWebServerRequest *request) {
        config.AUTOMATIC_MODE = !config.AUTOMATIC_MODE;
        Serial.println("Automatic mode set to: " + String(config.AUTOMATIC_MODE ? "true" : "false"));
        request->send(200, "application/json", "{\"status\":" + String(config.AUTOMATIC_MODE ? "true" : "false") + "}");
    });
    server.on("/activateRain", HTTP_GET, [](AsyncWebServerRequest *request) {
        config.PRECIPITATION_MODE = !config.PRECIPITATION_MODE;
        Serial.println("Evaluate rain mode set to: " + String(config.PRECIPITATION_MODE ? "true" : "false"));
        request->send(200, "application/json", "{\"status\":" + String(config.PRECIPITATION_MODE ? "true" : "false") + "}");
    });
    server.on("/activateRainForecast", HTTP_GET, [](AsyncWebServerRequest *request) {
        config.PRECIPITATION_MODE_FORECAST = !config.PRECIPITATION_MODE_FORECAST;
        Serial.println("Evaluate rain forecast mode set to: " + String(config.PRECIPITATION_MODE_FORECAST ? "true" : "false"));
        request->send(200, "application/json", "{\"status\":" + String(config.PRECIPITATION_MODE_FORECAST ? "true" : "false") + "}");
    });
    server.on("/mentionSunset", HTTP_GET, [](AsyncWebServerRequest *request) {
        config.MENTION_SUNSET = !config.MENTION_SUNSET;
        Serial.println("Mention sunset mode set to: " + String(config.MENTION_SUNSET ? "true" : "false"));
        request->send(200, "application/json", "{\"status\":" + String(config.MENTION_SUNSET ? "true" : "false") + "}");
    });

    server.on("/togglePump", HTTP_GET, [](AsyncWebServerRequest *request) {
        pumpState = !pumpState;
        pumpStartTime = pumpState ? getCurrentTimeInt() : -1;
        Serial.println("Pump set to: " + String(pumpState ? "active" : "off"));
        request->send(200, "application/json", "{\"status\":" + String(pumpState ? "true" : "false") + "}");
    });
    server.on("/toggleVent1", HTTP_GET, [](AsyncWebServerRequest *request) {
        vent1State = !vent1State;
        vent1StartTime = vent1State ? getCurrentTimeInt() : -1;
        Serial.println("Vent1 set to: " + String(vent1State ? "active" : "off"));
        request->send(200, "application/json", "{\"status\":" + String(vent1State ? "true" : "false") + "}");
    });
    server.on("/toggleVent2", HTTP_GET, [](AsyncWebServerRequest *request) {
        vent2State = !vent2State;
        vent2StartTime = vent2State ? getCurrentTimeInt() : -1;
        Serial.println("Vent2 set to: " + String(vent2State ? "active" : "off"));
        request->send(200, "application/json", "{\"status\":" + String(vent2State ? "true" : "false") + "}");
    });

    // settings endpoints
    // MARK: settings
    server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json;
        JsonDocument doc;

        doc["ntp"] = config.network.NTP_SERVER;
        doc["ssid"] = config.network.SSID;
        doc["hostname"] = config.network.HOSTNAME;
        doc["precipitation_level"] = config.PRECIPITATION_AMOUNT;
        doc["precipitation_level_forecast"] = config.PRECIPITATION_AMOUNT_FORECAST;
        doc["max_pump_time"] = config.MAX_PUMP_TIME;
        doc["max_vent1_time"] = config.MAX_VENT1_TIME;
        doc["max_vent2_time"] = config.MAX_VENT2_TIME;
        doc["weather_channel"] = config.weather.weatherChannel;
        doc["longitude"] = config.weather.LONGITUDE;
        doc["latitude"] = config.weather.LATITUDE;
        doc["rain_duration_past"] = config.weather.DURATION_PAST;
        doc["rain_duration_forecast"] = config.weather.DURATION_FORECAST;
        doc["openweather_api_key"] = config.weather.API_KEY;

        if (config.weather.weatherChannel == "metehomatics") {
            doc["metehomatics_user"] = config.weather.METHEO_USERNAME;
            doc["metehomatics_password"] = config.weather.METHEO_PASSWORD;
        }

        serializeJson(doc, json);
        request->send(200, "application/json", json);
        Serial.println("Settings data send");
    });

    server.on("/settings", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, data);
        if (error) {
            request->send(400, "application/json",
                          "{\"status\":\"error\", \"message\":\"Invalid JSON\"}");
            return;
        }
        if (doc.containsKey("zeitserver")) {
            config.network.NTP_SERVER = doc["zeitserver"].as<String>();
        }
        if (doc.containsKey("wifi_ssid")) {
            config.network.SSID = doc["wifi_ssid"].as<String>();
        }
        if (doc.containsKey("wifi_pw")) {
            config.network.PASSWORD = doc["wifi_pw"].as<String>();
        }
        if (doc.containsKey("hostname")) {
            config.network.HOSTNAME = doc["hostname"].as<String>();
        }
        if (doc.containsKey("precipitation_level")) {
            config.PRECIPITATION_AMOUNT = doc["precipitation_level"];
        }
        if (doc.containsKey("precipitation_level_forecast")) {
            config.PRECIPITATION_AMOUNT_FORECAST = doc["precipitation_level_forecast"];
        }
        if (doc.containsKey("weather_channel")) {
            config.weather.weatherChannel = doc["weather_channel"].as<String>();
        }
        if (doc.containsKey("longitude")) {
            config.weather.LONGITUDE = doc["longitude"];
        }
        if (doc.containsKey("latitude")) {
            config.weather.LATITUDE = doc["latitude"];
        }
        if (doc.containsKey("rain_duration_past")) {
            config.weather.DURATION_PAST = doc["rain_duration_past"];
        }
        if (doc.containsKey("rain_duration_forecast")) {
            config.weather.DURATION_FORECAST = doc["rain_duration_forecast"];
        }
        if (config.weather.weatherChannel == "openweather") {
            if (doc.containsKey("api_key")) {
                config.weather.API_KEY = doc["api_key"].as<String>();
            }
        } else if (config.weather.weatherChannel == "metehomatics") {
            if (doc.containsKey("meteho_user")) {
                config.weather.METHEO_USERNAME = doc["meteho_user"].as<String>();
            }
            if (doc.containsKey("meteho_pw")) {
                config.weather.METHEO_PASSWORD = doc["meteho_pw"].as<String>();
            }
        }
        if (doc.containsKey("max_pump_time")) {
            config.MAX_PUMP_TIME = doc["max_pump_time"];
        }
        if (doc.containsKey("max_vent1_time")) {
            config.MAX_VENT1_TIME = doc["max_vent1_time"];
        }
        if (doc.containsKey("max_vent2_time")) {
            config.MAX_VENT2_TIME = doc["max_vent2_time"];
        }
        saveConfig(configFilePath);
        request->send(200, "application/json", "{\"status\":\"true\"}");
        Serial.println("Settings data received"); });

    // send esp chip info for nerds
    // MARK: chip-info
    server.on("/chip-info", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;

        doc["version"] = config.VERSION;
        doc["ip"] = WiFi.localIP(); //.toString()
        doc["mac"] = WiFi.macAddress();
        doc["subnet"] = WiFi.subnetMask(); //.toString()
        doc["ssid"] = WiFi.SSID();
        doc["gateway"] = WiFi.gatewayIP(); //.toString()
        doc["hostname"] = WiFi.getHostname();
        doc["kernel"] = ESP.getSdkVersion();
        doc["chip_id"] = ESP.getChipModel(); // String(ESP.getChipModel(), HEX);
        doc["flash_id"] = ESP.getEfuseMac(); // String(ESP.getEfuseMac(), HEX);

        // Umrechnung in Kilobytes (KB)
        float flashSizeKB = ESP.getFlashChipSize() / 1024.0;
        float sketchSizeKB = ESP.getSketchSize() / 1024.0;
        float freeSpaceKB = ESP.getFreeSketchSpace() / 1024.0;
        float freeHeapKB = ESP.getFreeHeap() / 1024.0;
        float FSSizeKB = LittleFS.totalBytes() / 1024.0;
        float FSusedKB = LittleFS.usedBytes() / 1024.0;

        doc["flash_size"] = String(flashSizeKB);
        doc["sketch_size"] = String(sketchSizeKB);
        doc["free_space"] = String(freeSpaceKB);
        doc["free_heap"] = String(freeHeapKB);
        doc["fs_size"] = String(FSSizeKB);
        doc["fs_used"] = String(FSusedKB);

        // doc["flash_size"] = ESP.getFlashChipSize();
        // doc["sketch_size"] = ESP.getSketchSize();
        // doc["free_space"] = ESP.getFreeSketchSpace();
        // doc["free_heap"] = ESP.getFreeHeap();
        // doc["fs_size"] = LittleFS.totalBytes();
        // doc["fs_used"] = LittleFS.usedBytes();

        doc["cpu_freq"] = ESP.getCpuFreqMHz();

        unsigned long runTime = millis();
        unsigned long days = runTime / (24UL * 60 * 60 * 1000);
        unsigned long hours = (runTime % (24UL * 60 * 60 * 1000)) / (60 * 60 * 1000);
        unsigned long minutes = (runTime % (60UL * 60 * 1000)) / (60 * 1000);
        String runtime = String(days) + " Tage, " + String(hours) + " Stunden, " + String(minutes) + " Minuten";
        doc["esp_runtime"] = runtime;

        // unsigned long runTime = millis() / 60000UL;
        // doc["esp_runtime"] = String(runTime) + " min";

        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
        Serial.println("Chip info send");
    });

    // MARK: serve index
    //  Web Server Root URL
    // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    //          { request->send(LittleFS, "/index.html", "text/html"); });
    // server.serveStatic("/", LittleFS, "/");
    // alternative:
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    ElegantOTA.begin(&server); // Start ElegantOTA
    WebSerial.begin(&server);  // Initialize WebSerial

    // Attach a callback function to handle incoming messages
    WebSerial.onMessage([](uint8_t *data, size_t len) {
        Serial.printf("Received %lu bytes from WebSerial: ", len);
        Serial.write(data, len);
        Serial.println();
        WebSerial.println("Received Data...");
        String d = "";
        for (size_t i = 0; i < len; i++) {
            d += char(data[i]);
        }
        WebSerial.println(d);
    });

    server.onNotFound(notFound);
    server.begin();

    // Setup rtc time
    while (!syncTimeWithNTP()) {
        Serial.println("Failed to sync time with NTP server. Trying again in a second...");
        delay(1000);
    }
}

// MARK: Security Check
void securityCheck() {
    if (vent1State && calculateSecurityCheck(vent1StartTime, config.MAX_VENT1_TIME)) {
        vent1State = false;
        vent1StartTime = -1;
        Serial.println("Max Vent1 Time reached. Turned off for security reasons");
    }
    if (vent2State && calculateSecurityCheck(vent2StartTime, config.MAX_VENT2_TIME)) {
        vent2State = false;
        vent2StartTime = -1;
        Serial.println("Max Vent2 Time reached. Turned off for security reasons");
    }
    if (pumpState && calculateSecurityCheck(pumpStartTime, config.MAX_PUMP_TIME)) {
        pumpState = false;
        pumpStartTime = -1;
        Serial.println("Max Pump Time reached. Turned off for security reasons");
    }
    // if (actionState && calculateSecurityCheck(actionStartTime, config.MAX_ACTION_TIME)) {
    //     actionState = false;
    //     actionStartTime = -1;
    //      Serial.println("Max Action Time reached. Turned off for security reasons");
    // }
}

// MARK: Timer Logic
void TimerLogic() {
    if (setTimer1Active || setTimer2Active) {
        if (config.TIMER.Time_EN) {
            TimerLogicActive = true;
            // Serial.println("Timer on time logic active");
            if (setTimer1Active && !vent1State && config.TIMER.DURATION > 0) {
                vent1EndTime = calculateTime(config.TIMER.TIME, config.TIMER.DURATION);
                if (isTimeSlot(config.TIMER.TIME, vent1EndTime)) {
                    pumpState = true;
                    pumpStartTime = getCurrentTimeInt();
                    vent1State = true;
                    vent1StartTime = getCurrentTimeInt();
                    timer1Active = true;
                    setTimer1Active = false;
                    alreadyPrinted1 = false;
                    Serial.println("Timer valve 1 is active for: " + String(config.TIMER.DURATION) + " min until: " + convertTimeToString(vent1EndTime));
                } else if (!alreadyPrinted1) {
                    Serial.println(
                        "Timer 1 activ at: " + convertTimeToString(config.TIMER.TIME) +
                        " for " + String(config.TIMER.DURATION) +
                        " min until: " + convertTimeToString(vent1EndTime));
                    alreadyPrinted1 = true;
                }

            } else if (setTimer2Active && !setTimer1Active && !vent2State && !vent1State && config.TIMER.DURATION > 0) {
                vent2EndTime = calculateTime(config.TIMER.TIME, config.TIMER.DURATION);
                if (isTimeSlot(config.TIMER.TIME, vent2EndTime)) {
                    pumpState = true;
                    pumpStartTime = getCurrentTimeInt();
                    vent2State = true;
                    vent2StartTime = getCurrentTimeInt();
                    timer2Active = true;
                    setTimer2Active = false;
                    alreadyPrinted2 = false;
                    Serial.println("Timer valve 2 is active for: " + String(config.TIMER.DURATION) + " min until: " + convertTimeToString(vent2EndTime));
                } else if (!alreadyPrinted2) {
                    Serial.println(
                        "Timer 2 activ at: " + convertTimeToString(config.TIMER.TIME) +
                        " for " + String(config.TIMER.DURATION) +
                        " min until: " + convertTimeToString(vent1EndTime));
                    alreadyPrinted2 = true;
                }
            }

            if (config.TIMER.ALL) {
                if (!alreadyPrinted2) {
                    Serial.println(
                        "Timer 2 activ at: " + convertTimeToString(config.TIMER.TIME + config.TIMER.DURATION) +
                        " for " + String(config.TIMER.DURATION) +
                        " min until: " + convertTimeToString(config.TIMER.TIME + (2 * config.TIMER.DURATION)));
                    alreadyPrinted2 = true;
                }
                if (vent1State && (currentTime >= vent1EndTime)) {
                    vent1State = false;
                    timer1Active = false;
                    vent1StartTime = -1;
                    Serial.println("Timer valve 1 ended");
                    if (config.TIMER.DURATION > 0) {
                        vent2EndTime = calculateTime(currentTime, config.TIMER.DURATION);
                        vent2State = true;
                        vent2StartTime = getCurrentTimeInt();
                        timer2Active = true;
                        setTimer2Active = false;
                        alreadyPrinted2 = false;
                        Serial.println("Timer valve 2 is active for: " + String(config.TIMER.DURATION) + " min until: " + convertTimeToString(vent2EndTime));
                    }
                }
            }
        } else {
            TimerLogicActive = true;
            Serial.println("Timer logic active");
            if (setTimer1Active && !vent1State && config.TIMER.DURATION > 0) {
                vent1EndTime = calculateTime(currentTime, config.TIMER.DURATION);
                pumpState = true;
                pumpStartTime = getCurrentTimeInt();
                vent1State = true;
                vent1StartTime = getCurrentTimeInt();
                timer1Active = true;
                setTimer1Active = false;
                Serial.println("Timer valve 1 is active for: " + String(config.TIMER.DURATION) + " min until: " + convertTimeToString(vent1EndTime));
            } else if (setTimer2Active && !setTimer1Active && !vent2State && !vent1State && config.TIMER.DURATION > 0) {
                vent2EndTime = calculateTime(currentTime, config.TIMER.DURATION);
                pumpState = true;
                pumpStartTime = getCurrentTimeInt();
                vent2State = true;
                vent2StartTime = getCurrentTimeInt();
                timer2Active = true;
                setTimer2Active = false;
                Serial.println("Timer valve 2 is active for: " + String(config.TIMER.DURATION) + " min until: " + convertTimeToString(vent2EndTime));
            }

            if (config.TIMER.ALL) {
                if (vent1State && (currentTime >= vent1EndTime)) {
                    timer1Active = false;
                    vent1State = false;
                    vent1StartTime = -1;
                    Serial.println("Timer valve 1 ended");
                    if (config.TIMER.DURATION > 0) {
                        vent2EndTime = calculateTime(currentTime, config.TIMER.DURATION);
                        vent2State = true;
                        vent2StartTime = getCurrentTimeInt();
                        timer2Active = true;
                        setTimer2Active = false;
                        Serial.println("Timer valve 2 is active for: " + String(config.TIMER.DURATION) + " min until: " + convertTimeToString(vent2EndTime));
                    }
                }
            }
        }
    } else if (TimerLogicActive) {

        if (vent1State && (currentTime >= vent1EndTime)) {
            vent1State = false;
            timer1Active = false;
            vent1StartTime = -1;
            pumpState = false;
            pumpStartTime = -1;
            TimerLogicActive = false;
            Serial.println("Timer valve 1 ended");
        } else if (vent2State && (currentTime >= vent2EndTime)) {
            vent2State = false;
            timer2Active = false;
            vent2StartTime = -1;
            pumpState = false;
            pumpStartTime = -1;
            TimerLogicActive = false;
            Serial.println("Timer valve 2 ended");
        }
    }
}

// MARK: Logic
void TimetableLogic() {
    if (!TimerLogicActive) {

        int day = timeinfo.tm_wday;
        DayConfig *dayConfig = NULL;

        switch (day) {
        case 0:
            dayConfig = &config.SUN;
            break;
        case 1:
            dayConfig = &config.MON;
            break;
        case 2:
            dayConfig = &config.TUE;
            break;
        case 3:
            dayConfig = &config.WED;
            break;
        case 4:
            dayConfig = &config.THU;
            break;
        case 5:
            dayConfig = &config.FRI;
            break;
        case 6:
            dayConfig = &config.SAT;
            break;
        default:
            break;
        }

        if (dayConfig->ON) {
            if (config.AUTOMATIC_MODE) {
                if (config.PRECIPITATION_MODE) {
                    if (calculatedRain24h > config.PRECIPITATION_AMOUNT) {
                        Serial.println("Past rain amount over limit, no watering: " + String(calculatedRain24h) + "mm");
                        return;
                    }
                }
                if (config.PRECIPITATION_MODE_FORECAST) {
                    if (rainForecast > config.PRECIPITATION_AMOUNT_FORECAST) {
                        Serial.println("Forecast rain amount over limit, no watering: " + String(rainForecast) + "mm");
                        return;
                    }
                }

                if (config.MENTION_SUNSET) {
                    if (!(dayConfig->TIME.ON_TIME < sunsetTime)) {
                        if (!vent1State && dayConfig->TIME.TIMER_1 > 0) {
                            vent1EndTime = calculateTime(currentTime, dayConfig->TIME.TIMER_1);
                            if (isTimeSlot(currentTime, vent1EndTime)) {
                                pumpState = true;
                                pumpStartTime = getCurrentTimeInt();
                                vent1State = true;
                                vent1StartTime = getCurrentTimeInt();
                                Serial.println("Turned valve 1 on due to sunset");
                            }

                        } else if (!vent2State && !vent1State && dayConfig->TIME.TIMER_2 > 0) {
                            vent2EndTime = calculateTime(currentTime, dayConfig->TIME.TIMER_2);
                            if (isTimeSlot(currentTime, vent2EndTime)) {
                                pumpState = true;
                                pumpStartTime = getCurrentTimeInt();
                                vent2State = true;
                                vent2StartTime = getCurrentTimeInt();
                                Serial.println("Turned valve 2 on due to sunset");
                            }
                        }

                        if (vent1State && (currentTime >= vent1EndTime)) {
                            vent1State = false;
                            vent1StartTime = -1;
                            Serial.println("Turned valve 1 off");
                            if (dayConfig->TIME.TIMER_2 > 0) {
                                vent2EndTime = calculateTime(currentTime, dayConfig->TIME.TIMER_2);
                                vent2State = true;
                                vent2StartTime = getCurrentTimeInt();
                            } else {
                                pumpState = false;
                                pumpStartTime = -1;
                                Serial.println("Turned pump off");
                            }
                        } else if (vent2State && (currentTime >= vent2EndTime)) {
                            vent2State = false;
                            vent2StartTime = -1;
                            pumpState = false;
                            pumpStartTime = -1;
                            Serial.println("Turned valve 2 and pump off");
                        }
                    }
                }

                if (!vent1State && dayConfig->TIME.TIMER_1 > 0) {
                    vent1EndTime = calculateTime(dayConfig->TIME.ON_TIME, dayConfig->TIME.TIMER_1);
                    if (isTimeSlot(dayConfig->TIME.ON_TIME, vent1EndTime)) {
                        pumpState = true;
                        pumpStartTime = getCurrentTimeInt();
                        vent1State = true;
                        vent1StartTime = getCurrentTimeInt();
                        Serial.println("Turned valve 1 on due to timetable");
                    }

                } else if (!vent2State && !vent1State && dayConfig->TIME.TIMER_2 > 0) {
                    vent2EndTime = calculateTime(dayConfig->TIME.ON_TIME, dayConfig->TIME.TIMER_2);
                    if (isTimeSlot(dayConfig->TIME.ON_TIME, vent2EndTime)) {
                        pumpState = true;
                        pumpStartTime = getCurrentTimeInt();
                        vent2State = true;
                        vent2StartTime = getCurrentTimeInt();
                        Serial.println("Turned valve 2 on due to timetable");
                    }
                }

                if (vent1State && (currentTime >= vent1EndTime)) {
                    vent1State = false;
                    vent1StartTime = -1;
                    Serial.println("Turned valve 1 off");
                    if (dayConfig->TIME.TIMER_2 > 0) {
                        vent2EndTime = calculateTime(currentTime, dayConfig->TIME.TIMER_2);
                        vent2State = true;
                        vent2StartTime = getCurrentTimeInt();
                    } else {
                        pumpState = false;
                        pumpStartTime = -1;
                        Serial.println("Turned pump off");
                    }
                } else if (vent2State && (currentTime >= vent2EndTime)) {
                    vent2State = false;
                    vent2StartTime = -1;
                    pumpState = false;
                    pumpStartTime = -1;
                    Serial.println("Turned valve 2 and pump off");
                }
            }
        }
    }
}

// MARK: checkButtons
void checkButtons() {
    // Vent 1 Button
    int readVent1 = digitalRead(vent_1_button);
    if (readVent1 != lastVent1State) {
        lastVent1DebounceTime = millis();
    }
    if ((millis() - lastVent1DebounceTime) > debounceDelay) {
        if (readVent1 != Vent1ButtonState) {
            Vent1ButtonState = readVent1;

            if (Vent1ButtonState == LOW) {
                vent1State = !vent1State;
                Serial.println("Button pressed: " + vent1State ? "Valve 1 is on" : "Vent 1 is off");
            }
        }
    }
    lastVent1State = readVent1;

    // Vent 2 Button
    int readVent2 = digitalRead(vent_2_button);
    if (readVent2 != lastVent2State) {
        lastVent2DebounceTime = millis();
    }
    if ((millis() - lastVent2DebounceTime) > debounceDelay) {
        if (readVent2 != Vent2ButtonState) {
            Vent2ButtonState = readVent2;

            if (Vent2ButtonState == LOW) {
                vent2State = !vent2State;
                Serial.println("Button pressed: " + vent2State ? "Valve 2 is on" : "Vent 2 is off");
            }
        }
    }
    lastVent2State = readVent2;

    // Pump Button
    int readPump = digitalRead(pump_button);
    if (readPump != lastPumpState) {
        lastPumpDebounceTime = millis();
    }
    if ((millis() - lastPumpDebounceTime) > debounceDelay) {
        if (readPump != PumpButtonState) {
            PumpButtonState = readPump;

            if (PumpButtonState == LOW) {
                pumpState = !pumpState;
                Serial.println("Button pressed: " + pumpState ? "Pump is on" : "Pump is off");
            }
        }
    }
    lastPumpState = readPump;

    // Action Button
    int readAction = digitalRead(action_button);
    if (readAction != lastActionState) {
        lastActionDebounceTime = millis();
    }
    if ((millis() - lastActionDebounceTime) > debounceDelay) {
        if (readAction != ActionButtonState) {
            ActionButtonState = readAction;

            if (ActionButtonState == LOW) {
                actionState = !actionState;
                Serial.println("Button pressed: " + actionState ? "Action is on" : "Action is off");
            }
        }
    }
    lastActionState = readAction;
}

// MARK: loop
void loop() {
    ElegantOTA.loop();
    WebSerial.loop();

    updateLocaleTime();
    currentTime = getCurrentTimeInt();
    checkReboot();

    checkButtons();
    securityCheck();
    updateCalculatedRainDataWithDuration();

    TimerLogic();
    TimetableLogic();

    // relay control
    digitalWrite(pump_relay, pumpState);
    digitalWrite(vent_1_relay, vent1State);
    digitalWrite(vent_2_relay, vent2State);
    digitalWrite(action_relay, actionState);

    // led control
    digitalWrite(pump_led, pumpState);
    digitalWrite(vent_1_led, vent1State);
    digitalWrite(vent_2_led, vent2State);
    digitalWrite(action_led, actionState);

    delay(100);
}
