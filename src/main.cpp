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
//#include <stdarg.h>

#include "configuration.h"
#include "ntp.h"
#include "sensors.h"
#include "weather.h"
#include "WebSerialUtils.h"

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
const int valve_1_button = 36; // the number of the pushbutton pin for valve 1
const int valve_1_led = 27;    // the number of the LED pin for light indicator of valve 1
const int valve_2_button = 39; // the number of the pushbutton pin for valve 2
const int valve_2_led = 32;    // the number of the LED pin for light indicator of valve 2
const int pump_button = 34;   // the number of the pushbutton pin for pump
const int pump_led = 23;      // the number of the LED pin for light indicator of pump
const int action_button = 35; // the number of the pushbutton pin for action
const int action_led = 13;    // the number of the LED pin for light indicator of action
const int valve_1_relay = 19;  // the number of the relay pin for valve 1
const int valve_2_relay = 18;  // the number of the relay pin for valve 2
const int pump_relay = 17;    // the number of the relay pin for pump
const int action_relay = 16;  // the number of the relay pin for action

const int groundHumidityPin = 33;
// const int groundHumidityPinDigital = 25;
const int OneWireBus = 26;

// const int data1 = 14;
// const int data2 = 15;
// const int data3 = 5;

const int DHTPIN = 4; // Digital pin connected to the DHT sensor

//const char *ntpServer1 = "ptbtime1.ptb.de";
const char *ntpServerBackUp = "pool.ntp.org";
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
int valve1EndTime = -1;
int valve2EndTime = -1;
int actionEndTime = -1;

int pumpStartTime = -1;
int valve1StartTime = -1;
int valve2StartTime = -1;
int actionStartTime = -1;

int currentTime = -1;

bool pumpState = false;
bool valve1State = false;
bool valve2State = false;
bool actionState = false;

bool setTimer1Active = false;
bool setTimer2Active = false;

bool timer1Active = false;
bool timer2Active = false;

bool TimerLogicActive = false;

int timerTyp = 0;

// the debounce time to avoid flickers
const unsigned long debounceDelay = 16;

int lastValve1State = LOW;
int lastValve2State = LOW;
int lastPumpState = LOW;
int lastActionState = LOW;
int Valve1ButtonState;
int Valve2ButtonState;
int PumpButtonState;
int ActionButtonState;

unsigned long lastValve1DebounceTime = 0;
unsigned long lastValve2DebounceTime = 0;
unsigned long lastPumpDebounceTime = 0;
unsigned long lastActionDebounceTime = 0;

bool alreadyPrinted1 = false; // for serial output for timer 1 with enable time
bool alreadyPrinted2 = false; // for serial output for timer 2 with enable time

// create objects
AsyncWebServer server(80); // Create AsyncWebServer object on port 80
// AsyncWebSocket ws("/ws");   // Create WebSockets object
DNSServer dns; // Wifi Manager DNS Server for manager website

//MARK: Reboot
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
            println("rebooting due to reboot interval");
            delay(400); // delay to allow serial to print
            reboot();
        }
    }
}

// MARK: Wifi
boolean setNewWiFi(String ssid, String password) {
    WiFi.disconnect();
    WiFi.begin(ssid.c_str(), password.c_str());

    print("New SSID: ");
    println(ssid);
    print("New password: ");
    println(password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        print(".");
    }
    println("\nConnected to new network!");
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
        println("An Error has occurred while mounting LittleFS");
        return;
    }
    println("LittleFS initialized.");

    /*
    if(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
      println("An Error has occurred while mounting LittleFS");
      return;
    */
}

// MARK: initPins
void initPins() {
    pinMode(valve_1_button, INPUT);
    pinMode(valve_2_button, INPUT);
    pinMode(pump_button, INPUT);
    pinMode(action_button, INPUT);

    pinMode(valve_1_led, OUTPUT);
    pinMode(valve_2_led, OUTPUT);
    pinMode(pump_led, OUTPUT);
    pinMode(action_led, OUTPUT);

    pinMode(valve_1_relay, OUTPUT);
    pinMode(valve_2_relay, OUTPUT);
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

// MARK: handle FileUpload
void handleUploadDepricated(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
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
        printFormatted("UploadStart: %s\n", filename.c_str());
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
        printFormatted("UploadEnd: %s, %u B\n", filename.c_str(), index + len);
        if (!loadConfig(configFilePath)) {
            println("Failed to read config file");
            request->send(200, "application/json", "{\"status\":\"false\"}");
            return;
        }
        request->send(200, "application/json", "{\"status\":\"true\"}");
    }
}

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    static String jsonBuffer;  // Temporärer Speicher für den gesamten JSON-Inhalt

    // Überprüfen, ob die Datei eine .json-Datei ist
    if (!filename.endsWith(".json")) {
        request->send(400, "text/plain", "Only .json files are allowed");
        return;
    }

    // Beim ersten Chunk den Puffer initialisieren
    if (index == 0) {
        jsonBuffer = "";  // Puffer leeren
        printFormatted("UploadStart: %s\n", filename.c_str());
    }

    // Daten in den temporären Puffer laden
    jsonBuffer += String((char*)data).substring(0, len);

    // Wenn der Upload abgeschlossen ist
    if (final) {
        printFormatted("UploadEnd: %s, %u B\n", filename.c_str(), index + len);

        // Versuchen, den JSON-Inhalt zu parsen
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, jsonBuffer);

        if (error) {
            printFormatted("JSON Parsing failed: %s\n", error.c_str());
            request->send(400, "text/plain", "Invalid JSON format");
            return;
        }

        // Speichern der Datei im Dateisystem nach erfolgreicher JSON-Verarbeitung
        File file = LittleFS.open(configFilePath, "w");
        if (!file) {
            request->send(500, "text/plain", "Could not open file for writing");
            return;
        }
        file.print(jsonBuffer);  // JSON-Inhalt in die Datei schreiben
        file.close();

        // Nach dem Speichern kann die Konfiguration geladen werden
        if (!loadConfig(configFilePath)) {
            println("Failed to read config file");
            request->send(200, "application/json", "{\"status\":\"false\"}");
            return;
        }

        // Erfolgsmeldung senden
        request->send(200, "application/json", "{\"status\":\"true\"}");
    }
}

// MARK: Emergency off
bool handleEmergency() {
    digitalWrite(valve_1_relay, LOW);
    digitalWrite(valve_2_relay, LOW);
    digitalWrite(pump_relay, LOW);
    digitalWrite(action_relay, LOW);
    digitalWrite(valve_1_led, LOW);
    digitalWrite(valve_2_led, LOW);
    digitalWrite(pump_led, LOW);
    digitalWrite(action_led, LOW);

    valve1State = false;
    valve2State = false;
    pumpState = false;
    actionState = false;

    delay(10);

    bool allLow = (valve1State == false) &&
                  (valve2State == false) &&
                  (pumpState == false) &&
                  (actionState == false) &&
                  (digitalRead(valve_1_relay) == LOW) &&
                  (digitalRead(valve_2_relay) == LOW) &&
                  (digitalRead(pump_relay) == LOW) &&
                  (digitalRead(action_relay) == LOW) &&
                  (digitalRead(valve_1_led) == LOW) &&
                  (digitalRead(valve_2_led) == LOW) &&
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
        println("failed to connect --- hit timeout --- rebooting ESP...");
        delay(3000);
        ESP.restart();
        delay(5000);
    }
    printFormatted("Connected to %s\n", WiFi.SSID().c_str());
    println("");

    initLittleFS();
    initPins();
    setupDHT();

    loadConfig(configFilePath);

    // Print ESP Local IP Address
    println("WiFi connected");
    print("IP address: ");
    println(String(WiFi.localIP()));
    // sets a custom hostname for website
    WiFi.setHostname(config.network.HOSTNAME.c_str());
    print("Hostname: ");
    println(WiFi.getHostname());

    config.network.SSID = WiFi.SSID().c_str();
    saveConfig(configFilePath);

    // Handle the config download button
    // server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request)
    //          { request->send(LittleFS, "/config.json", String(), true); });
    server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request) {
        println("Download config file");
        request->send(LittleFS, configFilePath, String(), true);
    });

    // define Upload-Handler
    server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) { request->send(200); }, handleUpload);

    // define Emergency-Handler
    server.on("/emergency", HTTP_GET, [](AsyncWebServerRequest *request) {
        bool result = handleEmergency();
        println("Emergency shutoff initiated");
        if (result) {
            println("Emergency shutoff successful");
        } else {
            println("Emergency shutoff failed, retrying...");
            handleEmergency();
        }
        request->send(200, "text/plain", "emergency initiated! Success: " + result);
    });

    server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request) {
        println("rebooting initiated");
        request->send(200, "application/json", "{\"status\":\"true\"}");
        reboot();
    });

    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
        bool result = resetConfig();
        println("Reset initiated: Success: " + String(result));
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

            println("Timer data received");

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
        println("Timer data send");
    });

    // Handle stop timers
    server.on("/stop-timer", HTTP_GET, [](AsyncWebServerRequest *request) {
        timer1Active = false;
        timer2Active = false;
        setTimer1Active = false;
        setTimer2Active = false;

        pumpState = false;
        valve1State = false;
        valve2State = false;
        actionState = false;

        request->send(200, "application/json", "{\"status\":\"true\"}");
        println("All timers stopped");
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
            println("Timetable data received"); });

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
        println("Timetable data send");
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
                " min bis: " + convertTimeToString(valve1EndTime);
        } else if (setTimer1Active && config.TIMER.Time_EN && (timerTyp == 1 || timerTyp == 3)) {
            doc["kreis1_timer"] =
                "Aktiv ab: " + convertTimeToString(config.TIMER.TIME) +
                " für " + String(config.TIMER.DURATION) +
                " min bis: " + convertTimeToString(valve1EndTime);
        } else {
            doc["kreis1_timer"] = "Timer nicht aktiv";
        }
        if (timer2Active) {
            doc["kreis2_timer"] =
                "Aktiv: " + String(config.TIMER.DURATION) +
                " min bis: " + convertTimeToString(valve2EndTime);
        } else if (setTimer2Active && config.TIMER.Time_EN && (timerTyp == 2 || timerTyp == 3)) {
            if (timerTyp == 2) {
                doc["kreis2_timer"] =
                    "Aktiv ab: " + convertTimeToString(config.TIMER.TIME) +
                    " für " + String(config.TIMER.DURATION) +
                    " min bis: " + convertTimeToString(valve2EndTime);
            } else {
                doc["kreis2_timer"] =
                    "Aktiv ab: " + convertTimeToString(calculateTime(config.TIMER.TIME, config.TIMER.DURATION)) +
                    " für " + String(config.TIMER.DURATION) +
                    " min bis: " + convertTimeToString(calculateTime(config.TIMER.TIME, (2 * config.TIMER.DURATION)));
            }
        } else {
            doc["kreis2_timer"] = "Timer nicht aktiv";
        }

        doc["valve1_button"] = valve1State;
        doc["valve2_button"] = valve2State;
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
        println("Automatic mode set to: " + String(config.AUTOMATIC_MODE ? "true" : "false"));
        request->send(200, "application/json", "{\"status\":" + String(config.AUTOMATIC_MODE ? "true" : "false") + "}");
    });
    server.on("/activateRain", HTTP_GET, [](AsyncWebServerRequest *request) {
        config.PRECIPITATION_MODE = !config.PRECIPITATION_MODE;
        println("Evaluate rain mode set to: " + String(config.PRECIPITATION_MODE ? "true" : "false"));
        request->send(200, "application/json", "{\"status\":" + String(config.PRECIPITATION_MODE ? "true" : "false") + "}");
    });
    server.on("/activateRainForecast", HTTP_GET, [](AsyncWebServerRequest *request) {
        config.PRECIPITATION_MODE_FORECAST = !config.PRECIPITATION_MODE_FORECAST;
        println("Evaluate rain forecast mode set to: " + String(config.PRECIPITATION_MODE_FORECAST ? "true" : "false"));
        request->send(200, "application/json", "{\"status\":" + String(config.PRECIPITATION_MODE_FORECAST ? "true" : "false") + "}");
    });
    server.on("/mentionSunset", HTTP_GET, [](AsyncWebServerRequest *request) {
        config.MENTION_SUNSET = !config.MENTION_SUNSET;
        println("Mention sunset mode set to: " + String(config.MENTION_SUNSET ? "true" : "false"));
        request->send(200, "application/json", "{\"status\":" + String(config.MENTION_SUNSET ? "true" : "false") + "}");
    });

    server.on("/togglePump", HTTP_GET, [](AsyncWebServerRequest *request) {
        pumpState = !pumpState;
        pumpStartTime = pumpState ? getCurrentTimeInt() : -1;
        println("Pump set to: " + String(pumpState ? "active" : "off"));
        request->send(200, "application/json", "{\"status\":" + String(pumpState ? "true" : "false") + "}");
    });
    server.on("/toggleValve1", HTTP_GET, [](AsyncWebServerRequest *request) {
        valve1State = !valve1State;
        valve1StartTime = valve1State ? getCurrentTimeInt() : -1;
        println("Valve1 set to: " + String(valve1State ? "active" : "off"));
        request->send(200, "application/json", "{\"status\":" + String(valve1State ? "true" : "false") + "}");
    });
    server.on("/toggleValve2", HTTP_GET, [](AsyncWebServerRequest *request) {
        valve2State = !valve2State;
        valve2StartTime = valve2State ? getCurrentTimeInt() : -1;
        println("Valve2 set to: " + String(valve2State ? "active" : "off"));
        request->send(200, "application/json", "{\"status\":" + String(valve2State ? "true" : "false") + "}");
    });

    // settings endpoints
    // MARK: settings
    server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;

        doc["ntp"] = config.network.NTP_SERVER;
        doc["ssid"] = config.network.SSID;
        doc["hostname"] = config.network.HOSTNAME;
        doc["precipitation_level"] = config.PRECIPITATION_AMOUNT;
        doc["precipitation_level_forecast"] = config.PRECIPITATION_AMOUNT_FORECAST;
        doc["max_pump_time"] = config.MAX_PUMP_TIME;
        doc["max_valve1_time"] = config.MAX_VALVE1_TIME;
        doc["max_valve2_time"] = config.MAX_VALVE2_TIME;
        doc["weather_channel"] = config.weather.weatherChannel;
        doc["longitude"] = config.weather.LONGITUDE;
        doc["latitude"] = config.weather.LATITUDE;
        doc["rain_duration_past"] = config.weather.DURATION_PAST;
        doc["rain_duration_forecast"] = config.weather.DURATION_FORECAST;
        doc["api_key"] = config.weather.API_KEY;

        if (config.weather.weatherChannel == "metehomatics") {
            doc["metehomatics_user"] = config.weather.METHEO_USERNAME;
            doc["metehomatics_password"] = config.weather.METHEO_PASSWORD;
        }

        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
        println("Settings data send");
    });

    server.on("/settings", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, data);
        if (error) {
            request->send(400, "application/json",
                          "{\"status\":\"error\", \"message\":\"Invalid JSON\"}");
            return;
        }
        println("Settings data received");

        if (doc.containsKey("zeitserver")) {
            config.network.NTP_SERVER = doc["zeitserver"].as<String>();
            while (!syncTimeWithNTP()) {
                println("Failed to sync time with NTP server. Trying again in a second...");
                delay(1000);
            }
            println("NTP server set to: " + config.network.NTP_SERVER);
        }
        if (doc.containsKey("wifi_ssid")) {
            config.network.SSID = doc["wifi_ssid"].as<String>();
        }
        if (doc.containsKey("wifi_pw")) {
            config.network.PASSWORD = doc["wifi_pw"].as<String>();
            println("New wifi credentials set");
        }
        if (doc.containsKey("hostname")) {
            config.network.HOSTNAME = doc["hostname"].as<String>();
            WiFi.setHostname(config.network.HOSTNAME.c_str());
            println("Hostname set to: " + config.network.HOSTNAME);
        }
        if (doc.containsKey("precipitation_level")) {
            config.PRECIPITATION_AMOUNT = doc["precipitation_level"];
            println("Precipitation level set to: " + String(config.PRECIPITATION_AMOUNT));
        }
        if (doc.containsKey("precipitation_level_forecast")) {
            config.PRECIPITATION_AMOUNT_FORECAST = doc["precipitation_level_forecast"];
            println("Precipitation level forecast set to: " + String(config.PRECIPITATION_AMOUNT_FORECAST));
        }
        if (doc.containsKey("weather_channel")) {
            config.weather.weatherChannel = doc["weather_channel"].as<String>();
            println("Weather channel set to: " + config.weather.weatherChannel);
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
        if (doc.containsKey("api_key")) {
            config.weather.API_KEY = doc["api_key"].as<String>();
            println("Weather settings set");
        }
        if (config.weather.weatherChannel == "metehomatics") {
            if (doc.containsKey("metheo_user")) {
                config.weather.METHEO_USERNAME = doc["metheo_user"].as<String>();
            }
            if (doc.containsKey("metheo_pw")) {
                config.weather.METHEO_PASSWORD = doc["metheo_pw"].as<String>();
            }
        }
        if (doc.containsKey("max_pump_time")) {
            config.MAX_PUMP_TIME = doc["max_pump_time"];
        }
        if (doc.containsKey("max_valve1_time")) {
            config.MAX_VALVE1_TIME = doc["max_valve1_time"];
        }
        if (doc.containsKey("max_valve2_time")) {
            config.MAX_VALVE2_TIME = doc["max_valve2_time"];
            println("Max times set");
        }
        saveConfig(configFilePath);
        request->send(200, "application/json", "{\"status\":\"true\"}");
     });

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
        println("Chip info send");
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

    //MARK: WebSerial
    // Attach a callback function to handle incoming messages
    WebSerial.onMessage([](uint8_t *data, size_t len) {
        Serial.printf("Received %lu bytes from WebSerial: ", len);
        Serial.write(data, len);
        Serial.println();
        WebSerial.println("Received Data...");
        String d = "";
        for(size_t i = 0; i < len; i++){
          d += char(data[i]);
        }
        WebSerial.println(d);
      });

    server.onNotFound(notFound);
    server.begin();

    // Setup rtc time
    while (!syncTimeWithNTP()) {
        println("Failed to sync time with NTP server. Trying again in a second...");
        delay(1000);
    }
}

// MARK: Security Check
void securityCheck() {
    if (valve1State && calculateSecurityCheck(valve1StartTime, config.MAX_VALVE1_TIME)) {
        valve1State = false;
        valve1StartTime = -1;
        println("Max Valve1 Time reached. Turned off for security reasons");
    }
    if (valve2State && calculateSecurityCheck(valve2StartTime, config.MAX_VALVE2_TIME)) {
        valve2State = false;
        valve2StartTime = -1;
        println("Max Valve2 Time reached. Turned off for security reasons");
    }
    if (pumpState && calculateSecurityCheck(pumpStartTime, config.MAX_PUMP_TIME)) {
        pumpState = false;
        pumpStartTime = -1;
        println("Max Pump Time reached. Turned off for security reasons");
    }
    // if (actionState && calculateSecurityCheck(actionStartTime, config.MAX_ACTION_TIME)) {
    //     actionState = false;
    //     actionStartTime = -1;
    //      println("Max Action Time reached. Turned off for security reasons");
    // }
}

// MARK: Timer Logic
void TimerLogic() {
    if (setTimer1Active || setTimer2Active) {
        if (config.TIMER.Time_EN) {
            TimerLogicActive = true;
            // println("Timer on time logic active");
            if (setTimer1Active && !valve1State && config.TIMER.DURATION > 0) {
                valve1EndTime = calculateTime(config.TIMER.TIME, config.TIMER.DURATION);
                if (isTimeSlot(config.TIMER.TIME, valve1EndTime)) {
                    pumpState = true;
                    pumpStartTime = getCurrentTimeInt();
                    valve1State = true;
                    valve1StartTime = getCurrentTimeInt();
                    timer1Active = true;
                    setTimer1Active = false;
                    alreadyPrinted1 = false;
                    println("Timer valve 1 is active for: " + String(config.TIMER.DURATION) + " min until: " + convertTimeToString(valve1EndTime));
                } else if (!alreadyPrinted1) {
                    println(
                        "Timer 1 activ at: " + convertTimeToString(config.TIMER.TIME) +
                        " for " + String(config.TIMER.DURATION) +
                        " min until: " + convertTimeToString(valve1EndTime));
                    alreadyPrinted1 = true;
                }

            } else if (setTimer2Active && !setTimer1Active && !valve2State && !valve1State && config.TIMER.DURATION > 0) {
                valve2EndTime = calculateTime(config.TIMER.TIME, config.TIMER.DURATION);
                if (isTimeSlot(config.TIMER.TIME, valve2EndTime)) {
                    pumpState = true;
                    pumpStartTime = getCurrentTimeInt();
                    valve2State = true;
                    valve2StartTime = getCurrentTimeInt();
                    timer2Active = true;
                    setTimer2Active = false;
                    alreadyPrinted2 = false;
                    println("Timer valve 2 is active for: " + String(config.TIMER.DURATION) + " min until: " + convertTimeToString(valve2EndTime));
                } else if (!alreadyPrinted2) {
                    println(
                        "Timer 2 activ at: " + convertTimeToString(config.TIMER.TIME) +
                        " for " + String(config.TIMER.DURATION) +
                        " min until: " + convertTimeToString(valve1EndTime));
                    alreadyPrinted2 = true;
                }
            }

            if (config.TIMER.ALL) {
                if (!alreadyPrinted2) {
                    println(
                        "Timer 2 activ at: " + convertTimeToString(config.TIMER.TIME + config.TIMER.DURATION) +
                        " for " + String(config.TIMER.DURATION) +
                        " min until: " + convertTimeToString(config.TIMER.TIME + (2 * config.TIMER.DURATION)));
                    alreadyPrinted2 = true;
                }
                if (valve1State && (currentTime >= valve1EndTime)) {
                    valve1State = false;
                    timer1Active = false;
                    valve1StartTime = -1;
                    println("Timer valve 1 ended");
                    if (config.TIMER.DURATION > 0) {
                        valve2EndTime = calculateTime(currentTime, config.TIMER.DURATION);
                        valve2State = true;
                        valve2StartTime = getCurrentTimeInt();
                        timer2Active = true;
                        setTimer2Active = false;
                        alreadyPrinted2 = false;
                        println("Timer valve 2 is active for: " + String(config.TIMER.DURATION) + " min until: " + convertTimeToString(valve2EndTime));
                    }
                }
            }
        } else {
            TimerLogicActive = true;
            println("Timer logic active");
            if (setTimer1Active && !valve1State && config.TIMER.DURATION > 0) {
                valve1EndTime = calculateTime(currentTime, config.TIMER.DURATION);
                pumpState = true;
                pumpStartTime = getCurrentTimeInt();
                valve1State = true;
                valve1StartTime = getCurrentTimeInt();
                timer1Active = true;
                setTimer1Active = false;
                println("Timer valve 1 is active for: " + String(config.TIMER.DURATION) + " min until: " + convertTimeToString(valve1EndTime));
            } else if (setTimer2Active && !setTimer1Active && !valve2State && !valve1State && config.TIMER.DURATION > 0) {
                valve2EndTime = calculateTime(currentTime, config.TIMER.DURATION);
                pumpState = true;
                pumpStartTime = getCurrentTimeInt();
                valve2State = true;
                valve2StartTime = getCurrentTimeInt();
                timer2Active = true;
                setTimer2Active = false;
                println("Timer valve 2 is active for: " + String(config.TIMER.DURATION) + " min until: " + convertTimeToString(valve2EndTime));
            }

            if (config.TIMER.ALL) {
                if (valve1State && (currentTime >= valve1EndTime)) {
                    timer1Active = false;
                    valve1State = false;
                    valve1StartTime = -1;
                    println("Timer valve 1 ended");
                    if (config.TIMER.DURATION > 0) {
                        valve2EndTime = calculateTime(currentTime, config.TIMER.DURATION);
                        valve2State = true;
                        valve2StartTime = getCurrentTimeInt();
                        timer2Active = true;
                        setTimer2Active = false;
                        println("Timer valve 2 is active for: " + String(config.TIMER.DURATION) + " min until: " + convertTimeToString(valve2EndTime));
                    }
                }
            }
        }
    } else if (TimerLogicActive) {

        if (valve1State && (currentTime >= valve1EndTime)) {
            valve1State = false;
            timer1Active = false;
            valve1StartTime = -1;
            pumpState = false;
            pumpStartTime = -1;
            TimerLogicActive = false;
            println("Timer valve 1 ended");
        } else if (valve2State && (currentTime >= valve2EndTime)) {
            valve2State = false;
            timer2Active = false;
            valve2StartTime = -1;
            pumpState = false;
            pumpStartTime = -1;
            TimerLogicActive = false;
            println("Timer valve 2 ended");
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
                        println("Past rain amount over limit, no watering: " + String(calculatedRain24h) + "mm");
                        return;
                    }
                }
                if (config.PRECIPITATION_MODE_FORECAST) {
                    if (rainForecast > config.PRECIPITATION_AMOUNT_FORECAST) {
                        println("Forecast rain amount over limit, no watering: " + String(rainForecast) + "mm");
                        return;
                    }
                }

                if (config.MENTION_SUNSET) {
                    if (!(dayConfig->TIME.ON_TIME < sunsetTime)) {
                        if (!valve1State && dayConfig->TIME.TIMER_1 > 0) {
                            valve1EndTime = calculateTime(currentTime, dayConfig->TIME.TIMER_1);
                            if (isTimeSlot(currentTime, valve1EndTime)) {
                                pumpState = true;
                                pumpStartTime = getCurrentTimeInt();
                                valve1State = true;
                                valve1StartTime = getCurrentTimeInt();
                                println("Turned valve 1 on due to sunset");
                            }

                        } else if (!valve2State && !valve1State && dayConfig->TIME.TIMER_2 > 0) {
                            valve2EndTime = calculateTime(currentTime, dayConfig->TIME.TIMER_2);
                            if (isTimeSlot(currentTime, valve2EndTime)) {
                                pumpState = true;
                                pumpStartTime = getCurrentTimeInt();
                                valve2State = true;
                                valve2StartTime = getCurrentTimeInt();
                                println("Turned valve 2 on due to sunset");
                            }
                        }

                        if (valve1State && (currentTime >= valve1EndTime)) {
                            valve1State = false;
                            valve1StartTime = -1;
                            println("Turned valve 1 off");
                            if (dayConfig->TIME.TIMER_2 > 0) {
                                valve2EndTime = calculateTime(currentTime, dayConfig->TIME.TIMER_2);
                                valve2State = true;
                                valve2StartTime = getCurrentTimeInt();
                            } else {
                                pumpState = false;
                                pumpStartTime = -1;
                                println("Turned pump off");
                            }
                        } else if (valve2State && (currentTime >= valve2EndTime)) {
                            valve2State = false;
                            valve2StartTime = -1;
                            pumpState = false;
                            pumpStartTime = -1;
                            println("Turned valve 2 and pump off");
                        }
                    }
                }

                if (!valve1State && dayConfig->TIME.TIMER_1 > 0) {
                    valve1EndTime = calculateTime(dayConfig->TIME.ON_TIME, dayConfig->TIME.TIMER_1);
                    if (isTimeSlot(dayConfig->TIME.ON_TIME, valve1EndTime)) {
                        pumpState = true;
                        pumpStartTime = getCurrentTimeInt();
                        valve1State = true;
                        valve1StartTime = getCurrentTimeInt();
                        println("Turned valve 1 on due to timetable");
                    }

                } else if (!valve2State && !valve1State && dayConfig->TIME.TIMER_2 > 0) {
                    valve2EndTime = calculateTime(dayConfig->TIME.ON_TIME, dayConfig->TIME.TIMER_2);
                    if (isTimeSlot(dayConfig->TIME.ON_TIME, valve2EndTime)) {
                        pumpState = true;
                        pumpStartTime = getCurrentTimeInt();
                        valve2State = true;
                        valve2StartTime = getCurrentTimeInt();
                        println("Turned valve 2 on due to timetable");
                    }
                }

                if (valve1State && (currentTime >= valve1EndTime)) {
                    valve1State = false;
                    valve1StartTime = -1;
                    println("Turned valve 1 off");
                    if (dayConfig->TIME.TIMER_2 > 0) {
                        valve2EndTime = calculateTime(currentTime, dayConfig->TIME.TIMER_2);
                        valve2State = true;
                        valve2StartTime = getCurrentTimeInt();
                    } else {
                        pumpState = false;
                        pumpStartTime = -1;
                        println("Turned pump off");
                    }
                } else if (valve2State && (currentTime >= valve2EndTime)) {
                    valve2State = false;
                    valve2StartTime = -1;
                    pumpState = false;
                    pumpStartTime = -1;
                    println("Turned valve 2 and pump off");
                }
            }
        }
    }
}

// MARK: checkButtons
void checkButtons() {
    // Valve 1 Button
    int readValve1 = digitalRead(valve_1_button);
    if (readValve1 != lastValve1State) {
        lastValve1DebounceTime = millis();
    }
    if ((millis() - lastValve1DebounceTime) > debounceDelay) {
        if (readValve1 != Valve1ButtonState) {
            Valve1ButtonState = readValve1;

            if (Valve1ButtonState == LOW) {
                valve1State = !valve1State;
                println("Button pressed: " + String(valve1State ? "Valve 1 is on" : "Valve 1 is off"));
            }
        }
    }
    lastValve1State = readValve1;

    // Valve 2 Button
    int readValve2 = digitalRead(valve_2_button);
    if (readValve2 != lastValve2State) {
        lastValve2DebounceTime = millis();
    }
    if ((millis() - lastValve2DebounceTime) > debounceDelay) {
        if (readValve2 != Valve2ButtonState) {
            Valve2ButtonState = readValve2;

            if (Valve2ButtonState == LOW) {
                valve2State = !valve2State;
                println("Button pressed: " + String(valve2State ? "Valve 2 is on" : "Valve 2 is off"));
            }
        }
    }
    lastValve2State = readValve2;

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
                println("Button pressed: " + String(pumpState ? "Pump is on" : "Pump is off"));
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
                println("Button pressed: " + String(actionState ? "Action is on" : "Action is off"));
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
    digitalWrite(valve_1_relay, valve1State);
    digitalWrite(valve_2_relay, valve2State);
    digitalWrite(action_relay, actionState);

    // led control
    digitalWrite(pump_led, pumpState);
    digitalWrite(valve_1_led, valve1State);
    digitalWrite(valve_2_led, valve2State);
    digitalWrite(action_led, actionState);

    delay(80);
}
