#include "weather.h"

int sunsetTime = -1;
float rain1h = 0.0;
float rain24h = 0.0;
float rainForecast = 0.0;
float calculatedRain24h = 0.0;

float hourlyRainPast[24] = {0.0}; // save last 24 hours of rain data
int currentHourIndex = 0;

bool alreadyUpdated = false;

void getWeatherOpenWeather() {
    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/weather?lat=" + String(config.weather.LATITUDE) + "&lon=" + String(config.weather.LONGITUDE) + "&appid=" + config.weather.API_KEY + "&units=metric";
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode > 0) {
        String payload = http.getString();
        Serial.println("Recieved weather data OpenWeather: \n" + payload);

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
            if (doc["rain"]["1h"]) {
                rain1h = doc["rain"]["1h"].as<float>();
                Serial.printf("Rain in last hour OpenWeather: %f mm\n", rain1h);
            } else {
                Serial.println("No rain data for the last hour OpenWeather");
                rain1h = 0.0;
            }

            if (doc["sys"]["sunset"]) {
                sunsetTime = convertSunsetTimeTtoInt(doc["sys"]["sunset"]);
                Serial.printf("Sunset time: %d\n", sunsetTime);
            } else {
                Serial.println("No sunset data OpenWeather");
            }
        } else {
            Serial.printf("deserializeJson() failed: %s", error.f_str());
        }
    } else {
        Serial.printf("Error on HTTP request: %s", httpCode);
    }
    http.end();
}

bool updateCalculatedRainDataWithDuration() {
    if (config.weather.DURATION_PAST == 24 && config.weather.weatherChannel == "meteomatics") {
        getWeatherMeteomatics();
        calculatedRain24h = rain24h;
        return true;
    }
    if (checkUpdateRainData()) {
        Serial.println("Update calculated rain data");
        if (config.weather.DURATION_PAST <= 0) {
            Serial.println("Past duration must be greater than 0.");
            return false;
        }

        if (config.weather.weatherChannel == "openweather") {
            getWeatherOpenWeather();
        } else {
            getWeatherMeteomatics();
        }

        if (!updateLocaleTime()) {
            return false;
        }

        hourlyRainPast[currentHourIndex] = rain1h;
        currentHourIndex = (currentHourIndex + 1) % config.weather.DURATION_PAST;

        for (int i = 0; i < config.weather.DURATION_PAST; i++) {
            calculatedRain24h += hourlyRainPast[i];
        }
        Serial.printf("Rain last %dh: %f mm\n", config.weather.DURATION_PAST, calculatedRain24h);
        return true;
    }
    return false;
}

bool checkUpdateRainData() {
    if (updateLocaleTime()) {
        if (timeinfo.tm_min == 0 && alreadyUpdated == false) { // every begin of hour
            alreadyUpdated = true;
            Serial.println("Time to update rain data");
            return true;
        } else if (timeinfo.tm_min != 0) {
            alreadyUpdated = false;
        }
        return false;
    }
    return false;
}

void getWeatherForecastOpenWeather() {
    if (config.weather.DURATION_FORECAST <= 0) {
        Serial.println("Forecast duration must be greater than 0.");
        return;
    }

    int intervals = (config.weather.DURATION_FORECAST + 2) / 3; // calculate 3 hour intervalls

    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/forecast?lat=" + String(config.weather.LATITUDE) + "&lon=" + String(config.weather.LONGITUDE) + "&appid=" + config.weather.API_KEY + "&units=metric";
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode > 0) {
        String payload = http.getString();
        Serial.println("Recieved forecast weather data: \n" + payload);

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            for (int i = 0; i < intervals; i++) {
                JsonObject forecast = doc["list"][i];
                if (forecast.containsKey("rain")) {
                    JsonObject rain = forecast["rain"];
                    if (rain.containsKey("3h")) {
                        rainForecast += rain["3h"].as<float>();
                    }
                }
            }
        } else {
            Serial.printf("deserializeJson() failed: %s", error.f_str());
        }
    } else {
        Serial.printf("Error on HTTP request: %s", httpCode);
    }
    http.end();
}

void getWeatherMeteomatics() {
    HTTPClient http;
    String datetime = "now";
    String url = "https://api.meteomatics.com/" + datetime + "/precip_1h:mm,precip_24h:mm,sunset:ux/" + String(config.weather.LATITUDE) + "," + String(config.weather.LONGITUDE) + "/json?model=mix";
    http.begin(url);
    http.setAuthorization(config.weather.METHEO_USERNAME.c_str(), config.weather.METHEO_PASSWORD.c_str());

    int httpCode = http.GET();
    if (httpCode > 0) {
        String payload = http.getString();
        Serial.println("Recieved weather data meteomatics: \n" + payload);

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            for (JsonObject data : doc["data"].as<JsonArray>()) {
                String parameter = data["parameter"].as<String>();
                if (parameter == "precip_1h:mm") {
                    rain1h = data["coordinates"][0]["dates"][0]["value"].as<float>();
                    Serial.printf("Rain in last hour meteomatics: %s mm", rain1h);
                } else if (parameter == "precip_24h:mm") {
                    rain24h = data["coordinates"][0]["dates"][0]["value"].as<float>();
                    Serial.printf("Rain in last 24 hours meteomatics: %s mm", rain24h);
                } else if (parameter == "sunset:ux") {
                    sunsetTime = convertSunsetTimeTtoInt(data["coordinates"][0]["dates"][0]["value"]);
                    Serial.printf("Sunset time meteomatics: %d", sunsetTime);
                }
            }
        } else {
            Serial.printf("deserializeJson() failed: %s", error.f_str());
        }

    } else {
        Serial.printf("Error on HTTP request: %s", httpCode);
    }
    http.end();
}

int convertSunsetTimeTtoInt(time_t time) {
    struct tm *tm = localtime(&time);
    int hour = tm->tm_hour;
    int minute = tm->tm_min;
    return hour * 100 + minute;
}