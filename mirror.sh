#!/bin/bash
TIME=`date +%b-%d-%y`
FILENAME=backup-$TIME.tar.gz
SOURCE=/srv/dcastle2/lib
DESTINATION=/srv/dcastle/lib

tar cfz "$FILENAME" --exclude=playershop* --exclude=*02300* --exclude=*02399* $DESTINATION/mobs $DESTINATION/zonefiles $DESTINATION/world $DESTINATION/objects $DESTINATION/*index 
shopt -s extglob

cp $SOURCE/mobs/!(*clan*|*03100*|*03000*|*05000*|*17800*|*3200*|*11300*|*18900*|*0600*|*22701*|*00401*).mob $DESTINATION/mobs/
cp $SOURCE/objects/!(*clan*|*03100*|*03000*|*05000*|*17800*|*3200*|*11300*|*18900*|*0600*|*22701*|*00401*).obj $DESTINATION/objects/
cp $SOURCE/zonefiles/!(*clan*|*03100*|*03000*|*05000*|*17800*|*3200*|*11300*|*18900*|*0600*|*22701*|*00401*).zon $DESTINATION/zonefiles/
cp $SOURCE/world/!(*clan*|*03100*|*03001*|*05000*|*17800*|*3200*|*11300*|*18900*|*0600*|*22700*|*00400*).txt $DESTINATION/world/
cp $SOURCE/*index $DESTINATION
