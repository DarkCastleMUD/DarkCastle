# Dark Castle MUD

[Dark Castle](http://www.dcastle.org/) is a [MUD](https://en.wikipedia.org/wiki/MUD) that has been in existence since the early 90s.  Branched from an early version of [DIKU](https://en.wikipedia.org/wiki/DikuMUD) the code has been rewritten multiple times by many authors over the years.  It has been a source of enjoyment and rivalry for many over the years.  In 2020 it was decided to open-source it for the enjoyment of our players and for some nostalgia for those that return for a visit.

The game is available to login via telnet dcastle.org over ports 23 or 6969 for the standard no-botting/no-multiplaying server or dcastle.org 6666 for the botting/multiplaying server.

## Use existing container image with Podman or Docker
```
localhost:~ > podman run -d -p 4000:4000 --name darkcastle docker.io/jhhudso/darkcastle:latest
Trying to pull docker.io/jhhudso/darkcastle:latest...
Getting image source signatures
Copying blob 4f4fb700ef54 done   | 
Copying blob 743cbdefc427 done   | 
Copying blob d02f6ccf6f02 done   | 
Copying blob 4f4fb700ef54 done   | 
Copying blob 8a98d0457244 done   | 
Copying blob 73a452477877 done   | 
Copying blob 780e35cb8324 done   | 
Copying blob 17df0f60aa11 done   | 
Copying blob 4f4fb700ef54 skipped: already exists  
Copying config 6713af0e16 done   | 
Writing manifest to image destination
a01460cd48638e867970e302ef36c24a6e66d0aff21339d0f35c07209d5f2a70
```
```
localhost:~ # docker pull docker.io/jhhudso/darkcastle:latest
latest: Pulling from jhhudso/darkcastle
743cbdefc427: Already exists 
73a452477877: Already exists 
d02f6ccf6f02: Already exists 
4f4fb700ef54: Already exists 
8a98d0457244: Already exists 
780e35cb8324: Already exists 
17df0f60aa11: Already exists 
Digest: sha256:fe80408a8ab8559b61fd6cd2aac411bd95ea93caedb170d96d662205b91454eb
Status: Downloaded newer image for jhhudso/darkcastle:latest
docker.io/jhhudso/darkcastle:latest
~ # docker run -d -p 4000:4000 --name darkcastle docker.io/jhhudso/darkcastle:latest
f47f42e34429e62e7f3b556c5f493d60670dc10887b3f34634dc2bbedec4cde4
```
```
localhost:~ > telnet localhost 4000
Trying ::1...
Connected to localhost.
Escape character is '^]'.

What name for the roster? 
```


## Local development

Follow the instructions below to build and run DarkCastle MUD locally. Traditionally, Dark Castle is developed on openSUSE Tumbleweed or Leap. I've also included instructions for Ubuntu 22.04 which can also be used within Windows 10 WSL2.

Install dependencies for compilation.

openSUSE Tumbleweed
```
sudo zypper ref
sudo zypper in git fmt-devel libfmt8 gcc-c++ libpq5 zlib-devel cmake qt6-base-devel postgresql-devel qt6-httpserver-devel libssh-devel
```

openSUSE Leap 15.5
```
sudo zypper ref
sudo zypper in git fmt-devel libfmt8 gcc-c++ libpq5 zlib-devel cmake qt6-base-devel postgresql-devel qt6-httpserver-devel libstdc++6-devel-gcc12 gcc12-c++ libssh-devel
```

Ubuntu 22.04
```
sudo apt update
sudo apt install g++-12-multilib g++-12 libstdc++-12-dev g++ scons libcurl4 libpq-dev libpq5 libcurl4-openssl-dev unzip zlib1g-dev libfmt-dev cmake qt6-base-dev
```

Now build the DarkCastle project. Change -j64 option below to match the number of threads your CPU(s) can run in parallel.

```
git clone https://github.com/DarkCastleMUD/DarkCastle.git
cd DarkCastle
# The c++ compiler should be g++-12 or newer
cmake -S src -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_COMPILER=g++-12
or 
cmake -S src -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=g++-12


make -C build clean
make -C build -j64
or
make -C build -j64 VERBOSE=1
```

Run DarkCastle server

```
cd lib
../build/dcastle
```

Test server by connecting to it from another terminal.

```
telnet localhost 4000
```
