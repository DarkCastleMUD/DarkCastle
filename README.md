# Dark Castle MUD

[Dark Castle](http://www.dcastle.org/) is a [MUD](https://en.wikipedia.org/wiki/MUD) that has been running (almost) continually since the early 90s.  Branched from an early version of [DIKU](https://en.wikipedia.org/wiki/DikuMUD) the code has been rewritten multiple times by many authors over the years.  It has been a source of enjoyment and rivalry for many over the years.  In 2020 it was decided to open-source it for the enjoyment of our players and for some nostalgia for those that return for a visit.

The game is available to login via telnet dcastle.org over ports 23, 6969 or 8080 for the standard no-botting/no-multiplaying server or dcastle.org 6666 for the botting/multiplaying server.

## Local development

Follow the instructions below to build and run DarkCastle MUD locally. Traditionally, Dark Castle is developed on openSUSE Tumbleweed or Leap. However, the following steps can be followed by anyone with native Ubuntu or Ubuntu within Windows 10 WSL2.

From Ubuntu terminal

```
git clone https://github.com/DarkCastleMUD/DarkCastle.git
cd DarkCastle
```

Now we follow most of the steps listed in .github/workflows/ccpp.yml

```
  sudo dpkg --add-architecture i386
  sudo apt update
  sudo apt install gcc-10-multilib g++-10-multilib gcc-multilib
  sudo apt install g++-multilib scons libcurl4:i386
  sudo apt install libpq-dev:i386 libpq5:i386 libcurl4-openssl-dev:i386
  sudo apt install unzip zlib1g-dev:i386
  sudo apt install libfmt-dev:i386
```

Install cmake and gdb

```
sudo apt install cmake gdb
```

Need to set FLAGS to build 32 bit fmt lib

```
export CFLAGS=" -m32"
export CXXFLAGS=" -m32"
```

Install fmt lib

```
git clone https://github.com/fmtlib/fmt.git
cd fmt
cmake .
make -j4
sudo make install
```

Now build the DarkCastle project

```
cd ../src
scons -j4
```

Run DarkCastle server

```
cd ../lib
../src/dcastle -p6969
```

Test server by connecting to it from another terminal

```
telnet localhost 6969
```
