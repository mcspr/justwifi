#include <Arduino.h>
#include <JustWifi.h>

#include <iostream>
#include <future>
#include <thread>
#include <chrono>
#include <mutex>

const char *justwifi_messages_strings[] = {
    "MESSAGE_SCANNING",
    "MESSAGE_SCAN_FAILED",
    "MESSAGE_NO_NETWORKS",
    "MESSAGE_FOUND_NETWORK",
    "MESSAGE_FOUND_BETTER_NETWORK",
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

struct Timeout {
    Timeout(uint32_t timeout) :
        _flag(false),
        _last(millis()),
        _timeout(timeout)
    {}
    operator bool() {
        if (_flag) return false;
        const auto result = (millis() - _last > _timeout);
        if (result) _flag = true;
        return result;
    }
    bool _flag;
    uint32_t _last;
    uint32_t _timeout;
};

int main(int argc, char** argv) {

    WiFi.addNetworkInfo("TEST", ENC_TYPE_CCMP, -75, {0x11, 0x12, 0x13, 0x14, 0x15, 0x40}, 6, false);

    jw.subscribe([](justwifi_messages_t message, char* data) {
        if (message == MESSAGE_CONNECT_WAITING) return;
        std::cout 
            << "[" << millis() << "] "
            << "jw msg=" << justwifi_messages_strings[message]
            << " data=" << (data ? data : "NULL")
            << std::endl;
    });
    std::this_thread::sleep_for(std::chrono::seconds(1));

    jw.setPeriodicScanInterval(1000);
    jw.enableSTA(true);
    jw.enableScan(true);
    jw.addNetwork("TEST", "testtesttest");

    std::mutex jw_lock;

    std::future<void> initial_connection = std::async(std::launch::async, [&lock = jw_lock](){
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::lock_guard<std::mutex> guard(lock);
        WiFi._status = WL_CONNECTED;
        WiFi._rssi = -75;
        WiFi._channel = 6;
        WiFi._ssid = "TEST";
        return;
    });

    std::future<void> scramble_rssi = std::async(std::launch::async, [&lock = jw_lock](){
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        std::lock_guard<std::mutex> guard(lock);
        WiFi.addNetworkInfo("TEST", ENC_TYPE_CCMP, -40, {0x11, 0x12, 0x13, 0x14, 0x15, 0x20}, 6, false);
        return;
    });

    /*
    std::future<void> accidental_disconnect = std::async(std::launch::async, [&lock = jw_lock](){
        std::this_thread::sleep_for(std::chrono::milliseconds(5500));
        std::lock_guard<std::mutex> guard(lock);
        WiFi._status = WL_DISCONNECTED;
        WiFi._rssi = -40;
        return;
    });

    std::future<void> check_if_correct = std::async(std::launch::async, [&lock = jw_lock](){
        std::this_thread::sleep_for(std::chrono::milliseconds(7500));
        std::lock_guard<std::mutex> guard(lock);
        WiFi._status = WL_CONNECTED;
        return;
    });
    */

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::lock_guard<std::mutex> guard(jw_lock);
        jw.loop();
    }

}
