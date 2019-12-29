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

#include "JustWifi.h"
#include <functional>

// -----------------------------------------------------------------------------
// WPS callbacks
// -----------------------------------------------------------------------------

#if defined(JUSTWIFI_ENABLE_WPS)

#include "user_interface.h"

static wps_cb_status _jw_wps_status;

static void _jw_wps_status_cb(wps_cb_status status) {
    _jw_wps_status = status;
}

#endif // defined(JUSTWIFI_ENABLE_WPS)

//------------------------------------------------------------------------------
// CONSTRUCTOR
//------------------------------------------------------------------------------

JustWifi::JustWifi() {
    WiFi.enableAP(false);
    WiFi.enableSTA(false);
    snprintf_P(_hostname, sizeof(_hostname), PSTR("ESP-%06X"), ESP.getChipId());
}

//------------------------------------------------------------------------------
// NETWORK MANAGEMENT
//------------------------------------------------------------------------------

BSSID_t::BSSID_t() :
    _data{}
{}

void BSSID_t::copy(const uint8_t* data) {
    memcpy(_data, data, sizeof(_data));
}

BSSID_t::BSSID_t(const BSSID_t& other) {
    copy(other._data);
}

BSSID_t::BSSID_t(BSSID_t&& other) :
    BSSID_t(other)
{}

network_cfg_t::network_cfg_t() :
    ssid(),
    pass(),
    dhcp(false),
    ip(),
    gw(),
    netmask(),
    dns()
{}

network_t::network_t() :
    scanned(false),
    rssi(0),
    security(0),
    channel(0),
    bssid(),
    next(JUSTWIFI_ID_NONE)
{}

// Check SSID too long or missing
bool JustWifi::_checkSSID(const char* ssid) {
    if (ssid && *ssid != '\0' && strlen(ssid) <= 31) {
        return true;
    }
    return false;
}

// Check PASS too long (missing is OK, since there are OPEN networks)
// TODO: 64 char passphrase?
bool JustWifi::_checkPass(const char* pass) {
    if (pass && strlen(pass) <= 63) {
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
// VALUE-2-STRING HELPERS
//------------------------------------------------------------------------------

static String _state_to_string(justwifi_states_t state) {
    const __FlashStringHelper *result;
    switch (state) {
        case STATE_IDLE: result = F("STATE_IDLE"); break;
        case STATE_SCAN_START: result = F("STATE_SCAN_START"); break;
        case STATE_SCAN_ONGOING: result = F("STATE_SCAN_ONGOING"); break;
        case STATE_SCAN_PERIODIC_START: result = F("STATE_SCAN_PERIODIC_START"); break;
        case STATE_SCAN_PERIODIC_ONGOING: result = F("STATE_SCAN_PERIODIC_ONGOING"); break;
        case STATE_STA_START: result = F("STATE_STA_START"); break;
        case STATE_STA_ONGOING: result = F("STATE_STA_ONGOING"); break;
        case STATE_STA_FAILED: result = F("STATE_STA_FAILED"); break;
        case STATE_STA_SUCCESS: result = F("STATE_STA_SUCCESS"); break;
        case STATE_WPS_START: result = F("STATE_WPS_START"); break;
        case STATE_WPS_ONGOING: result = F("STATE_WPS_ONGOING"); break;
        case STATE_WPS_FAILED: result = F("STATE_WPS_FAILED"); break;
        case STATE_WPS_SUCCESS: result = F("STATE_WPS_SUCCESS"); break;
        case STATE_SMARTCONFIG_START: result = F("STATE_SMARTCONFIG_START"); break;
        case STATE_SMARTCONFIG_ONGOING: result = F("STATE_SMARTCONFIG_ONGOING"); break;
        case STATE_SMARTCONFIG_FAILED: result = F("STATE_SMARTCONFIG_FAILED"); break;
        case STATE_SMARTCONFIG_SUCCESS: result = F("STATE_SMARTCONFIG_SUCCESS"); break;
        case STATE_FALLBACK: result = F("STATE_FALLBACK"); break;
    }
    return String(result);
}

static String _wifimode_to_string(WiFiMode_t mode) {
    const __FlashStringHelper *result;
    switch (mode) {
        case WIFI_OFF: result = F("OFF"); break;
        case WIFI_AP: result = F("AP"); break;
        case WIFI_STA: result = F("STA"); break;
        case WIFI_AP_STA: result = F("AP_STA"); break;
        default: result = F("UNKNOWN"); break;
    }
    return String(result);
}

static String _wifistatus_to_string(wl_status_t status) {
    const __FlashStringHelper *result;
    switch (status) {
        case WL_IDLE_STATUS: result = F("WL_IDLE_STATUS"); break;
        case WL_NO_SSID_AVAIL: result = F("WL_NO_SSID_AVAIL"); break;
        case WL_SCAN_COMPLETED: result = F("WL_SCAN_COMPLETED"); break;
        case WL_CONNECTED: result = F("WL_CONNECTED"); break;
        case WL_CONNECT_FAILED: result = F("WL_CONNECT_FAILED"); break;
        case WL_CONNECTION_LOST: result = F("WL_CONNECTION_LOST"); break;
        case WL_DISCONNECTED: result = F("WL_DISCONNECTED"); break;
    }
    return String(result);
}

static String _security_to_string(uint8_t security) {
    const __FlashStringHelper *result;
    switch (security) {
        case ENC_TYPE_WEP:
            result = F("WEP ");
            break;
        case ENC_TYPE_TKIP:
            result = F("WPA ");
            break;
        case ENC_TYPE_CCMP:
            result = F("WPA2");
            break;
        case ENC_TYPE_AUTO:
            result = F("AUTO");
            break;
        default:
            result = F("OPEN");
    }
    return String(result);
}

static String _bssid_to_string(const uint8_t* bssid) {
    char buffer[20];
    snprintf(
        buffer, sizeof(buffer),
        "%02X:%02X:%02X:%02X:%02X:%02X",
        bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]
    );
    return String(buffer);
}

static void _update_network_from_scan(const network_scan_t& scan, network_t& network) {
    network.rssi = scan.rssi;
    network.security = scan.security;
    network.channel = scan.channel;
    network.scanned = true;
    network.bssid.copy(scan.bssid);
}

//------------------------------------------------------------------------------
// PRIVATE METHODS
//------------------------------------------------------------------------------

void JustWifi::_disable() {

    // See https://github.com/esp8266/Arduino/issues/2186
    if (strncmp_P(ESP.getSdkVersion(), PSTR("1.5.3"), 5) == 0) {
        if ((WiFi.getMode() & WIFI_AP) > 0) {
            WiFi.mode(WIFI_OFF);
            delay(10);
            WiFi.enableAP(true);
        } else {
            WiFi.mode(WIFI_OFF);
        }

    }

}
uint8_t JustWifi::_sortByRSSI() {

    bool first = true;
    uint8_t bestID = JUSTWIFI_ID_NONE;

    for (uint8_t i = 0; i < _network_list.size(); i++) {

        network_t* entry = &_network_list[i];

        // if no data skip
        if (entry->rssi == 0) continue;

        // Empty list
        if (first) {
            first = false;
            bestID = i;
            entry->next = JUSTWIFI_ID_NONE;

        // The best so far
        } else if (entry->rssi > _network_list[bestID].rssi) {
            entry->next = bestID;
            bestID = i;

        // Walk the list
        } else {

            network_t * current = &_network_list[bestID];
            while (current->next != JUSTWIFI_ID_NONE) {
                if (entry->rssi > _network_list[current->next].rssi) {
                    entry->next = current->next;
                    current->next = i;
                    break;
                }
                current = &_network_list[current->next];
            }

            // Place it the last
            if (current->next == JUSTWIFI_ID_NONE) {
                current->next = i;
                entry->next = JUSTWIFI_ID_NONE;
            }

        }

    }

    // In case we found that the current network has a better alternative, notify the user
    // Then, after calling ::disconnect(), we will use a new network
    if (connected() && ((_network_list[bestID].rssi - WiFi.RSSI()) >= _scan_rssi_threshold)) {
        _doCallback(MESSAGE_FOUND_BETTER_NETWORK);
    }

    return bestID;

}

uint8_t JustWifi::_populate(int8_t networkCount, bool keep) {

    uint8_t count = 0;

    // Copy existing list and reset existing status variables to disable networks that have disappeared
    // If still connected, preserve current value to avoid connecting to a worse quailty network
    auto connectedID = JUSTWIFI_ID_NONE;
    bool foundBetterNetwork = false;
    for (size_t id = 0; id < _network_list.size(); ++id) {
        auto& network = _network_list[id];
        if (keep && connected() && network.ssid.equals(WiFi.SSID())) {
            connectedID = id;
            network.rssi = WiFi.RSSI();
            network.channel = WiFi.channel();
        } else {
            network.rssi = 0;
            network.channel = 0;
        }
        network.scanned = false;
    }

    // Update network list with scan data

    network_scan_t scan;

    for (decltype(networkCount) scan_id = 0; scan_id < networkCount; ++scan_id) {

        WiFi.getNetworkInfo(scan_id, scan.ssid, scan.security, scan.rssi, scan.bssid, scan.channel, scan.hidden);

        bool known = false;

        for (unsigned char networkID = 0; networkID < _network_list.size(); ++networkID) {

            network_t& network = _network_list[networkID];

            if (scan.ssid.equals(network.ssid)) {

                // Ensure that we have a passphrase configured when network is protected
                if ((scan.security != ENC_TYPE_NONE) && (!network.pass.length())) continue;

                // In case of several networks with the same SSID
                // we want to get the one with the best RSSI
                // Thanks to Robert (robi772 @ bitbucket.org)
                if (network.rssi < scan.rssi || network.rssi == 0) {
                    _update_network_from_scan(scan, network);
                }

                {
                    char buffer[128];
                    sprintf_P(buffer,
                        PSTR("%s BSSID: %s CH: %2d RSSI: %3d SEC: %s SSID: %s"),
                        (known ? "-->" : "   "),
                        _bssid_to_string(scan.bssid).c_str(),
                        scan.channel,
                        scan.rssi,
                        _security_to_string(scan.security).c_str(),
                        scan.ssid.c_str()
                    );
                    _doCallback(MESSAGE_FOUND_NETWORK, buffer);
                }

                count++;
                known = true;
                break;

            }

        }

    }

    // After all scan data is copied, we can safely clear Core's buffers
    WiFi.scanDelete();

    return count;

}

justwifi_responses_t JustWifi::_doSTA(uint8_t id) {

    static uint8_t networkID = JUSTWIFI_ID_NONE;
    static auto response = RESPONSE_START;
    static unsigned long timeout = 0;

    // Reset connection process
    if (id != JUSTWIFI_ID_NONE) {
        response = RESPONSE_START;
        networkID = id;
    }

    // Get network when it is properly configured
    if (networkID == JUSTWIFI_ID_NONE) {
        return (response = RESPONSE_FAIL);
    }

    auto& entry = _network_list[networkID];

    // No state or previous network failed
    if (RESPONSE_START == response) {

        WiFi.persistent(false);
        _disable();
        WiFi.enableSTA(true);
        WiFi.hostname(_hostname);

        // Static options need to be configured before WiFi.begin()
        if (!entry.dhcp) {
            WiFi.config(entry.ip, entry.gw, entry.netmask, entry.dns);
        }

        // Try to connect to either specific network or let SDK decide which network to use
		{
            char buffer[128];
            if (entry.scanned) {
                snprintf_P(buffer, sizeof(buffer),
                    PSTR("BSSID: %s CH: %02d, RSSI: %3d, SEC: %s, SSID: %s"),
                    _bssid_to_string(entry.bssid.data()).c_str(),
                    entry.channel,
                    entry.rssi,
                    _security_to_string(entry.security).c_str(),
                    entry.ssid.c_str()
                );
            } else {
                snprintf_P(buffer, sizeof(buffer), PSTR("SSID: %s"), entry.ssid.c_str());
            }
		    _doCallback(MESSAGE_CONNECTING, buffer);
        }

        if (entry.channel == 0) {
            WiFi.begin(entry.ssid.c_str(), entry.pass.c_str());
        } else {
            WiFi.begin(entry.ssid.c_str(), entry.pass.c_str(), entry.channel, entry.bssid.data());
        }

        timeout = millis();
        return (response = RESPONSE_WAIT);

    }

    // Wait until connected ...
    if (WiFi.status() == WL_CONNECTED) {

        // Autoconnect only if DHCP, since it doesn't store static IP data
        WiFi.setAutoConnect(entry.dhcp);

        WiFi.setAutoReconnect(true);
        _doCallback(MESSAGE_CONNECTED);
        return (response = RESPONSE_OK);

    }

    // ... but not longer than specified timeout
    // XXX: must change _doCallback and TMessageFunction 2nd arg to `const char*`
    if (millis() - timeout > _connect_timeout) {
        WiFi.enableSTA(false);
        _doCallback(MESSAGE_CONNECT_FAILED, const_cast<char*>(entry.ssid.c_str()));
        return (response = RESPONSE_FAIL);
    }

    // When still waiting, return the same state
    _doCallback(MESSAGE_CONNECT_WAITING);
    return response;

}

bool JustWifi::_doAP() {

    // If already created recreate
    if (_ap_connected) enableAP(false);

    // Check if Soft AP configuration defined
    if (!_softap.ssid.length()) {
        _softap.ssid = _hostname;
    }

    _doCallback(MESSAGE_ACCESSPOINT_CREATING);

    WiFi.enableAP(true);

    // Configure static options
    if (_softap.dhcp) {
        WiFi.softAPConfig(_softap.ip, _softap.gw, _softap.netmask);
    }

    if (_softap.pass.length()) {
        WiFi.softAP(_softap.ssid.c_str(), _softap.pass.c_str());
    } else {
        WiFi.softAP(_softap.ssid.c_str());
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

    // Check if still scanning ...
    auto scanResult = WiFi.scanComplete();
    if (WIFI_SCAN_RUNNING == scanResult) {
        return RESPONSE_WAIT;
    }

    // Scan is finished at this point, reset for re-entry
    scanning = false;

    // Sometimes the scan fails,
    // this will force the scan to restart
    if (WIFI_SCAN_FAILED == scanResult) {
        _doCallback(MESSAGE_SCAN_FAILED);
        return RESPONSE_WAIT;
    }

    // scanResult returns number of found networks
    if (0 == scanResult) {
        _doCallback(MESSAGE_NO_NETWORKS);
        return RESPONSE_FAIL;
    }

    // When network is found in our list, update RSSI value
    auto count = _populate(scanResult, (_scan_periodic_interval > 0));

    if (0 == count) {
        _doCallback(MESSAGE_NO_KNOWN_NETWORKS);
        return RESPONSE_FAIL;
    }

    // Sort networks by RSSI, return the best network ID
    _currentID = _sortByRSSI();

    _scan_periodic_last = millis();

    return RESPONSE_OK;

}

void JustWifi::_doCallback(justwifi_messages_t message, char * parameter) {
    for (auto& cb : _callbacks) {
        cb(message, parameter);
    }
}

void JustWifi::_machine() {

    #if false
        static unsigned char previous = JUSTWIFI_ID_NONE;
        if (_state != previous) {
            previous = _state;
            Serial.printf("_state: %s (%u), WiFi.getMode(): %s (%u) WiFi.status(): %s (%u)\n",
                _state_to_string(_state).c_str(), _state,
                _wifimode_to_string(WiFi.getMode()).c_str(), WiFi.getMode(),
                _wifistatus_to_string(WiFi.status()).c_str(), WiFi.status()
            );
        }
    #endif

    switch (_state) {

        // ---------------------------------------------------------------------

        case STATE_IDLE: {
            bool periodic_scan = (_scan_periodic_interval && (millis() - _scan_periodic_last >= _scan_periodic_interval));

            if (WiFi.status() == WL_CONNECTED) {

                // When scan interval is configured, launch an async scanning without disconnection
                if (periodic_scan) {
                    _scan_periodic_last = millis();
                    _state = STATE_SCAN_PERIODIC_START;
                    return;
                }

            } else {

                // Try to connect when there are networks available and STA mode is enabled
                if (_sta_enabled) {
                    if (_network_list.size() > 0) {
                        if ((0 == _timeout) || ((_reconnect_timeout > 0) && (millis() - _timeout > _reconnect_timeout))) {
                            // When periodic scan is on, only scan again when periodic timeout expires
                            if (_currentID == 0 || periodic_scan) {
                                _state = _scan ? STATE_SCAN_START : STATE_STA_START;
                            } else {
                                _state = STATE_STA_START;
                            }
                            return;
                        }
                    }
                }

                // Fallback
                if (!_ap_connected & _ap_fallback_enabled) {
                    _state = STATE_FALLBACK;
                }

            }


            break;

        }

        // ---------------------------------------------------------------------

        case STATE_SCAN_PERIODIC_START:
            _doScan();
            _state = STATE_SCAN_PERIODIC_ONGOING;
            break;

        case STATE_SCAN_START:
            _doScan();
            _state = STATE_SCAN_ONGOING;
            break;

        case STATE_SCAN_PERIODIC_ONGOING:
            {
                uint8_t response = _doScan();
                if ((RESPONSE_OK == response) || (RESPONSE_FAIL == response)) {
                    _state = STATE_IDLE;
                }
            }
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
            _doSTA(_currentID);
            _state = STATE_STA_ONGOING;
            break;

        case STATE_STA_ONGOING:
            {
                auto response = _doSTA();
                if (RESPONSE_OK == response) {
                    _state = STATE_STA_SUCCESS;
                } else if (RESPONSE_FAIL == response) {
                    _state = STATE_STA_START;
                    if (_scan) {
                        _currentID = _network_list[_currentID].next;
                        if (_currentID == JUSTWIFI_ID_NONE) {
                            _state = STATE_STA_FAILED;
                        }
                    } else {
                        _currentID++;
                        if (_currentID == _network_list.size()) {
                            _state = STATE_STA_FAILED;
                        }
                    }
                }
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

            // setting status out of enum bounds to avoid default SUCCESS
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
                // Status is still out-of-bounds, meaning WPS has not yet finished
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
            addCurrentNetwork(true);
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
            addCurrentNetwork(true);
            _state = STATE_IDLE;
            break;

        #endif // defined(JUSTWIFI_ENABLE_SMARTCONFIG)

        // ---------------------------------------------------------------------

        case STATE_FALLBACK:
            if (!_ap_connected & _ap_fallback_enabled) _doAP();
            _timeout = millis();
            _scan_periodic_last = _timeout - _scan_periodic_interval;
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
    _network_list.clear();
    _currentID = 0;
}

bool JustWifi::addNetwork(
    const char * ssid,
    const char * pass,
    const char * ip,
    const char * gw,
    const char * netmask,
    const char * dns,
    bool front
) {

    if (!_checkSSID(ssid)) {
        return false;
    }

    if (!_checkPass(pass)) {
        return false;
    }

    network_t new_network;

    // Copy network SSID and PASS directly (will handle empty strings automatically)
    new_network.ssid = ssid;
    new_network.pass = pass;

    // Copy static config only when every item is available
    new_network.dhcp = true;
    if (ip && gw && netmask
        && *ip != '\0' && *gw != '\0' && *netmask != '\0') {
        new_network.dhcp = false;
        new_network.ip.fromString(ip);
        new_network.gw.fromString(gw);
        new_network.netmask.fromString(netmask);
    }

    // DNS setting is separate (?)
    if (dns && *dns != '\0') {
        new_network.dns.fromString(dns);
    }

    // Adding to the front in case we have scanning disabled, but need specific network to connect first
    if (front) {
        _network_list.insert(_network_list.begin(), new_network);
    } else {
        _network_list.push_back(new_network);
    }
    return true;

}

bool JustWifi::addCurrentNetwork(bool front) {
    return addNetwork(
        WiFi.SSID().c_str(),
        WiFi.psk().c_str(),
        nullptr, nullptr, nullptr, nullptr,
        front
    );
}

bool JustWifi::setSoftAP(
    const char * ssid,
    const char * pass,
    const char * ip,
    const char * gw,
    const char * netmask
) {

    if (!_checkSSID(ssid)) {
        return false;
    }

    if (!_checkPass(pass)) {
        return false;
    }

    // Copy network SSID and PASS (SSID cannot be empty, pass will be checked at the end)
    _softap.ssid = ssid;
    _softap.pass = pass;

    // Copy static config only when every item is available
    _softap.dhcp = false;
    if (ip && gw && netmask
        && *ip != '\0' && *gw != '\0' && *netmask != '\0') {
        _softap.dhcp = true;
        _softap.ip.fromString(ip);
        _softap.gw.fromString(gw);
        _softap.netmask.fromString(netmask);
    }

    if ((WiFi.getMode() & WIFI_AP) > 0) {

        // SDK will not update SoftAP config otherwise
        // https://github.com/xoseperez/justwifi/issues/4
        if (_softap.pass.length()) {
            WiFi.softAP(_softap.ssid.c_str(), _softap.pass.c_str());
        } else {
            WiFi.softAP(_softap.ssid.c_str());
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

void JustWifi::subscribe(TMessageFunction fn) {
    _callbacks.push_back(fn);
}

//------------------------------------------------------------------------------
// PUBLIC METHODS
//------------------------------------------------------------------------------

wl_status_t JustWifi::getStatus() {
    return WiFi.status();
}

const String& JustWifi::getAPSSID() {
    return _softap.ssid;
}

bool JustWifi::connected() {
    return (WiFi.status() == WL_CONNECTED);
}

bool JustWifi::connectable() {
    return _ap_connected;
}

void JustWifi::disconnect() {
    _timeout = 0;
    _currentID = 0;
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

void JustWifi::setPeriodicScanInterval(unsigned long interval) {
    _scan_periodic_interval = interval;
}

void JustWifi::setRSSIThreshold(int8_t rssi) {
    _scan_rssi_threshold = rssi;
}

void JustWifi::loop() {
    _machine();
}

JustWifi jw;
