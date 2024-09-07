#include "WebSerialUtils.h"

void println(String message) {
    Serial.println(message);
    WebSerial.println(message);
}

void print(String message) {
    Serial.print(message);
    WebSerial.print(message);
}

void printFormatted(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Serial.print(buffer);
    WebSerial.print(buffer);
}