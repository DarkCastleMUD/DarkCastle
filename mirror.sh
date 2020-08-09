#!/bin/bash
TIME=`date +%Y%m%d`
FILENAME=dcastle_world_$TIME.tar.xz
FILENAME2=dcastle2_world_$TIME.tar.xz
SOURCE=/srv/dcastle2/lib
DESTINATION=/srv/dcastle/lib

cd $DESTINATION
tar cfJ "$FILENAME" --exclude=playershop* --exclude=*clan* --exclude=*03100* --exclude=*03000* --exclude=*05000* --exclude=*17800* --exclude=*03200* --exclude=*11300* --exclude=*18900* --exclude=*00600* --exclude=*22701* --exclude=*00401* --exclude=*00400* --exclude=*22700* mobs zonefiles world objects ./*index

cd $SOURCE
tar cfJ "$FILENAME2" --exclude=playershop* --exclude=*clan* --exclude=*03100* --exclude=*03000* --exclude=*05000* --exclude=*17800* --exclude=*03200* --exclude=*11300* --exclude=*18900* --exclude=*00600* --exclude=*22701* --exclude=*00401* --exclude=*00400* --exclude=*22700* mobs zonefiles world objects ./*index

cd $DESTINATION
cp "$SOURCE/$FILENAME2" .
sudo -u dcastle tar xJf "$FILENAME2"
