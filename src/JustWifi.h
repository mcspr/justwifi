/*

JustWifi, Wifi Manager for ESP8266

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

#include <ESP8266WiFi.h>
#include <forward_list>
#include <list>

// Check NO_EXTRA_4K_HEAP build flag in SDK 2.4.2
#include <core_version.h>
#if defined(JUSTWIFI_ENABLE_WPS) && defined(ARDUINO_ESP8266_RELEASE_2_4_2) && not defined(NO_EXTRA_4K_HEAP)
    #error "SDK 2.4.2 has WPS support disabled by default, enable it by adding -DNO_EXTRA_4K_HEAP to your build flags"
#endif

#define DEFAULT_CONNECT_TIMEOUT         10000
#define DEFAULT_RECONNECT_INTERVAL      60000
#define JUSTWIFI_SMARTCONFIG_TIMEOUT    60000

#ifdef DEBUG_ESP_WIFI
#ifdef DEBUG_ESP_PORT
#define DEBUG_WIFI_MULTI(...) DEBUG_ESP_PORT.printf( __VA_ARGS__ )
#endif
#endif

#ifndef DEBUG_WIFI_MULTI
#define DEBUG_WIFI_MULTI(...)
#endif

typedef struct {
    char * ssid { nullptr };
    char * pass { nullptr };
    bool dhcp { false };
    bool scanned { false };
    IPAddress ip;
    IPAddress gw;
    IPAddress netmask;
    IPAddress dns;
    int32_t rssi { 0 };
    uint8_t security { 0u };
    uint8_t channel { 0u };
    uint8_t bssid[6] { 0u };
#if JUSTWIFI_ENABLE_ENTERPRISE
    char * enterprise_username { nullptr };
    char * enterprise_password { nullptr };
#endif
} network_t;

typedef enum {
    STATE_IDLE,
    STATE_SCAN_START,
    STATE_SCAN_ONGOING,
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
} justwifi_states_t;

typedef enum {
    MESSAGE_SCANNING,
    MESSAGE_SCAN_FAILED,
    MESSAGE_NO_NETWORKS,
    MESSAGE_FOUND_NETWORK,
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
} justwifi_messages_t;

enum {
    RESPONSE_START,
    RESPONSE_OK,
    RESPONSE_WAIT,
    RESPONSE_FAIL
};

class JustWifi {

    public:

        static constexpr size_t SsidSizeMax { 32u };
        static constexpr size_t PassphraseSizeMax { 64u };

        using callback_type = void(*)(justwifi_messages_t, char *);
        using callbacks_type = std::forward_list<callback_type>;

        using networks_type = std::list<network_t>;

        JustWifi();
        ~JustWifi();

        void cleanNetworks();
        bool addCurrentNetwork();
        bool addNetwork(
            const char * ssid,
            const char * pass = nullptr,
            const char * ip = nullptr,
            const char * gw = nullptr,
            const char * netmask = nullptr,
            const char * dns = nullptr
        );

#if JUSTWIFI_ENABLE_ENTERPRISE
        bool addEnterpriseNetwork(
            const char * ssid,
            const char * enterprise_username = nullptr,
            const char * enterprise_password = nullptr,
            const char * ip = nullptr,
            const char * gw = nullptr,
            const char * netmask = nullptr,
            const char * dns = nullptr
        );
#endif
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
        void subscribe(callback_type callback);

        wl_status_t getStatus();
        String getAPSSID();

        bool connected();
        bool connectable();

        void turnOff();
        void turnOn();
        void disconnect();
        void enableScan(bool scan);
        void enableSTA(bool enabled);
        void enableAP(bool enabled);
        void enableAPFallback(bool enabled);

        #if defined(JUSTWIFI_ENABLE_WPS)
            void startWPS();
        #endif

        #if defined(JUSTWIFI_ENABLE_SMARTCONFIG)
            void startSmartConfig();
        #endif

        void begin();
        void loop();

    private:
        callbacks_type _callbacks;

        networks_type _network_list;
        networks_type::iterator _current;

        unsigned long _connect_timeout = DEFAULT_CONNECT_TIMEOUT;
        unsigned long _reconnect_timeout = DEFAULT_RECONNECT_INTERVAL;
        unsigned long _timeout = 0;
        unsigned long _start = 0;
        bool _scan = false;
        char _hostname[33];
        network_t _softap;

        justwifi_states_t _state = STATE_IDLE;
        bool _sta_enabled = true;

        char _ap_ssid[33] { 0 };
        char _ap_pass[65] { 0 };

        bool _ap_connected = false;
        bool _ap_fallback_enabled = true;

        bool _doAP();
        uint8_t _doScan();
        uint8_t _doSTA(networks_type::iterator);

        void _disable();
        void _machine();
        uint8_t _populate(uint8_t networkCount);
        String _MAC2String(const unsigned char* mac);
        String _encodingString(uint8_t security);
        void _doCallback(justwifi_messages_t message, char * parameter = nullptr);

};

extern JustWifi jw;

#endif
