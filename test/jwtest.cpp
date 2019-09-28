#include <Arduino.h>
#include <JustWifi.h>
#include <iostream>

const char *justwifi_messages_strings[] = {
    "MESSAGE_SCANNING",
    "MESSAGE_SCAN_FAILED",
    "MESSAGE_NO_NETWORKS",
    "MESSAGE_FOUND_NETWORK",
    "MESSAGE_NO_KNOWN_NETWORKS",
    "MESSAGE_CONNECTING",
    "MESSAGE_CONNECT_WAITING",
    "MESSAGE_CONNECT_FAILED",
    "MESSAGE_CONNECTED",
    "MESSAGE_ACCESSPOINT_CREATING",
    "MESSAGE_ACCESSPOINT_CREATED",
    "MESSAGE_ACCESSPOINT_FAILED",
    "MESSAGE_ACCESSPOINT_DESTROYED",
    "MESSAGE_DISCONNECTED",
    "MESSAGE_HOSTNAME_ERROR",
    "MESSAGE_TURNING_OFF",
    "MESSAGE_TURNING_ON",
    "MESSAGE_WPS_START",
    "MESSAGE_WPS_SUCCESS",
    "MESSAGE_WPS_ERROR",
    "MESSAGE_SMARTCONFIG_START",
    "MESSAGE_SMARTCONFIG_SUCCESS",
    "MESSAGE_SMARTCONFIG_ERROR"
};

bool connected_flag = false;

void connect() {
    static uint32_t last = millis();
    if (millis() - last > 5000) {
        last = millis();
        if (!connected_flag) {
            connected_flag = true;
            WiFi._status = WL_CONNECTED;
        }
    }
}

int main(int argc, char** argv) {

    WiFi.addNetworkInfo("TEST", ENC_TYPE_CCMP, -50, {0x11, 0x12, 0x13, 0x14, 0x15, 0x20}, 6, false);
    WiFi.addNetworkInfo("TEST", ENC_TYPE_CCMP, -75, {0x11, 0x12, 0x13, 0x14, 0x15, 0x40}, 6, false);

    jw.subscribe([](justwifi_messages_t message, char* data) {
        if (message == MESSAGE_CONNECT_WAITING) return;
        std::cout 
            << "[" << millis() << "] "
            << "jw msg=" << justwifi_messages_strings[message]
            << " data=" << (data ? data : "NULL")
            << std::endl;
    });
    jw.enableScan(true);
    jw.addNetwork("TEST", "testtesttest");
    do {
        jw.loop();
        connect();
    } while (true);

}
