#!/bin/bash

if [[ $# > 0 && $1 == "-v" ]]; then verbose=1; shift; else verbose=0; fi

#
# bldpkg.sh, build a java package.  java
# is incredibly slow to startup, so
# we'll make a list of .java files
# to compile and give them all at once
# to javac...
#
while [[ $# > 0 ]]; do
    echo "building: $1"

    #
    # get the project for this package
    #
    ap=`printf "%c2 ~ /^$1%c/ { print %c1; }" '$' '$' '$'`
    jar=`awk "${ap}" jardep.deps/all.dep`

    #
    # maybe more than one jar file for a package?
    #
    if echo ${jar} | grep -q ' '; then
	echo "bldpkg.sh: multiple jars for package: $1: ${jar}"
	exit 1
    fi
    project=`basename ${jar} | sed 's/.jar$//1'`

    #
    # get classpath, first the tools as jars, then the projects as paths...
    #
    tcp=`./getjars.sh $1 | grep '^../tools/lib' | sort | uniq | \
	awk '{ print "../" $1; }'`
    pcp=`./getjars.sh $1 | grep '^../lib' | sort | uniq | \
	sed 's/.jar$//1' | sed 's/\.\.\/lib\//\.\.\/\.\.\/build\//1'`
    cp=`echo ${tcp} ${pcp} | tr ' ' ':' | sed 's/:$//1'`

    #
    # get the src and bld paths...
    #
    pkgpath=`echo $1 | tr '.' '/'`
    srcpath=../${project}/src/${pkgpath}
    bldpath=../build/${project}/${pkgpath}

    #
    # make sure the build path exists...
    #
    if [[ ! -d ${bldpath} ]]; then
	echo "creating ${bldpath}... "
	if ! mkdir -p ${bldpath}; then
	    echo "bldpkg.sh: error: can not create ${bldpath}"
	    exit 1
	fi
    fi

    #
    # make sure the src path exists...
    #
    if [[ ! -d ${srcpath} ]]; then
	echo "bldpkg.sh: error ${srcpath} does not exist"
	exit 1
    fi

    #
    # get the list of java files in the pkg...
    #
    java=`cd ${srcpath}; lessecho *.java`

    rm -f /tmp/bldpkg.$$.files
    touch /tmp/bldpkg.$$.files
    for j in ${java}; do
	class=`echo ${j} | sed 's/\.java$/\.class/1'`
	bldclass=${bldpath}/${class}

	#
	# if the build file does not exist, or if the
	# build file is older than the java file, add
	# it to the list...
	#
	if [[ ${verbose} == 1 ]]; then
	    if [[ ! -f ${bldclass} ]]; then 
		echo "${srcpath}/${j}: ${bldclass} does not exist"
	    fi
	    if [[ ${srcpath}/${j} -nt ${bldclass} ]]; then 
		echo "${srcpath}/${j}: ${bldclass} is out of date"
	    fi
	fi

	if [[ ! -f ${bldclass} || ${srcpath}/${j} -nt ${bldclass} ]]; then
	    echo ${pkgpath}/${j} >> /tmp/bldpkg.$$.files
	fi
    done

    nfiles=`cat /tmp/bldpkg.$$.files | wc -l`
    if (( ${nfiles} > 0 )); then
	echo "compiling: $1"
	echo "classpath: ${cp}"
	#
	# the . allows within project circular dependencies...
	#
	ncp=.:${cp}
	if ! (cd ../${project}/src; \
	 javac  -d ../../build/${project} -classpath ${ncp} @/tmp/bldpkg.$$.files ); then
	    echo "$1 failed"
	    exit 1
	fi
    fi

    # clean up build file
    rm -f /tmp/bldpkg.$$.files

    shift
done
