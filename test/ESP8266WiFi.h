#pragma once

#include <Arduino.h>
#include <IPAddress.h>

typedef enum {
    WL_NO_SHIELD        = 255,   // for compatibility with WiFi Shield library
    WL_IDLE_STATUS      = 0,
    WL_NO_SSID_AVAIL    = 1,
    WL_SCAN_COMPLETED   = 2,
    WL_CONNECTED        = 3,
    WL_CONNECT_FAILED   = 4,
    WL_CONNECTION_LOST  = 5,
    WL_DISCONNECTED     = 6
} wl_status_t;

typedef enum WiFiMode 
{
    WIFI_OFF = 0, WIFI_STA = 1, WIFI_AP = 2, WIFI_AP_STA = 3,
    /* these two pseudo modes are experimental: */ WIFI_SHUTDOWN = 4, WIFI_RESUME = 8
} WiFiMode_t;

/* Encryption modes */
enum wl_enc_type {  /* Values map to 802.11 encryption suites... */
        ENC_TYPE_WEP  = 5,
        ENC_TYPE_TKIP = 2,
        ENC_TYPE_CCMP = 4,
        /* ... except these two, 7 and 8 are reserved in 802.11-2007 */
        ENC_TYPE_NONE = 7,
        ENC_TYPE_AUTO = 8
};

#define WIFI_SCAN_RUNNING   (-1)
#define WIFI_SCAN_FAILED    (-2)

struct WiFiClass {

    WiFiClass() :
        bssid1((uint8_t*)malloc(6)),
        bssid2((uint8_t*)malloc(6))
    {}

    void enableSTA(bool) { }
    void enableAP(bool) { }
    void forceSleepWake() { }
    void forceSleepBegin() { }
    void softAPdisconnect() { }
    void softAP(const char*) { }
    void softAP(const char*, const char*) { }
    void config(IPAddress, IPAddress, IPAddress, IPAddress) { }
    void softAPConfig(IPAddress, IPAddress, IPAddress) { }
    void softAPConfig(const char*, const char*, IPAddress) { }
    void disconnect() { }
    void persistent(bool) { }
    void setAutoConnect(bool) { }
    void setAutoReconnect(bool) { }

    void begin(const char*, const char*) { }
    void begin(const char*, const char*, uint8_t, uint8_t*) { }

    String _hostname;
    void hostname(const char* host) {
        _hostname = host;
    }

    uint8_t* bssid1;
    uint8_t* bssid2;

    void getNetworkInfo(uint8_t i, String& ssid_scan, uint8_t& sec_scan, int32_t& rssi_scan, uint8_t*& BSSID_scan, int32_t& chan_scan, bool& hidden_scan) {
        ssid_scan = "TEST";
        sec_scan = ENC_TYPE_CCMP;
        chan_scan = 6;
        hidden_scan = false;
        if (i == 0) {
            rssi_scan = 60;
            BSSID_scan = bssid1;
            BSSID_scan[0] = 0x11;
            BSSID_scan[1] = 0x12;
            BSSID_scan[2] = 0x13;
            BSSID_scan[3] = 0x14;
            BSSID_scan[4] = 0x15;
            BSSID_scan[5] = 0x20;
        } else if (i == 1) {
            rssi_scan = 50;
            BSSID_scan = bssid2;
            BSSID_scan[0] = 0x11;
            BSSID_scan[1] = 0x12;
            BSSID_scan[2] = 0x13;
            BSSID_scan[3] = 0x14;
            BSSID_scan[4] = 0x15;
            BSSID_scan[5] = 0x10;
        }
    }


    void scanNetworks(bool, bool) { }
    int8_t scanComplete() { return 2; }
    void scanDelete() { }

    String _ssid;
    String SSID() {
        return _ssid;
    }

    String _psk;
    String psk() {
        return _psk;
    }

    wl_status_t _status;
    wl_status_t status() {
        return _status;
    }

    WiFiMode_t _mode;
    WiFiMode_t getMode() {
        return _mode;
    }
    bool mode(WiFiMode_t mode) {
        _mode = mode;
    }

};

extern WiFiClass WiFi;
