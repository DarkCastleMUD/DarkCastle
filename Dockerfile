# Run docker-compose build or docker-compose up to build/update the darkcastle image
FROM opensuse/tumbleweed

#RUN zypper mr -a -e -f
RUN zypper -n in git fmt-devel libfmt8 gcc-c++ libpq5 zlib-devel cmake qt6-base-devel postgresql-devel

RUN mkdir -p /srv/dcastle2/git
WORKDIR /srv/dcastle2/git
RUN git clone https://github.com/DarkCastleMUD/DarkCastle.git

WORKDIR /srv/dcastle2/git/DarkCastle
RUN cmake -S src -B build
RUN make -C build -j 128

WORKDIR /srv/dcastle.6969/lib
CMD ["/srv/dcastle2/git/DarkCastle/build/dcastle", "-P"]

LABEL Name=darkcastle Version=0.0.1
