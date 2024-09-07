#include "ntp.h"

struct tm timeinfo;

//const char *ntpServer1 = "ptbtime1.ptb.de";
//const char *ntpServer2 = "pool.ntp.org";
//const char *Timezone = "CET-1CEST,M3.5.0/2,M10.5.0/3";
// Example time zones
// const char* Timezone = "GMT0BST,M3.5.0/01,M10.5.0/02";     // UK
// const char* Timezone = "MET-2METDST,M3.5.0/01,M10.5.0/02"; // Most of Europe
// const char* Timezone = "CET-1CEST,M3.5.0/2,M10.5.0/3";       // Central Europe
// const char* Timezone = "EST-2METDST,M3.5.0/01,M10.5.0/02";
// const char* Timezone = "EST5EDT,M3.2.0,M11.1.0"; // EST
// const char* Timezone = "CST6CDT,M3.2.0,M11.1.0";           // CST USA
// const char* Timezone = "MST7MDT,M4.1.0,M10.5.0";           // MST USA
// const char* Timezone = "NZST-12NZDT,M9.5.0,M4.1.0/3";      // Auckland
// const char* Timezone = "EET-2EEST,M3.5.5/0,M10.5.5/0";     // Asia
// const char* Timezone = "ACST-9:30ACDT,M10.1.0,M4.1.0/3":   // Australia

int lastSyncDay = -1;

bool syncTimeWithNTP() {
  // Set Timezones and config
  configTime(0, 0, config.network.NTP_SERVER.c_str(), ntpServerBackUp);
  setenv("TZ", Timezone, 1);
  tzset();

  // synchronize time
  if (getLocalTime(&timeinfo)) {
    Serial.println("Time successfully synchronized");
    lastSyncDay = timeinfo.tm_wday; // last day of sync
    return true;
  } else {
    Serial.println("Failed to obtain time");
    return false;
  }
}

bool updateLocaleTime() {
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return false;
  }
  return true;
}

bool isTimeToSync(int hour, int minute) {
  if (lastSyncDay == -1) {
    syncTimeWithNTP();
    return true;
  }

  if (getLocalTime(&timeinfo)) {
    if (lastSyncDay != timeinfo.tm_wday && timeinfo.tm_hour == hour && timeinfo.tm_min == minute) {
      syncTimeWithNTP();
      return true;
    }
  }
  return false;
}

int getCurrentTimeInt() {
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return -1;
  }
  return timeinfo.tm_hour * 100 + timeinfo.tm_min;
}

bool isTimeSlot(int startTime, int endTime) {
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return false;
  }
  int currentTime = timeinfo.tm_hour * 100 + timeinfo.tm_min;

  if (startTime > endTime) {
    return currentTime >= startTime || currentTime <= endTime;
  }  
  return currentTime >= startTime && currentTime <= endTime;
}

int getHour() {
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return -1;
  }
  return timeinfo.tm_hour;
}

int getMinute() {
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return -1;
  }
  return timeinfo.tm_min;
}

int getDayInt() {
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return -1;
  }
  return timeinfo.tm_wday;
}

String getDayNameShort() {
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return "";
  }
  char buffer[3];
  strftime(buffer, sizeof(buffer), "%a", &timeinfo);
  return buffer;
}

String asctime() {           // https://cplusplus.com/reference/ctime/asctime/
  return asctime(&timeinfo); // Www Mmm dd hh:mm:ss yyyy
}

String strftime() { // https://cplusplus.com/reference/ctime/strftime/
  char buffer[80];
  strftime(buffer, sizeof(buffer), "%d.%m.%Y %H:%M:%S",
           &timeinfo); // dd.mm.jjjj hh:mm:ss
  return buffer;
}
