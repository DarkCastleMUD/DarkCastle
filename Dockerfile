# Run docker-compose build or docker-compose up to build/update the darkcastle image
FROM opensuse/tumbleweed

#RUN zypper mr -a -e -f
RUN zypper -n in fmt-devel libfmt9 gcc-c++ gcc14-c++ libpq5 zlib-devel cmake postgresql-devel \
    qt6-sql-devel qt6-dbus-devel qt6-httpserver-devel qt6-concurrent-devel libssh-devel git rpmbuild clang17
RUN zypper -n dup --auto-agree-with-licenses

RUN mkdir -p /srv/dcastle2/git
WORKDIR /srv/dcastle2/git
RUN git clone https://github.com/DarkCastleMUD/DarkCastle.git

WORKDIR /srv/dcastle2/git/DarkCastle
RUN cmake -S src -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -GNinja
RUN ninja -C build package
RUN zypper -n in --allow-unsigned-rpm build/dc*rpm

WORKDIR /srv/dcastle2/git/DarkCastle/lib
CMD ["/srv/dcastle2/git/DarkCastle/build/dcastle", "-P"]

LABEL Name=darkcastle Version=0.0.4
