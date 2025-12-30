#!/bin/bash
NINJA_BUILD="$(cmake --build --preset debug)"
echo "$NINJA_BUILD"
if [[ ! "$NINJA_BUILD_STATE" =~ "ninja: no work to do." ]]; then
    if cmake --build --preset debug; then
        #build/testDC
        if pidof dcastle; then
            rebootdc.sh
        fi
    fi
fi
true