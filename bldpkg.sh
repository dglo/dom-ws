#!/bin/bash

#
# bldpkg.sh, build a java package...
#
while [[ $# > 0 ]]; do
    echo "building: $1"

    #
    # figure out the project for this package
    #
    ap=`printf "%c2 ~ /^$1%c/ { print %c1; }" '$' '$' '$'`
    jar=`awk "${ap}" jardep.deps/all.dep`

    #
    # maybe more than one jar file for a package?
    #
    if echo ${jar} | grep -q ' '; then
	echo "bldpkg.sh: multiple jars for package: $1:"
	echo "${jar}"
	exit 1
    fi

    #
    # maybe no jar file found!
    #
    if (( ${#jar} == 0 )); then
	echo "bldpkg.sh: can't find jar file for package: $1"
	echo "  you may need to compile this project by hand (bldproject.sh)"
	exit 1
    fi

    #
    # extract project name from jar file...
    #
    project=`basename ${jar} | sed 's/.jar$//1'`

    #
    # get the src path...
    #
    pkgpath=`echo $1 | tr '.' '/'`
    srcpath=../${project}/src/${pkgpath}

    #
    # make sure the src path exists...
    #
    if [[ ! -d ${srcpath} ]]; then
	echo "bldpkg.sh: error ${srcpath} does not exist"
	exit 1
    fi

    #
    # run make...
    #
    if ! (cd ../${project}/src; make -f ../../dom-ws/pkgmf.tmpl \
	  PACKAGE=$1 ${target:=all}); then
	echo "bldpkg: unable to make ${target}"
	exit 1
    fi

    shift
done









