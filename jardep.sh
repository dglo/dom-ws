#!/bin/bash -f

#
# make a dependency package dependency file from a jar...
#
# the dependency file has the format:
# jarfile package dependent_package dependent_package ...
#

#
# check for our arg
#
if [[ $# != 1 ]]; then
    echo "usage: jardep.sh jarfile"
    exit 1
fi

#
# create working directory...
#
rm -rf /tmp/jardep.$$
mkdir /tmp/jardep.$$
cp $1 /tmp/jardep.$$/file.jar

#
# get dependency output
#
java jdepend.textui.JDepend /tmp/jardep.$$ | \
    awk -v jar=$1 -f ./jardep.awk

#
# clean up...
#
rm -rf /tmp/jardep.$$

