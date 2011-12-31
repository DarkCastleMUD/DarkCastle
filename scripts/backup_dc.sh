#!/bin/bash
DATE=$(date +%Y-%m-%d-%H-%M-%S)
tar cjvf /srv/backup/dcastle/dc-$DATE.tar.bz2 -C /home/dcastle/ --exclude=dcastle/save.backup/* --exclude=dcastle/log/*.log --exclude=dcastle/log/archives/* --exclude=dcastle/log/player/* --exclude=dcastle/lib/*_core.* --exclude=dcastle/cores/* --exclude=dcastle/lib/*/*.last --exclude=dcastle/lib/*/*.backup --exclude=dcastle/docs/html/* --exclude=dcastle/save/?/*.backup --exclude=dcastle/src/*.o --exclude=dcastle/src/*/*.o --exclude=dcastle/src/research* dcastle/ &>/srv/backup/dcastle/dc-$DATE.log

