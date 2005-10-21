#!/bin/bash

#
# prjdeps.sh, project dependencies, output in
# a tsort friendly manner...
#
if (( $# != 1 )); then
	echo "usage: `basename $0` project"
	exit 1
fi

project=$1

if [[ ! -d ../${project} ]]; then
	echo "`basename $0`: unable to find project ${project} directory"
	exit 1
fi

deps=`./prjparse.sh ${project} | egrep "^lib " | \
    awk	'{ print $2; }' | sed 's/\.jar//1'`

for dep in ${deps}; do
    $0 ${dep}
    echo "${dep} ${project}"
done
