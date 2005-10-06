#!/bin/bash

# for the project given, print the lib, tools, test-lib and test-tools...
if (( $# != 1 )); then
	echo "usage: `basename $0` project"
	exit 1
fi

if [[ ! -d ../$1 ]]; then
	echo "`basename $0`: unable to find project directory $1"
	exit 1
fi

xmln ../$1/project.xml | awk -f project.awk | tr -d ' ' | \
	sed 's/\\n//g' | sed 's/\\t//g' | \
	tr '\t' ' ' | awk '{ if (NF>1) print $0; }'
