#ifndef WEBSERIALUTILS_H
#define WEBSERIALUTILS_H

#include <Arduino.h>
#include <WebSerial.h>

extern WebSerialClass WebSerial;

void println(String message);
void print(String message);
void printFormatted(const char* format, ...);

#endif  // WEBSERIALUTILS_H