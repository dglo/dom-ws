#!/bin/bash

#
# set revision...
#
rev=REV4

#
# get latest production run
#
num=`cat prod.num`

#
# cp files and setup new link...
#
scp prod-REV4-${num}.tar.gz glacier:public_html/prod
ssh glacier \
 "cd public_html/prod; rm -f prod-latest.tar.gz;\
 ln -s prod-REV4-${num}.tar.gz prod-latest.tar.gz"


