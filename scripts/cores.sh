#!/bin/bash
cd /home/dcastle/dcastle/cores/

for c in /home/dcastle/dcastle/lib/[0-9]*core.????;
do
  ln "$c"
  chown dcastle:dcastle_cvs "$c"
  chmod 0640 "$c"
done
