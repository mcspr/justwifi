#pragma once

#include <string>
#include <chrono>
#include <thread>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <vector>

#include "ESP.h"

#define PSTR(X) X
#define strncmp_P strncmp
#define snprintf_P snprintf
#define sprintf_P sprintf

#define DEBUG_ESP_WIFI
#define DEBUG_ESP_PORT Serial

extern void delay(uint32_t ms);
extern uint32_t millis();

class SerialClass {
    public:
        int printf(const char* format, ...) {
            va_list args;
            int res = 0;
            va_start(args, format);
            res = vfprintf(stdout, format, args);
            va_end(args);
            return res;
        }
};

class String {
    public:
        String() {}
        String(const char* str) : _string(str) {}
        String(const String& str) : _string(str._string) {}

        String& operator =(const char* str) { _string = str; }
        String& operator =(const String& str) { _string = str._string; }

        bool operator ==(const String& other) { _string == other._string; }
        bool equals(const String& other) { return _string == other._string; }

        const char* c_str() const {
            return _string.c_str();
        }

    private:
        std::string _string;

};

extern SerialClass Serial;
