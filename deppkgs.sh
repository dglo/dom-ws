#!/bin/bash

#
# deppkgs.sh, find dependent packages from packages given on command line...
#

#
# required packages...
#
pkgs="$*"

#
# get a list of jar files for packages that
# we can build...
#
jars=`./getjars.sh ${pkgs} | sort | uniq | grep '^../lib/'`
projects=`echo ${jars} | sed 's/..\/lib\///g' | sed 's/\.jar[ ]/ /g' |\
 sed 's/\.jar$//1'`

#
# make sure projects exist...
#
if (( ${#projects} == 0 )); then
    echo "deppkgs.sh: can not find projects"
    exit 1
fi

for p in ${projects}; do
    if [[ ! -d ../${p} ]]; then
	echo "deppkgs.sh: ../${p} does not exist"
	exit 1
    fi
done

#
# topologically sort packages...
#
./pairpkgs.sh ${pkgs} | tsort | sed '1s/^.*$//1i' | tr '\n' ' ' | \
	sed 's/^[ ]*//1'

