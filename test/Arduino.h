#pragma once

#include <string>
#include <chrono>
#include <thread>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <vector>

#include "ESP.h"
#include "pgmspace.h"

#define DEBUG_ESP_WIFI
#define DEBUG_ESP_PORT Serial

extern void delay(uint32_t ms);
extern uint32_t millis();

class SerialClass {
    public:
        void println(const char* string) {
            fprintf(stdout, "%s\n", string);
        }
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

        String(const String&) = default;
        String(String&&) = default;

        String(const __FlashStringHelper* str) : _string(reinterpret_cast<const char*>(str)) {}
        String(const char* str) : _string(str) {}

        String& operator =(const char* str) { _string = str; }
        String& operator =(const String& str) { _string = str._string; }

        bool operator ==(const String& other) { _string == other._string; }
        bool equals(const String& other) { return _string == other._string; }

        const char* c_str() const {
            return _string.c_str();
        }

        const size_t length() const {
            return _string.length();
        }

    private:
        std::string _string;

};

extern SerialClass Serial;
