#ifndef NTP_H
#define NTP_H

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <time.h>
#include "configuration.h"

extern struct tm timeinfo;

//extern const char *ntpServer1;
extern const char *ntpServerBackUp;
extern const char *Timezone;

bool syncTimeWithNTP();
bool updateLocaleTime();
bool isTimeToSync(int hour, int minute);
int getCurrentTimeInt();
bool isTimeSlot(int startTime, int endTime);
int getHour();
int getMinute();
int getDayInt();
String getDayNameShort();
String asctime();
String strftime();

#endif  // NTP_H