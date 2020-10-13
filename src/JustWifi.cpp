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

#include "JustWifi.h"

#include <user_interface.h>
#include <cstring>

// -----------------------------------------------------------------------------
// WPA2E support (no support from Arduino Core WiFi, needs manual SDK calls!)
// -----------------------------------------------------------------------------

#ifdef JUSTWIFI_ENABLE_ENTERPRISE

#include <wpa2_enterprise.h>

#endif

// -----------------------------------------------------------------------------
// WPS callbacks
// -----------------------------------------------------------------------------

#if defined(JUSTWIFI_ENABLE_WPS)

static wps_cb_status _jw_wps_status;

void _jw_wps_status_cb(wps_cb_status status) {
    _jw_wps_status = status;
}

#endif // defined(JUSTWIFI_ENABLE_WPS)

//------------------------------------------------------------------------------
// CONSTRUCTOR
//------------------------------------------------------------------------------

JustWifi::JustWifi() :
    _current(_network_list.end())
{
    snprintf_P(_hostname, sizeof(_hostname), PSTR("ESP-%06X"), ESP.getChipId());
}

JustWifi::~JustWifi() {
    cleanNetworks();
}

void JustWifi::begin() {
    WiFi.persistent(false);
    WiFi.enableAP(false);
    WiFi.enableSTA(false);
}

//------------------------------------------------------------------------------
// PRIVATE METHODS
//------------------------------------------------------------------------------

void JustWifi::_disable() {

#if defined(ARDUINO_ESP8266_RELEASE_2_3_0)
    // See https://github.com/esp8266/Arduino/issues/2186
    if ((WiFi.getMode() & WIFI_AP) > 0) {
        WiFi.mode(WIFI_OFF);
        delay(10);
        WiFi.enableAP(true);
    } else {
        WiFi.mode(WIFI_OFF);
    }
#endif // defined(ARDUINO_ESP8266_RELEASE_2_3_0)

}

String JustWifi::_encodingString(uint8_t security) {
    if (security == ENC_TYPE_WEP) return String("WEP ");
    if (security == ENC_TYPE_TKIP) return String("WPA ");
    if (security == ENC_TYPE_CCMP) return String("WPA2");
    if (security == ENC_TYPE_AUTO) return String("AUTO");
    return String("OPEN");
}

uint8_t JustWifi::_populate(uint8_t networkCount) {

    uint8_t count = 0;

    // Reset RSSI to disable networks that have disappeared
    for (auto& entry : _network_list) {
        entry.rssi = 0;
        entry.scanned = false;
    }

    String ssid_scan;
    int32_t rssi_scan;
    uint8_t sec_scan;
    uint8_t* BSSID_scan;
    int32_t chan_scan;
    bool hidden_scan;

    // Populate defined networks with scan data
    for (int8_t i = 0; i < networkCount; ++i) {

        WiFi.getNetworkInfo(i, ssid_scan, sec_scan, rssi_scan, BSSID_scan, chan_scan, hidden_scan);

        bool known = false;

        for (auto& entry : _network_list) {
            if (ssid_scan.equals(entry.ssid)) {

                // Check security
                if ((sec_scan != ENC_TYPE_NONE) && (entry.pass == nullptr)) continue;

                // In case of several networks with the same SSID
                // we want to get the one with the best RSSI
                // Thanks to Robert (robi772 @ bitbucket.org)
                if (entry.rssi < rssi_scan || entry.rssi == 0) {
                    entry.rssi = rssi_scan;
                    entry.security = sec_scan;
                    entry.channel = chan_scan;
                    entry.scanned = true;
                    std::memcpy(&entry.bssid, BSSID_scan, sizeof(entry.bssid));
                }

                count++;
                known = true;
                break;

            }

        }

		{
		    char buffer[128];
		    sprintf_P(buffer,
		        PSTR("%s BSSID: %02X:%02X:%02X:%02X:%02X:%02X CH: %2d RSSI: %3d SEC: %s SSID: %s"),
		        (known ? "-->" : "   "),
		        BSSID_scan[0], BSSID_scan[1], BSSID_scan[2], BSSID_scan[3], BSSID_scan[4], BSSID_scan[5],
		        chan_scan,
                rssi_scan,
                _encodingString(sec_scan).c_str(),
		        ssid_scan.c_str()
		    );
		    _doCallback(MESSAGE_FOUND_NETWORK, buffer);
		}

    }

    _network_list.sort([](const network_t& lhs, const network_t& rhs) {
        return (lhs.rssi && rhs.rssi) ? lhs.rssi > rhs.rssi : false;
    });

    return count;

}

uint8_t JustWifi::_doSTA(networks_type::iterator it) {

    static networks_type::iterator network { _network_list.end() };
    static uint8_t state { RESPONSE_START };
    static unsigned long timeout { 0ul };

    // Reset connection process
    if (network != it) {
        state = RESPONSE_START;
        network = it;
    }

    if (network == _network_list.end()) {
        return (state = RESPONSE_FAIL);
    }

    auto& entry = *network;

    // No state or previous network failed
    if (RESPONSE_START == state) {

        _disable();
        WiFi.enableSTA(true);
        WiFi.hostname(_hostname);

        // Configure static options
        if (!entry.dhcp) {
            WiFi.config(entry.ip, entry.gw, entry.netmask, entry.dns);
        }

        // Connect
		{
            char buffer[128];
            if (entry.scanned) {
                snprintf_P(buffer, sizeof(buffer),
                    PSTR("BSSID: %02X:%02X:%02X:%02X:%02X:%02X CH: %02d, RSSI: %3d, SEC: %s, SSID: %s"),
                    entry.bssid[0], entry.bssid[1], entry.bssid[2], entry.bssid[3], entry.bssid[4], entry.bssid[5],
                    entry.channel,
                    entry.rssi,
                    _encodingString(entry.security).c_str(),
                    entry.ssid
                );
            } else {
                snprintf_P(buffer, sizeof(buffer), PSTR("SSID: %s"), entry.ssid);
            }
		    _doCallback(MESSAGE_CONNECTING, buffer);
        }

#ifdef JUSTWIFI_ENABLE_ENTERPRISE
        // **Note**: this will only work with PEAP/TTPS configurations, see:
        // https://github.com/xoseperez/justwifi/pull/18

        // We need to manually do the connection, without WiFi.begin()
        if (entry.enterprise_username && entry.enterprise_password) {
            station_config wifi_config{};

            // c/p from ESP8266WiFiSTA
            // since we never pass along the real SSID size, we depend on '\0' < 32 and no '\0' when exactly 32
            if (SsidSizeMax == strlen(entry.ssid)) {
                std::memcpy(reinterpret_cast<char*>(wifi_config.ssid), entry.ssid, SsidSizeMax);
            } else {
                std::strcpy(reinterpret_cast<char*>(wifi_config.ssid), entry.ssid);
            }

            wifi_config.bssid_set = 0;
            if (entry.channel) {
                wifi_config.bssid_set = 1;
                std::memcpy(reinterpret_cast<uint8_t*>(wifi_config.bssid), entry.bssid, sizeof(entry.bssid));
            }

            *wifi_config.password = 0;

            // TODO: from ESP8266WiFiSTA, we see following calls around most of these funcs
            // > ETS_UART_INTR_DISABLE();
            // > ... call something() ...
            // > ETS_UART_INTR_ENABLE();
            // Do we need those? e.g., nodemcu-firmware code does not bother with this lock:
            // e.g. https://github.com/nodemcu/nodemcu-firmware/blob/...branch.../app/modules/wifi.c
            wifi_set_opmode(STATION_MODE);
            wifi_station_set_config_current(&wifi_config);
            wifi_station_set_enterprise_disable_time_check(1);
            wifi_station_clear_cert_key();
            wifi_station_clear_enterprise_ca_cert();
            wifi_station_set_wpa2_enterprise_auth(1);

            // Note: this is safe on ESP b/c we use -funsigned-char
            wifi_station_set_enterprise_identity((uint8_t*)entry.enterprise_username, strlen(entry.enterprise_username));
            wifi_station_set_enterprise_username((uint8_t*)entry.enterprise_username, strlen(entry.enterprise_username));
            wifi_station_set_enterprise_password((uint8_t*)entry.enterprise_password, strlen(entry.enterprise_password));

            wifi_station_connect();
            if (entry.channel) {
                wifi_set_channel(entry.channel);
            }

            // We don't (?) need identity structs in RAM anymore
            wifi_station_clear_enterprise_identity();
            wifi_station_clear_enterprise_username();
            wifi_station_clear_enterprise_password();
            wifi_station_clear_cert_key();
            wifi_station_clear_enterprise_ca_cert();
        } else
#endif

        if (entry.channel) {
            WiFi.begin(entry.ssid, entry.pass, entry.channel, entry.bssid);
        } else {
            WiFi.begin(entry.ssid, entry.pass);
        }

        timeout = millis();
        return (state = RESPONSE_WAIT);

    }

    // Connected?
    if (WiFi.status() == WL_CONNECTED) {

        // Autoconnect only if DHCP, since it doesn't store static IP data
        WiFi.setAutoConnect(entry.dhcp);

        WiFi.setAutoReconnect(true);
        _doCallback(MESSAGE_CONNECTED);
        return (state = RESPONSE_OK);

    }

    // Check timeout
    if (millis() - timeout > _connect_timeout) {
        WiFi.enableSTA(false);
        _doCallback(MESSAGE_CONNECT_FAILED, entry.ssid);
        return (state = RESPONSE_FAIL);
    }

    // Still waiting
    _doCallback(MESSAGE_CONNECT_WAITING);
    return state;

}

bool JustWifi::_doAP() {

    // If already created recreate
    if (_ap_connected) enableAP(false);

    // If we never set anything via setSoftAP, use default hostname as SSID
    if (!_softap.ssid) {
        _softap.ssid = _hostname;
    }

    WiFi.enableAP(true);

    // Configure static options
    if (_softap.dhcp) {
        WiFi.softAPConfig(_softap.ip, _softap.gw, _softap.netmask);
    }

    _doCallback(MESSAGE_ACCESSPOINT_CREATING);

    if (_softap.pass) {
        WiFi.softAP(_softap.ssid, _softap.pass);
    } else {
        WiFi.softAP(_softap.ssid);
    }

    _doCallback(MESSAGE_ACCESSPOINT_CREATED);

    _ap_connected = true;
    return true;

}

uint8_t JustWifi::_doScan() {

    static bool scanning = false;

    // If not scanning, start scan
    if (false == scanning) {
        WiFi.disconnect();
        WiFi.enableSTA(true);
        WiFi.scanNetworks(true, true);
        _doCallback(MESSAGE_SCANNING);
        scanning = true;
        return RESPONSE_WAIT;
    }

    // Check if scanning
    int8_t scanResult = WiFi.scanComplete();
    if (WIFI_SCAN_RUNNING == scanResult) {
        return RESPONSE_WAIT;
    }

    // Scan finished
    scanning = false;

    // Sometimes the scan fails,
    // this will force the scan to restart
    if (WIFI_SCAN_FAILED == scanResult) {
        _doCallback(MESSAGE_SCAN_FAILED);
        return RESPONSE_WAIT;
    }

    // Check networks
    if (0 == scanResult) {
        _doCallback(MESSAGE_NO_NETWORKS);
        return RESPONSE_FAIL;
    }

    // Populate network list
    uint8_t count = _populate(scanResult);

    // Free memory, since the Core will keep it otherwise
    WiFi.scanDelete();

    if (0 == count) {
        _doCallback(MESSAGE_NO_KNOWN_NETWORKS);
        return RESPONSE_FAIL;
    }

    return RESPONSE_OK;

}

void JustWifi::_doCallback(justwifi_messages_t message, char * parameter) {
    for (auto& callback : _callbacks) {
        callback(message, parameter);
    }
}

String JustWifi::_MAC2String(const unsigned char* mac) {
    char buffer[20];
    snprintf(
        buffer, sizeof(buffer),
        "%02x:%02x:%02x:%02x:%02x:%02x",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
    );
    return String(buffer);
}

void JustWifi::_machine() {

    #if false
        static unsigned char previous = 0xFF;
        if (_state != previous) {
            previous = _state;
            Serial.printf("_state: %u, WiFi.getMode(): %u\n", _state, WiFi.getMode());
        }
    #endif

    switch(_state) {

        // ---------------------------------------------------------------------

        case STATE_IDLE:

            // Should we connect in STA mode?
            if (WiFi.status() != WL_CONNECTED) {

                if (_sta_enabled) {
                    if (_network_list.size()) {
                        if ((0 == _timeout) || ((_reconnect_timeout > 0) && (millis() - _timeout > _reconnect_timeout))) {
                            _current = _network_list.begin();
                            _state = _scan ? STATE_SCAN_START : STATE_STA_START;
                            return;
                        }
                    }
                }

                // Fallback
                if (!_ap_connected && _ap_fallback_enabled) {
                    _state = STATE_FALLBACK;
                }

            }


            break;

        // ---------------------------------------------------------------------

        case STATE_SCAN_START:
            _doScan();
            _state = STATE_SCAN_ONGOING;
            break;

        case STATE_SCAN_ONGOING:
            {
                uint8_t response = _doScan();
                if (RESPONSE_OK == response) {
                    _state = STATE_STA_START;
                } else if (RESPONSE_FAIL == response) {
                    _state = STATE_FALLBACK;
                }
            }
            break;

        // ---------------------------------------------------------------------

        case STATE_STA_START:
            _doSTA(_current);
            _state = STATE_STA_ONGOING;
            break;

        case STATE_STA_ONGOING:
            switch (_doSTA(_current)) {
            case RESPONSE_OK:
                _state = STATE_STA_SUCCESS;
                break;
            case RESPONSE_FAIL:
                if (_current == _network_list.end()) {
                    _state = STATE_STA_FAILED;
                    break;
                }

                _state = STATE_STA_START;
                std::advance(_current, 1);
                if (_current == _network_list.end()) {
                    _state = STATE_STA_FAILED;
                }
                break;
            case RESPONSE_START:
            case RESPONSE_WAIT:
                break;
            }
            break;

        case STATE_STA_FAILED:
            _state = STATE_FALLBACK;
            break;

        case STATE_STA_SUCCESS:
            _state = STATE_IDLE;
            break;

        // ---------------------------------------------------------------------

        #if defined(JUSTWIFI_ENABLE_WPS)

        case STATE_WPS_START:

            _doCallback(MESSAGE_WPS_START);

            _disable();

            if (!WiFi.enableSTA(true)) {
                _state = STATE_WPS_FAILED;
                return;
            }

            WiFi.disconnect();

            if (!wifi_wps_disable()) {
                _state = STATE_WPS_FAILED;
                return;
            }

            // so far only WPS_TYPE_PBC is supported (SDK 1.2.0)
            if (!wifi_wps_enable(WPS_TYPE_PBC)) {
                _state = STATE_WPS_FAILED;
                return;
            }

            _jw_wps_status = (wps_cb_status) 5;
            if (!wifi_set_wps_cb((wps_st_cb_t) &_jw_wps_status_cb)) {
                _state = STATE_WPS_FAILED;
                return;
            }

            if (!wifi_wps_start()) {
                _state = STATE_WPS_FAILED;
                return;
            }

            _state = STATE_WPS_ONGOING;
            break;

        case STATE_WPS_ONGOING:
            if (5 == _jw_wps_status) {
                // Still ongoing
            } else if (WPS_CB_ST_SUCCESS == _jw_wps_status) {
                _state = STATE_WPS_SUCCESS;
            } else {
                _state = STATE_WPS_FAILED;
            }
            break;

        case STATE_WPS_FAILED:
            _doCallback(MESSAGE_WPS_ERROR);
            wifi_wps_disable();
            _state = STATE_FALLBACK;
            break;

        case STATE_WPS_SUCCESS:
            _doCallback(MESSAGE_WPS_SUCCESS);
            wifi_wps_disable();
            addCurrentNetwork();
            _state = STATE_IDLE;
            break;

        #endif // defined(JUSTWIFI_ENABLE_WPS)

        // ---------------------------------------------------------------------

        #if defined(JUSTWIFI_ENABLE_SMARTCONFIG)

        case STATE_SMARTCONFIG_START:

            _doCallback(MESSAGE_SMARTCONFIG_START);

            enableAP(false);

            if (!WiFi.beginSmartConfig()) {
                _state = STATE_SMARTCONFIG_FAILED;
                return;
            }

            _state = STATE_SMARTCONFIG_ONGOING;
            _start = millis();

            break;

        case STATE_SMARTCONFIG_ONGOING:
            if (WiFi.smartConfigDone()) {
                _state = STATE_SMARTCONFIG_SUCCESS;
            } else if (millis() - _start > JUSTWIFI_SMARTCONFIG_TIMEOUT) {
                _state = STATE_SMARTCONFIG_FAILED;
            }
            break;

        case STATE_SMARTCONFIG_FAILED:
            _doCallback(MESSAGE_SMARTCONFIG_ERROR);
            WiFi.stopSmartConfig();
            WiFi.enableSTA(false);
            _state = STATE_FALLBACK;
            break;

        case STATE_SMARTCONFIG_SUCCESS:
            _doCallback(MESSAGE_SMARTCONFIG_SUCCESS);
            addCurrentNetwork();
            _state = STATE_IDLE;
            break;

        #endif // defined(JUSTWIFI_ENABLE_SMARTCONFIG)

        // ---------------------------------------------------------------------

        case STATE_FALLBACK:
            if (!_ap_connected && _ap_fallback_enabled) _doAP();
            _timeout = millis();
            _state = STATE_IDLE;
            break;

        default:
            _state = STATE_IDLE;
            break;

    }

}

//------------------------------------------------------------------------------
// CONFIGURATION METHODS
//------------------------------------------------------------------------------

void JustWifi::cleanNetworks() {
    for (auto& entry : _network_list) {
        free(entry.ssid);
        free(entry.pass);
#if JUSTWIFI_ENABLE_ENTERPRISE
        free(entry.enterprise_username);
        free(entry.enterprise_password);
#endif
    }
    _network_list.clear();
}

namespace {

bool _can_set_credentials(const char* ssid, const char* pass) {
    return ((ssid && *ssid != '\0' && strlen(ssid) <= JustWifi::SsidSizeMax) && (!pass || (strlen(pass) <= JustWifi::PassphraseSizeMax)));
}

bool _maybe_set_dhcp(network_t& network, const char* ip, const char* gw, const char* netmask) {
    if (ip && gw && netmask
        && *ip != '\0' && *gw != '\0' && *netmask != '\0') {
        network.ip.fromString(ip);
        network.gw.fromString(gw);
        network.netmask.fromString(netmask);
        return true;
    }

    return false;
}

} // namespace

bool JustWifi::addNetwork(
    const char * ssid,
    const char * pass,
    const char * ip,
    const char * gw,
    const char * netmask,
    const char * dns
) {

    if (!_can_set_credentials(ssid, pass)) {
        return false;
    }

    network_t new_network;

    // Copy SSID and PASS directly, as strings Arduino API expects

    new_network.ssid = strdup(ssid);
    if (!new_network.ssid) {
        return false;
    }

    if (pass && *pass != '\0') {
        new_network.pass = strdup(pass);
        if (!new_network.pass) {
            free(new_network.ssid);
            return false;
        }
    }

    // normal network will set dhcp flag when ip, gw and netmask are not set
    new_network.dhcp = !_maybe_set_dhcp(new_network, ip, gw, netmask);
    if (dns && *dns != '\0') {
        new_network.dns.fromString(dns);
    }

    _network_list.push_back(new_network);

    return true;

}

#if JUSTWIFI_ENABLE_ENTERPRISE

bool JustWifi::addEnterpriseNetwork(
    const char * ssid,
    const char * enterprise_username,
    const char * enterprise_password,
    const char * ip,
    const char * gw,
    const char * netmask,
    const char * dns
) {
    if (!enterprise_username \
        || !enterprise_password \
        || *enterprise_username == '\0' \
        || *enterprise_password == '\0'
    ) {
        return false;
    }

    if (!addNetwork(ssid, nullptr, ip, gw, netmask, dns)) {
        return false;
    }

    auto& network = _network_list.back();
    network.enterprise_username = strdup(enterprise_username);
    network.enterprise_password = strdup(enterprise_password);

    return true;
}

#endif // JUSTWIFI_ENABLE_ENTERPRISE

bool JustWifi::addCurrentNetwork() {
    return addNetwork(
        WiFi.SSID().c_str(),
        WiFi.psk().c_str(),
        nullptr, nullptr, nullptr, nullptr
    );
}

bool JustWifi::setSoftAP(
    const char * ssid,
    const char * pass,
    const char * ip,
    const char * gw,
    const char * netmask
) {

    if (!_can_set_credentials(ssid, pass)) {
        return false;
    }

    _softap.ssid = _ap_ssid;
    std::memcpy(_softap.ssid, ssid, std::min(sizeof(_ap_ssid), strlen(ssid) + 1));

    if (pass && *pass != '\0') {
        _softap.pass = _ap_pass;
        std::memcpy(_softap.pass, pass, std::min(sizeof(_ap_pass), strlen(pass) + 1));
    }

    // softap sets dhcp flag when ip, gw and netmask are set
    // (meaning, we will propogate those via DHCP)
    _softap.dhcp = _maybe_set_dhcp(_softap, ip, gw, netmask);

    // https://github.com/xoseperez/justwifi/issues/4
    if ((WiFi.getMode() & WIFI_AP) > 0) {
        if (_softap.pass) {
            WiFi.softAP(_softap.ssid, _softap.pass);
        } else {
            WiFi.softAP(_softap.ssid);
        }
    }

    return true;

}

void JustWifi::setConnectTimeout(unsigned long ms) {
    _connect_timeout = ms;
}

void JustWifi::setReconnectTimeout(unsigned long ms) {
    _reconnect_timeout = ms;
}

void JustWifi::resetReconnectTimeout() {
    _timeout = millis();
}

void JustWifi::setHostname(const char * hostname) {
    strncpy(_hostname, hostname, sizeof(_hostname));
}

void JustWifi::subscribe(callback_type callback) {
    _callbacks.push_front(callback);
}

//------------------------------------------------------------------------------
// PUBLIC METHODS
//------------------------------------------------------------------------------

wl_status_t JustWifi::getStatus() {
    return WiFi.status();
}

String JustWifi::getAPSSID() {
    return String(_softap.ssid);
}

bool JustWifi::connected() {
    return (WiFi.status() == WL_CONNECTED);
}

bool JustWifi::connectable() {
    return _ap_connected;
}

void JustWifi::disconnect() {
    _timeout = 0;
    WiFi.disconnect();
    WiFi.enableSTA(false);
    _doCallback(MESSAGE_DISCONNECTED);
}

void JustWifi::turnOff() {
    WiFi.disconnect();
    WiFi.enableAP(false);
    WiFi.enableSTA(false);
    WiFi.forceSleepBegin();
    delay(1);
    _doCallback(MESSAGE_TURNING_OFF);
    _sta_enabled = false;
    _state = STATE_IDLE;
}

void JustWifi::turnOn() {
    WiFi.forceSleepWake();
    delay(1);
    setReconnectTimeout(0);
    _doCallback(MESSAGE_TURNING_ON);
    WiFi.enableSTA(true);
    _sta_enabled = true;
    _state = STATE_IDLE;
}

#if defined(JUSTWIFI_ENABLE_WPS)
void JustWifi::startWPS() {
    _state = STATE_WPS_START;
}
#endif // defined(JUSTWIFI_ENABLE_WPS)

#if defined(JUSTWIFI_ENABLE_SMARTCONFIG)
void JustWifi::startSmartConfig() {
    _state = STATE_SMARTCONFIG_START;
}
#endif // defined(JUSTWIFI_ENABLE_SMARTCONFIG)

void JustWifi::enableSTA(bool enabled) {
    _sta_enabled = enabled;
}

void JustWifi::enableAP(bool enabled) {
    if (enabled) {
        _doAP();
    } else {
        WiFi.softAPdisconnect();
        WiFi.enableAP(false);
        _ap_connected = false;
        _doCallback(MESSAGE_ACCESSPOINT_DESTROYED);
    }
}

void JustWifi::enableAPFallback(bool enabled) {
    _ap_fallback_enabled = enabled;
}


void JustWifi::enableScan(bool scan) {
    _scan = scan;
}

void JustWifi::loop() {
    _machine();
}

JustWifi jw;
