#!/bin/bash
umask 0077
grep $1\@ ~dcastle/dcastle/log/socket.log | grep connected | awk '{print $7}' | awk -F\@ '{print $2}' | sort | uniq > /tmp/$1.part1
(for d in `cat /tmp/$1.part1`; do grep $d socket.log ; done;) | grep connected | awk '{print $7}' | awk -F\@ '{print $1}' | sort | uniq
rm /tmp/$1.part1