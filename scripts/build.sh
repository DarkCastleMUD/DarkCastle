#!/bin/bash
if [[ "$1" =~ rel.* ]]; then
    cmake --workflow --fresh --preset release
    find build/release/ -name \*rpm -exec rpm --addsign {} \;
else
    cmake --workflow --fresh --preset debug
fi
