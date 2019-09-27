#pragma once

#include <cstdint>

struct EspClass {
    const char* getSdkVersion() {
        return "2.5.2";
    }

    uint32_t getChipId() {
        return 0x112233;
    }
};

extern EspClass ESP;

