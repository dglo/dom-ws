#!/bin/bash

#
# bldpkgs.sh, build packages given on command line
# and build all dependent packages as well...
#
if ! ./bldpkg.sh `./deppkgs.sh $*`; then
    echo "bldpkgs.sh: unable to build, exiting..."
    exit 1
fi

