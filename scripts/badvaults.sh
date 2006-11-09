#!/bin/sh

VAULTDIR=/home/dcastle/dcastle/vaults
PLAYERDIR=/home/dcastle/dcastle/save
DELETEDIR=/home/dcastle/dcastle/tobedeleted

mkdir -p $DELETEDIR

cd $VAULTDIR
for v in `find . -type f -name '*\.vault' ! -name 'Clan[0-9]*'`;
do
  PLAYER=`echo $v | awk -F'.vault' '{print $1}'`;
  if [ ! -e $PLAYERDIR/$PLAYER ];
  then
      echo "Moving missing player `basename $PLAYER`'s vault to $DELETEDIR"
      mv $VAULTDIR/$PLAYER* $DELETEDIR
  fi
done
