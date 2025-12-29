#!/bin/bash
NINJA_BUILD="$(ninja -n -C build/default)"
echo "$NINJA_BUILD"
if [[ ! "$NINJA_BUILD_STATE" =~ "ninja: no work to do." ]]; then
    if ninja -C build/default; then
        #build/testDC
        if pidof dcastle; then
            rebootdc.sh
        fi
    fi
fi
true