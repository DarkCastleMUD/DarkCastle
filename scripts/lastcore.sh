#!/bin/sh
CORE=`ls ../lib/[0-9]*_core.[0-9]* | tail -n 1`
PID=`echo $CORE | awk -Fcore. '{print $2}'`

if [ -e research1.$PID ];
then
    FILENAME=research1.$PID
fi

if [ -e research.debug.$PID ];
then
    FILENAME=research.debug.$PID
fi

echo "Debugging $FILENAME with corefile $CORE"
gdb $FILENAME $CORE