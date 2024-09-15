#include "weather.h"

int sunsetTime = -1;
float rain1h = 0.0;
float rain24h = 0.0;
float rainForecast = 0.0;
float calculatedRain = 0.0;

float hourlyRainPast[24] = {0.0}; // save last 24 hours of rain data
int currentHourIndex = 0;

bool alreadyUpdated = false;

// MARK: getWeatherOpenWeather
void getWeatherOpenWeather() {
    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/weather?lat=" + String(config.weather.LATITUDE) + "&lon=" + String(config.weather.LONGITUDE) + "&appid=" + config.weather.API_KEY + "&units=metric";
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == 200) {
        String payload = http.getString();
        println("Recieved weather data OpenWeather");

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
            if (doc["rain"]["1h"]) {
                rain1h = doc["rain"]["1h"].as<float>();
                printFormatted("Rain in last hour OpenWeather: %f mm\n", rain1h);
            } else {
                println("No rain data for the last hour OpenWeather");
                rain1h = 0.0;
            }

            if (doc["sys"]["sunset"]) {
                sunsetTime = convertSunsetTimeTtoInt(doc["sys"]["sunset"]);
                printFormatted("Sunset time: %d\n", sunsetTime);
            } else {
                println("No sunset data OpenWeather");
            }
        } else {
            printFormatted("deserializeJson() failed: %s\n", error.f_str());
            rain1h = 0.0;
        }
        doc.clear();
    } else if (httpCode == 401) {
        println("Error: Invalid API key");
    } else if (httpCode == 403) {
        println("Error: Access denied or request limit reached");
    } else {
        printFormatted("Error on HTTP request: %s\n", httpCode);
    }
    http.end();
}

// MARK: getWeatherMeteo24H
bool getWeatherMeteo24H() {
    if (config.weather.DURATION_PAST == 24 && config.weather.weatherChannel == "meteomatics") {
        getWeatherMeteomatics();
        calculatedRain = rain24h;
        return true;
    }
    getWeatherMeteomatics();
    return false;
}

// MARK: updateCalculatedRainDataWithDuration
bool updateCalculatedRainDataWithDuration() {
    if (checkUpdateRainData()) {
        println("Update calculated rain data");
        if (config.weather.DURATION_PAST <= 0) {
            println("Past duration must be greater than 0.");
            return false;
        }

        if (getWeatherMeteo24H()) {
            printFormatted("Rain last %dh: %f mm\n", config.weather.DURATION_PAST, calculatedRain);
            return true;
        } else {
            getWeatherOpenWeather();
        }

        if (!updateLocaleTime()) {
            return false;
        }

        hourlyRainPast[currentHourIndex] = rain1h;
        currentHourIndex = (currentHourIndex + 1) % config.weather.DURATION_PAST;

        for (int i = 0; i < config.weather.DURATION_PAST; i++) {
            calculatedRain += hourlyRainPast[i];
        }
        printFormatted("Rain last %dh: %f mm\n", config.weather.DURATION_PAST, calculatedRain);
        return true;
    }
    return false;
}

// MARK: checkUpdateRainData
bool checkUpdateRainData() {
    if (updateLocaleTime()) {
        if (timeinfo.tm_min == 0 && alreadyUpdated == false) { // every begin of hour
            alreadyUpdated = true;
            println("Time to update rain data");
            return true;
        } else if (timeinfo.tm_min != 0) {
            alreadyUpdated = false;
        }
        return false;
    }
    return false;
}

// MARK: getCalculatedWeatherUpdateNow
bool getCalculatedWeatherUpdateNow() {
        println("Update calculated rain data");
        if (config.weather.DURATION_PAST <= 0) {
            println("Past duration must be greater than 0.");
            return false;
        }

        if (getWeatherMeteo24H()) {
            printFormatted("Rain last %dh: %f mm\n", config.weather.DURATION_PAST, calculatedRain);
            return true;
        } else {
            getWeatherOpenWeather();
        }

        if (!updateLocaleTime()) {
            return false;
        }

        hourlyRainPast[currentHourIndex] = rain1h;
        currentHourIndex = (currentHourIndex + 1) % config.weather.DURATION_PAST;

        for (int i = 0; i < config.weather.DURATION_PAST; i++) {
            calculatedRain += hourlyRainPast[i];
        }
        printFormatted("Rain last %dh: %f mm\n", config.weather.DURATION_PAST, calculatedRain);
        return true;
}

// MARK: getWeatherUpdateNow
void getWeatherUpdateNow() {
    println("Updating weather data now");
    getWeatherForecastOpenWeather();
    getCalculatedWeatherUpdateNow();
}

// MARK: getWeatherForecastOpenWeather
void getWeatherForecastOpenWeather() {
    if (config.weather.DURATION_FORECAST <= 0) {
        println("Forecast duration must be greater than 0.");
        return;
    }

    int intervals = (config.weather.DURATION_FORECAST + 2) / 3; // calculate 3 hour intervalls

    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/forecast?lat=" + String(config.weather.LATITUDE) + "&lon=" + String(config.weather.LONGITUDE) + "&appid=" + config.weather.API_KEY + "&units=metric";
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == 200) {
        String payload = http.getString();
        println("Recieved forecast openweather data:");

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
            printFormatted("Forecast rain next %dh: %f mm\n", config.weather.DURATION_FORECAST, rainForecast);
        } else {
            printFormatted("deserializeJson() failed: %s\n", error.f_str());
            rainForecast = 0.0;
        }
        doc.clear();
    } else if (httpCode == 401) {
        println("Error: Invalid API key");
    } else if (httpCode == 403) {
        println("Error: Access denied or request limit reached");
    } else {
        printFormatted("Error on HTTP request: %s\n", httpCode);
    }
    http.end();
}

// MARK: getWeatherMeteomatics
void getWeatherMeteomatics() {
    HTTPClient http;
    String url = "https://api.meteomatics.com/now/precip_1h:mm,precip_24h:mm,sunset:ux/" + String(config.weather.LATITUDE) + "," + String(config.weather.LONGITUDE) + "/json?model=mix";
    http.begin(url);
    http.setAuthorization(config.weather.METEO_USERNAME.c_str(), config.weather.METEO_PASSWORD.c_str());

    int httpCode = http.GET();
    if (httpCode == 200) {
        String payload = http.getString();
        println("Recieved weather data meteomatics");

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            if (doc["data"][0]["parameter"] == "precip_1h:mm") {
                rain1h = doc["data"][0]["coordinates"][0]["dates"][0]["value"].as<float>();
                printFormatted("Rain in last hour meteomatics: %f mm\n", rain1h);
            } else {
                println("No rain data for the last hour meteomatics");
                rain1h = 0.0;
            }

            if (doc["data"][1]["parameter"] == "precip_24h:mm") {
                rain24h = doc["data"][1]["coordinates"][0]["dates"][0]["value"].as<float>();
                printFormatted("Rain in last 24 hours meteomatics: %f mm\n", rain24h);
            } else {
                println("No rain data for the last 24 hours meteomatics");
                rain24h = 0.0;
            }

            if (doc["data"][2]["parameter"] == "sunset:ux") {
                sunsetTime = convertSunsetTimeTtoInt(doc["data"][2]["coordinates"][0]["dates"][0]["value"]);
                printFormatted("Sunset time meteomatics: %d\n", sunsetTime);
            } else {
                println("No sunset data meteomatics");
                sunsetTime = -1;
            }

            //for (JsonObject data : doc["data"].as<JsonArray>()) {
            //    String parameter = data["parameter"].as<String>();
            //    if (parameter == "precip_1h:mm") {
            //        rain1h = data["coordinates"][0]["dates"][0]["value"].as<float>();
            //        printFormatted("Rain in last hour meteomatics: %f mm\n", rain1h);
            //    } else {
            //        println("No rain data for the last hour meteomatics");
            //        rain1h = 0.0;
            //    }
            //    if (parameter == "precip_24h:mm") {
            //        rain24h = data["coordinates"][0]["dates"][0]["value"].as<float>();
            //        printFormatted("Rain in last 24 hours meteomatics: %f mm\n", rain24h);
            //    } else {
            //        println("No rain data for the last 24 hours meteomatics");
            //        rain24h = 0.0;
            //    }
            //    if (parameter == "sunset:ux") {
            //        sunsetTime = convertSunsetTimeTtoInt(data["coordinates"][0]["dates"][0]["value"]);
            //        printFormatted("Sunset time meteomatics: %d\n", sunsetTime);
            //    } else {
            //        println("No sunset data meteomatics");
            //        sunsetTime = -1;
            //    }
            //}
        } else {
            printFormatted("deserializeJson() failed: %s\n", error.f_str());
            rain1h = 0.0;
            rain24h = 0.0;
        }
        doc.clear();
    } else if (httpCode == 401) {
        println("Wrong credentials: HTTP 401 Unauthorized.");
    } else {
        printFormatted("Error on HTTP request: %s\n", httpCode);
    }
    http.end();
}

int convertSunsetTimeTtoInt(time_t time) {
    struct tm *tm = localtime(&time);
    int hour = tm->tm_hour;
    int minute = tm->tm_min;
    return hour * 100 + minute;
}