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
for p in ${projects}; do
    if [[ ! -d ../${p} ]]; then
	echo "ckprod.sh: ../${p} does not exist"
	exit 1
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

#
# build jar files...
#
for p in ${projects}; do
    found=0
    j=../lib/${p}.jar
    if [[ ! -f ${j} ]]; then 
	found=1; 
    else
	nlines=`find ../build/${p} -name '*.class' -newer ${j} -print | \
	    wc -l`

	if (( ${nlines} > 0 )); then found=1; fi
    fi

    if [[ ${found} == 1 ]]; then
	# rebuild jar file...
	echo "jarring: ${p}"
	(cd ../build/${p}; jar cf ../../lib/${p}.jar icecube)

	# update dependencies...
	make -f jardep.mk
    fi
done








