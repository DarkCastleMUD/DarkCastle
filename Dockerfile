# Run docker-compose build or docker-compose up to build/update the darkcastle image
FROM opensuse/tumbleweed

RUN zypper mr -a -e -f
RUN zypper -n rm patterns-base-fips
RUN zypper -n install git scons fmt-devel libfmt8 libfmt8-32bit libfmt8-32bit-debuginfo libfmt8-debuginfo gcc-c++-32bit gcc-c++ libpq5-32bit libpq5 postgresql14-server libcrypt1-32bit zlib-devel-32bit
RUN mkdir -p /srv/dcastle2/git
WORKDIR /srv/dcastle2/git
RUN git clone https://github.com/DarkCastleMUD/DarkCastle.git
WORKDIR /srv/dcastle2/git/DarkCastle/src
RUN ln -s /usr/lib/libcrypt.so.1 /usr/lib/libcrypt.so
RUN ln -s /usr/lib/libfmt.so.8 /usr/lib/libfmt.so
RUN ln -s /usr/lib/libpq.so.5 /usr/lib/libpq.so

RUN scons -c
RUN scons -j128

WORKDIR /srv/dcastle.6969/lib

CMD ["/srv/dcastle2/git/DarkCastle/src/dcastle", "-p6969", "-P"]

LABEL Name=darkcastle Version=0.0.1
