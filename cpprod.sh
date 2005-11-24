#!/bin/bash

#
# set revision...
#
rev=REV5

#
# get latest production run
#
num=`cat prod.num`

#
# cp files and setup new link...
#
scp ChangeLog prod-${rev}-${num}.tar.gz \
  glacier.lbl.gov:/var/www/html/releases/DOM-MB/stable_hex

ssh glacier.lbl.gov \
 "cd /var/www/html/releases/DOM-MB/stable_hex; \
  rm -f prod-latest.tar.gz; \
  ln -s prod-${rev}-${num}.tar.gz prod-latest.tar.gz"

exec ./tagprod.sh
