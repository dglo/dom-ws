#!/bin/sh

#
# bldproject.sh, indiscriminantly build an entire project...
#

#
# make sure build directory exists...
#
if [[ ! -d ../build/$1 ]]; then
	mkdir -p ../build/$1
fi

#
# use every jar file we know about...
#
CPLIB=`lessecho ../build/* | sed 's/^/..\//1' | sed 's/ /:..\//g'`
CPTOOLS=`lessecho ../tools/lib/*.jar | sed 's/^/..\//1' | sed 's/ /:..\//g'`
CP=.:${CPLIB}:${CPTOOLS}

echo classpath=${CP}

if (( $# != 1 )); then
    echo "usage: bldproject.sh project"
    exit 1
fi

if [[ ! -d ../$1/src ]]; then
    echo "bldproject.sh: can't seem to find project source"
    exit 1
fi

if ! (cd ../$1/src; find . -name '*.java' -print > all.src ); then
    echo "bldproject.sh: unable to find java files"
    exit 1
fi

if ! (cd ../$1/src; javac -d ../../build/$1 -classpath ${CP} @all.src ); then
    echo "bldproject.sh: unable to compile java files"
    exit 1
fi

if ! (cd ../build/$1; jar cf ../../lib/$1.jar `lessecho *`); then
    echo "bldproject.sh: unable to create jar file $1.jar"
    exit 1
fi

make -f jardep.mk

