#!/bin/bash

#
# prjbld1.sh, build a single java project...
#
if (( $# != 1 )); then
    echo "usage: `basename $0` project"
    exit 1
fi

project=$1

#
# make sure build directory exists...
#
if ! mkdir -p ../build/${project}; then
    echo "`basename $0`: unable to create build directory"
    exit 1
fi

#
# get classpath
#
PRJCP=`./prjparse.sh ${project} | awk '$1 ~ /^lib$/ { print $2; }' | \
    sed 's/\.jar//1' | awk '{ print "../../build/" $0; }' | \
    tr '\n' ':' | sed 's/:$//1'`
TOOLCP=`./prjparse.sh ${project} | awk '$1 ~ /^tools$/ { print $2; }' | \
    awk '{ print "/home/arthur/bfd-tools/tools/lib/" $0; }' | \
    tr '\n' ':' | sed 's/:$//1'`
CP=../../build/${project}:${PRJCP}:${TOOLCP}

#
# get packages to build
#
pkgs=`./prjparse.sh ${project} | awk '$1 ~ /^package$/ { print $2; }' | \
    tr '.' '/' | awk -vp=${project} '{ print "./" $0; }' | \
    tr '\n' ' ' | sed 's/ $//1'`


allsrc=`mktemp`

if ! (cd ../$1/src; \
    find ${pkgs} -name '*.java' -maxdepth 1 -print > ${allsrc}); then
    echo "`basename $0`: unable to find java files"
    exit 1
fi

#
# FIXME: remove lines from the src file if:
# the class file exists and it is newer than the
# src file...
#
# nothing to do?
#if (( `wc -l ${allsrc}` == 0 )); then
#    exit 0
#fi

#
# build them
#
if ! (cd ../$1/src; javac -d ../../build/$1 -classpath ${CP} @${allsrc} ); then
    echo "`basename $0`: unable to compile java files"
    exit 1
fi

#
# jar them...
#
if ! (cd ../build/$1; jar cf ../../lib/$1.jar `lessecho *`); then
    echo "`basename $0`: unable to create jar file $1.jar"
    exit 1
fi

rm -f ${allsrc}
