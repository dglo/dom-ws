#!/bin/bash

#
# pairpkgs.sh, pair a list of java packages, suitable
# for input to tsort(1)
#
while [[ $# > 0 ]]; do
    echo "pairpkgs.sh: $1"

    #
    # find all dependent packages for this package ...
    #
    ap=`printf "%s ~ /^$1%c/ { for (i=3; i<=NF; i++) print %ci; }" \
	'$2' '$' '$'`
    deps=`awk "${ap}" jardep.deps/all.dep | grep '^icecube'`
    for d in ${deps}; do
	ap=`printf \
	    "%c2 ~ /^${d}%c/ { for (i=3; i<=NF; i++) print %ci \" ${d}\"; }" \
	    '$' '$' '$'`
	awk "${ap}" jardep.deps/all.dep | grep '^icecube'
    done

    #
    # now find all de

    # next arg...
    shift
done



