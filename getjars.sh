#!/bin/bash

#
# get all the jars that are required for
# a package...
#

#
# check usage
#
if [[ $# < 1 ]]; then
    echo "usage: getjars.sh package ..."
    exit 1
fi

while [[ $# > 0 ]]; do
    #
    # get the list of dependent packages...
    #
    grep "^$1 " jardep.deps/all.jardep | sed 's/[^ ]* //1' | tr ' ' '\n'
    shift
done


