#!/bin/bash

#
# bldpkgs.sh, build packages given on command line
# and build all dependent packages as well...
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
    echo "bldpkgs.sh: can not find projects"
    exit 1
fi

for p in ${projects}; do
    if [[ ! -d ../${p} ]]; then
	echo "bldpkgs.sh: ../${p} does not exist"
	exit 1
    fi
done

#
# topologically sort then build packages...
#
#bldpkgs=`./pairpkgs.sh ${pkgs} | tsort | grep '^icecube'`
bldpkgs=`./pairpkgs.sh ${pkgs} | tsort | sed '1s/^.*$//1i'`

if ! ./bldpkg.sh ${bldpkgs}; then
    echo "bldpkgs.sh: unable to build, exiting..."
    exit 1
fi

