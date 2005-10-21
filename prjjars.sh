#!/bin/bash

#
# prjjars.sh, project jar file dependencies, may be duplicates,
# filter through sort | uniq before using...
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

deps=`./prjparse.sh ${project} | awk '$1 ~ /^lib$/ { print $2; }' | \
    sed 's/\.jar$//1'`

for dep in ${deps}; do
    $0 ${dep}
    echo "../lib/${dep}.jar"
done

echo "../lib/${project}.jar"

./prjparse.sh ${project} | \
    awk '$1 ~ /^tools$/ { print "/home/arthur/bfd-tools/tools/lib/" $2; }'
