# JustWifi

**Notice** this is the fork of the original [justwifi](https://github.com/xoseperez/justwifi) by **[@xoseperez](https://github.com/xoseperez)**

JustWifi is a WIFI Manager library for the [Arduino Core for ESP8266][2]. The goal of the library is to manage ONLY the WIFI connection (no webserver, no mDNS,...) from code and in a reliable and flexible way.

[![version](https://img.shields.io/github/v/tag/mcspr/justwifi)](CHANGELOG.md)
[![CI](https://github.com/mcspr/justwifi/workflows/PlatformIO%20CI/badge.svg?branch=master)](https://github.com/mcspr/justwifi/actions?query=workflow%3A%22PlatformIO+CI%22)
[![license](https://img.shields.io/github/license/xoseperez/justwifi.svg)](LICENSE)
<br />

## Features

The main features of the JustWifi library are:

* Configure multiple possible networks
* Scan wifi networks so it can try to connect to only those available, in order of signal strength
* Smart Config support (when built with -DJUSTWIFI_ENABLE_SMARTCONFIG, tested with ESP8266 SmartConfig or IoT SmartConfig apps)
* WPS support (when built with -DJUSTWIFI_ENABLE_WPS)
* Fallback to AP mode
* Configurable timeout to try to reconnect after AP fallback
* AP+STA mode
* Static IP (autoconnect is disabled when using static IP)
* Single debug/action callback

## Usage

See examples.

## License

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
