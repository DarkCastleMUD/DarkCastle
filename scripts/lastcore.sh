#!/bin/sh
CORE=`ls ../lib/[0-9]*_core.[0-9]* | tail -n 1`
PID=`echo $CORE | awk -Fcore. '{print $2}'`

if [ -e dcastle.$PID ];
then
    FILENAME=dcastle.$PID
fi

if [ -e dcastle.debug.$PID ];
then
    FILENAME=dcastle.debug.$PID
fi

echo gdb $FILENAME $CORE
gdb $FILENAME $CORE
