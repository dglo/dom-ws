#!/bin/sh

#
# bldproject.sh, indiscriminantly build an entire project...
#

#
# use every jar file we know about...
#
CPLIB=`lessecho ../lib/*.jar | sed 's/^/..\//1' | sed 's/ /:..\//g'`
CPTOOLS=`lessecho ../tools/lib/*.jar | sed 's/^/..\//1' | sed 's/ /:..\//g'`
CP=.:${CPLIB}:${CPTOOLS}

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

if ! (cd ../$1/src; javac -classpath ${CP} @all.src ); then
    echo "bldproject.sh: unable to compile java files"
    exit 1
fi

make -f jardep.mk
