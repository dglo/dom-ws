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
scp prod-${rev}-${num}.tar.gz getprod.sh glacier.lbl.gov:public_html/prod
scp prod-${rev}-${num}.tar.gz \
  glacier.lbl.gov:/home/icecube/system/httpd/htdocs/releases/DOM-MB/stable_hex/
ssh glacier.lbl.gov \
 "cd public_html/prod; rm -f prod-latest.tar.gz;\
 ln -s prod-${rev}-${num}.tar.gz prod-latest.tar.gz"

ssh glacier.lbl.gov \
 "cd /home/icecube/system/httpd/htdocs/releases/DOM-MB/stable_hex; \
  rm -f prod-latest.tar.gz; \
  ln -s prod-${rev}-${num}.tar.gz prod-latest.tar.gz"

exec ./tagprod.sh

