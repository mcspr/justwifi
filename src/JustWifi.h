/*

JustWifi 2.0.0

Wifi Manager for ESP8266

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

The JustWifi library is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

The JustWifi library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the JustWifi library.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef JustWifi_h
#define JustWifi_h

#include <functional>
#include <vector>

#include <ESP8266WiFi.h>
#include <pgmspace.h>

extern "C" {
  #include "user_interface.h"
}

// Check NO_EXTRA_4K_HEAP build flag in SDK 2.4.2
#include <core_version.h>
#if defined(JUSTWIFI_ENABLE_WPS) && defined(ARDUINO_ESP8266_RELEASE_2_4_2) && not defined(NO_EXTRA_4K_HEAP)
    #error "SDK 2.4.2 has WPS support disabled by default, enable it by adding -DNO_EXTRA_4K_HEAP to your build flags"
#endif

#define DEFAULT_CONNECT_TIMEOUT         10000
#define DEFAULT_RECONNECT_INTERVAL      60000

#define JUSTWIFI_SMARTCONFIG_TIMEOUT    60000
#define JUSTWIFI_ID_NONE                0xFF

#define JUSTWIFI_RSSI_THRESHOLD                 20
#define JUSTWIFI_PERIODIC_SCAN_INTERVAL         (60000 * 5)

#ifdef DEBUG_ESP_WIFI
#ifdef DEBUG_ESP_PORT
#define DEBUG_WIFI_MULTI(...) DEBUG_ESP_PORT.printf( __VA_ARGS__ )
#endif
#endif

#ifndef DEBUG_WIFI_MULTI
#define DEBUG_WIFI_MULTI(...)
#endif

struct BSSID_t {
    BSSID_t();
    BSSID_t(const BSSID_t&);
    BSSID_t(BSSID_t&&);
    
    void copy(const uint8_t* data);

    BSSID_t& operator=(const BSSID_t& other) = default;

    uint8_t* data() {
        return _data;
    }

    uint8_t operator[](size_t index) {
        if ((index >= 0) && (index < 6)) {
            return _data[index];
        }
        return 0;
    }

    private:
        uint8_t _data[6];
};

struct network_cfg_t {
    network_cfg_t();

    String ssid;
    String pass;
    bool dhcp;
    IPAddress ip;
    IPAddress gw;
    IPAddress netmask;
    IPAddress dns;
};

struct network_t : public network_cfg_t {
    network_t();

    bool scanned;
    int32_t rssi;
    uint8_t security;
    uint8_t channel;
    BSSID_t bssid;
    uint8_t next;
};

struct network_scan_t {
    String ssid;
    int32_t rssi;
    uint8_t security;
    uint8_t* bssid;
    int32_t channel;
    bool hidden;
};

enum justwifi_states_t {
    STATE_IDLE,
    STATE_SCAN_START,
    STATE_SCAN_ONGOING,
    STATE_SCAN_PERIODIC_START,
    STATE_SCAN_PERIODIC_ONGOING,
    STATE_STA_START,
    STATE_STA_ONGOING,
    STATE_STA_FAILED,
    STATE_STA_SUCCESS,
    STATE_WPS_START,
    STATE_WPS_ONGOING,
    STATE_WPS_FAILED,
    STATE_WPS_SUCCESS,
    STATE_SMARTCONFIG_START,
    STATE_SMARTCONFIG_ONGOING,
    STATE_SMARTCONFIG_FAILED,
    STATE_SMARTCONFIG_SUCCESS,
    STATE_FALLBACK
};

enum justwifi_messages_t {
    MESSAGE_SCANNING,
    MESSAGE_SCAN_FAILED,
    MESSAGE_NO_NETWORKS,
    MESSAGE_FOUND_NETWORK,
    MESSAGE_FOUND_BETTER_NETWORK,
    MESSAGE_NO_KNOWN_NETWORKS,
    MESSAGE_CONNECTING,
    MESSAGE_CONNECT_WAITING,
    MESSAGE_CONNECT_FAILED,
    MESSAGE_CONNECTED,
    MESSAGE_ACCESSPOINT_CREATING,
    MESSAGE_ACCESSPOINT_CREATED,
    MESSAGE_ACCESSPOINT_FAILED,
    MESSAGE_ACCESSPOINT_DESTROYED,
    MESSAGE_DISCONNECTED,
    MESSAGE_HOSTNAME_ERROR,
    MESSAGE_TURNING_OFF,
    MESSAGE_TURNING_ON,
    MESSAGE_WPS_START,
    MESSAGE_WPS_SUCCESS,
    MESSAGE_WPS_ERROR,
    MESSAGE_SMARTCONFIG_START,
    MESSAGE_SMARTCONFIG_SUCCESS,
    MESSAGE_SMARTCONFIG_ERROR
};

enum justwifi_responses_t {
    RESPONSE_START,
    RESPONSE_OK,
    RESPONSE_WAIT,
    RESPONSE_FAIL
};

class JustWifi {

    public:

        JustWifi();

        using TMessageFunction = std::function<void(justwifi_messages_t message, char * payload)>;

        void cleanNetworks();
        bool addCurrentNetwork(bool front = false);
        bool addNetwork(
            const char * ssid,
            const char * pass = nullptr,
            const char * ip = nullptr,
            const char * gw = nullptr,
            const char * netmask = nullptr,
            const char * dns = nullptr,
            bool front = false
        );
        bool setSoftAP(
            const char * ssid,
            const char * pass = nullptr,
            const char * ip = nullptr,
            const char * gw = nullptr,
            const char * netmask = nullptr
        );

        void setHostname(const char * hostname);
        void setConnectTimeout(unsigned long ms);
        void setReconnectTimeout(unsigned long ms = DEFAULT_RECONNECT_INTERVAL);
        void resetReconnectTimeout();
        void subscribe(TMessageFunction fn);

        wl_status_t getStatus();
        const String& getAPSSID();

        bool connected();
        bool connectable();

        void turnOff();
        void turnOn();
        void disconnect();
        void enableScan(bool scan);
        void enableSTA(bool enabled);
        void enableAP(bool enabled);
        void enableAPFallback(bool enabled);

        void setPeriodicScanInterval(unsigned long interval);
        void setRSSIThreshold(int8_t rssi);

        #if defined(JUSTWIFI_ENABLE_WPS)
            void startWPS();
        #endif

        #if defined(JUSTWIFI_ENABLE_SMARTCONFIG)
            void startSmartConfig();
        #endif

        void loop();

    private:

        std::vector<network_t> _network_list;
        std::vector<TMessageFunction> _callbacks;

        network_cfg_t _softap;

        uint8_t _currentID;

        unsigned long _connect_timeout = DEFAULT_CONNECT_TIMEOUT;
        unsigned long _reconnect_timeout = DEFAULT_RECONNECT_INTERVAL;
        unsigned long _timeout = 0;
        unsigned long _start = 0;

        bool _scan = false;
        int8_t _scan_rssi_threshold = JUSTWIFI_RSSI_THRESHOLD;
        unsigned long _scan_periodic_interval = JUSTWIFI_PERIODIC_SCAN_INTERVAL;
        unsigned long _scan_periodic_last = 0;

        bool _sta_enabled = true;
        bool _ap_connected = false;
        bool _ap_fallback_enabled = true;

        char _hostname[32];

        justwifi_states_t _state = STATE_IDLE;

        bool _doAP();
        uint8_t _doScan();
        justwifi_responses_t _doSTA(uint8_t id = JUSTWIFI_ID_NONE);

        void _disable();
        void _machine();
        uint8_t _populate(int8_t networkCount, bool keep = false);
        uint8_t _sortByRSSI();
        void _doCallback(justwifi_messages_t message, char * parameter = nullptr);

        bool _checkSSID(const char* ssid);
        bool _checkPass(const char* ssid);

};

extern JustWifi jw;

#endif // ifndef JustWifi_h
