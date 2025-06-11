#!/bin/bash
NINJA_BUILD="$(ninja -n -C build)"
echo "$NINJA_BUILD"
if [[ ! "$NINJA_BUILD_STATE" =~ "ninja: no work to do." ]]; then
    if ninja -C build; then
        #build/testDC
        if pidof dcastle; then
            rebootdc.sh
        fi
    fi
fi
true