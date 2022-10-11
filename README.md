# Dark Castle MUD

[Dark Castle](http://www.dcastle.org/) is a [MUD](https://en.wikipedia.org/wiki/MUD) that has been running (almost) continually since the early 90s.  Branched from an early version of [DIKU](https://en.wikipedia.org/wiki/DikuMUD) the code has been rewritten multiple times by many authors over the years.  It has been a source of enjoyment and rivalry for many over the years.  In 2020 it was decided to open-source it for the enjoyment of our players and for some nostalgia for those that return for a visit.

The game is available to login via telnet dcastle.org over ports 23, 6969 or 8080 for the standard no-botting/no-multiplaying server or dcastle.org 6666 for the botting/multiplaying server.

## Local development

Follow the instructions below to build and run DarkCastle MUD locally. Traditionally, Dark Castle is developed on openSUSE Tumbleweed or Leap. However, the following steps can be followed by anyone with native Ubuntu or Ubuntu within Windows 10 WSL2.

Install dependencies for compilation.

openSUSE Tumbleweed
```
  sudo zypper -n in git fmt-devel libfmt8 gcc-c++ libpq5 zlib-devel cmake qt6-base-devel postgresql-devel
```

Ubuntu 22.04
```
  sudo apt update
  sudo apt install g++-12-multilib g++-12 libstdc++-12-dev g++
  sudo apt install scons libcurl4
  sudo apt install libpq-dev libpq5 libcurl4-openssl-dev
  sudo apt install unzip zlib1g-dev
  sudo apt install libfmt-dev cmake qt6-base-dev
```

Now build the DarkCastle project. Change -j # option below to match the number of threads your CPU(s) can run in parallel.

```
git clone https://github.com/DarkCastleMUD/DarkCastle.git
cd DarkCastle
cmake -S src -B build
make -C build -j 128
```

Run DarkCastle server

```
cd lib
../build/dcastle
```

Test server by connecting to it from another terminal

```
telnet localhost 4000
```
