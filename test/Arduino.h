#pragma once

#include <string>
#include <chrono>
#include <thread>
#include <string.h>

#include "ESP.h"

#define PSTR(X) X
#define strncmp_P strncmp
#define snprintf_P snprintf
#define sprintf_P sprintf

extern void delay(uint32_t ms);
extern uint32_t millis();

class String : public std::string {
    public:
        String() {}
        String(const char* str) : _string(str) {}
        String& operator =(const char* str) { _string = str; }
        String& operator =(const String& str) { _string = str._string; }
        bool equals(const String& other) { return _string.compare(other._string) == 0; }
    private:
        std::string _string;
};


