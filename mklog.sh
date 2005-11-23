#!/bin/bash

nrel=`cat prod.num`
rel=`echo ${nrel} | awk '{ print $1 " 1 - p"; }' | dc`
echo "Release ${nrel} [`date '+%F'`]:"
printf '\n'
(cd ..; cg log -s -r rel-${rel}..HEAD) | \
	sed 's/.* [0-2][0-9]:[0-5][0-9] //1' | \
	awk '{ print "    o " $0; }'
printf '\n'
echo "-----------------------------------------------------------------------------"
