#!/bin/sh
CORE=`ls ../lib/[0-9]*_core.[0-9]* | tail -n 1`
PID=`echo $CORE | awk -Fcore. '{print $2}'`
echo "Debugging research1.$PID with corefile $CORE"
gdb research1.$PID $CORE