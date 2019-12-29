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

    WiFiClass() {}

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

    struct wifi_scan_t {
        String ssid;
        uint8_t sec;
        int32_t rssi;
        std::vector<uint8_t> bssid;
        int32_t chan;
        bool hidden;
    };

    std::vector<wifi_scan_t> _networks;
    void addNetworkInfo(const String& ssid, uint8_t sec, int32_t rssi, std::vector<uint8_t> BSSID, int32_t chan, bool hidden) {
        _networks.emplace_back(wifi_scan_t{
            ssid, sec, rssi, BSSID, chan, hidden
        });
    }

    void getNetworkInfo(uint8_t i, String& ssid_scan, uint8_t& sec_scan, int32_t& rssi_scan, uint8_t*& BSSID_scan, int32_t& chan_scan, bool& hidden_scan) {
        const auto& network = _networks.at(i);
        ssid_scan = network.ssid;
        sec_scan = network.sec;
        rssi_scan = network.rssi;
        chan_scan = network.chan;
        hidden_scan = network.hidden;
        BSSID_scan = const_cast<uint8_t*>(network.bssid.data());
    }

    void scanNetworks(bool, bool) { }
    int8_t scanComplete() { return _networks.size(); }
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

    int8_t _rssi;
    int8_t RSSI() {
        return _rssi;
    }

    uint8_t _channel;
    uint8_t channel() {
        return _channel;
    }

};

extern WiFiClass WiFi;
