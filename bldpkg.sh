#!/bin/bash

projects=""

#
# bldpkg.sh, build a java package...
#
while [[ $# > 0 ]]; do

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
    projects="$projects $project"

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
    echo "building $1 [${project}]"
    if ! (cd ../${project}/src; make -f ../../dom-ws/pkgmf.tmpl \
	  PACKAGE=$1 ${target:=all}); then
	echo "bldpkg: unable to make ${target}"
	exit 1
    fi

    shift
done

#
# now that everything is built, update the
# dependencies...
#
anyjar=0
np=`echo ${projects} | tr ' ' '\n' | sort | uniq`
for p in $np; do
	#
	# check for newer files, if there are any, rebuild
	# the jar file...
	#
	if [[ -f ../lib/${p}.jar ]]; then
		rm -f ../build/${p}/jar.touch
		find ../build/${p} -type f -newer ../lib/${p}.jar \
			-exec touch ../build/${p}/jar.touch \;
	else
		touch ../build/${p}/jar.touch
	fi

	if [[ -f ../build/${p}/jar.touch ]]; then
		echo "bldpkg: creating jar file: $p"
		(cd ../build/${p}; jar cf ../../lib/${p}.jar .)
		anyjar=1
	fi

	rm -f ../build/${p}/jar.touch	
done

# any jars updated, if so, make deps...
if [[ ${anyjar} == "1" ]]; then
	make -f jardep.mk
fi

