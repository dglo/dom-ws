#!/bin/bash

#
# getprod.sh, get the latest production env...
#

#
# set url here...
#
url=http://glacier.lbl.gov/~arthur/prod/prod-latest.tar.gz

#
# get it...
#
if [[ -f prod-latest.tar.gz ]]; then
    echo "deleting existing prod-latest.tar.gz"
fi

rm -f prod-latest.tar.gz

wget -q ${url}

if [[ ! -f prod-latest.tar.gz ]]; then
    echo "yikes: prod-latest.tar.gz did not copy over, talk to arthur"
    exit 1
fi

#
# unzip it...
#
gzip -dc prod-latest.tar.gz | tar xfk -

#
# clean up, display files and exit...
#
rm -f prod-latest.tar.gz

ls -l | grep prod-
