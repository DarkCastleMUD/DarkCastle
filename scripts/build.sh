#!/bin/bash
rm -rf build
if [[ "$1" =~ prod.* ]]; then
    echo production build
    cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo \
                   -DCMAKE_CXX_COMPILER="/usr/bin/g++-15" \
                   -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=mold" \
                   -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=mold" \
                   -GNinja
    ninja -C build package
    find build -name \*rpm -exec rpm --addsign {} \;
else
    echo debug build
    cmake -B build -DCMAKE_BUILD_TYPE=Debug \
                   -DCMAKE_CXX_COMPILER="/usr/bin/g++-15" \
                   -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=mold" \
                   -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=mold" \
                   -GNinja
    ninja -C build
fi
