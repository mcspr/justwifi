#!/bin/bash

set -x -e -v

for board in d1_mini ; do
    echo "- Building for $board"
    env PLATFORMIO_CI_SRC=examples/basic/ \
        pio ci --board=$board --lib="."
    env PLATFORMIO_CI_SRC=examples/advanced \
        pio ci --board=$board --lib="."
    env PLATFORMIO_CI_SRC=examples/ap \
        pio ci --board=$board --lib="."
    env PLATFORMIO_CI_SRC=examples/smartconfig PLATFORMIO_BUILD_FLAGS='-DJUSTWIFI_ENABLE_SMARTCONFIG' \
        pio ci --board=$board --lib="."
    env PLATFORMIO_CI_SRC=examples/wps PLATFORMIO_BUILD_FLAGS='-DJUSTWIFI_ENABLE_WPS' \
        pio ci --board=$board --lib="."
done

