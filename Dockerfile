# Run docker-compose build or docker-compose up to build/update the darkcastle image
FROM opensuse/tumbleweed

RUN zypper -n dup --no-recommends --auto-agree-with-licenses
RUN zypper -n in --no-recommends glibc-locale glibc-locale-base \
    gcc14-c++ clang18 libpq5 fmt-devel libfmt9 zlib-devel cmake postgresql-devel qt6-sql-devel \
    qt6-httpserver-devel qt6-concurrent-devel libssh-devel git libQt6Test6 qt6-test-devel mold

RUN mkdir -p /srv/dcastle/git
WORKDIR /srv/dcastle/git
RUN git clone --single-branch --branch stable https://github.com/DarkCastleMUD/DarkCastle.git

WORKDIR /srv/dcastle/git/DarkCastle
RUN cmake -S src -B build -GNinja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_CXX_COMPILER="/usr/bin/g++-15" \
    -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=mold" \
    -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=mold"
RUN ninja -C build package
RUN zypper -n in --allow-unsigned-rpm build/dc*rpm

WORKDIR /srv/dcastle/git/DarkCastle/lib
CMD ["/usr/bin/dcastle", "-P"]

LABEL Name=darkcastle Version=0.0.5
