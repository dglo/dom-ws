#!/bin/bash

#
# jbld.sh, build a java project...
#
if (( $# != 1 )); then
    echo "usage: `basename $0` project"
    exit 1
fi

project=$1

#
# make sure build directory exists...
#
if [[ ! -d ../build/${project} ]]; then
	mkdir -p ../build/${project}
fi

#
# build dependent projects...
#
deps=`./prjdeps.sh ${project} | tsort`
for dep in ${deps}; do
    echo "building: ${dep}..."
    ./prjbld1.sh ${dep}
done
