#!/bin/bash

#
# ckprod.sh, check to make sure that all
# the packages required by stf are up-to-date
# in preparation for a mkprod.sh
#

#
# required packages...
#
pkgs="icecube.daq.stf icecube.daq.domhub"

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
#if (( ${#projects} == 0 )); then
#    echo "can not find projects"
#    exit 1
#fi

for p in ${projects}; do
    if [[ ! -d ../${p} ]]; then
	if ! ( cd ..; cvs checkout $p ); then
		echo "ckprod.sh: ../${p} does not exist"
		exit 1
	fi
    fi
done

#
# update projects
#
echo "updating: ${projects}..."
( cd ..; cvs -q update -d ${projects} | grep -v '^\?' )

#
# build packages...
#
bldpkgs=`./pairpkgs.sh ${pkgs} | tsort | grep '^icecube'`
if ! ./bldpkg.sh ${bldpkgs}; then
    echo "ckprod.sh: unable to build, exiting..."
    exit 1
fi

