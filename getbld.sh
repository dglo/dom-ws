#!/bin/sh 

#
# get current build number
#
if [[ ! -f build_num ]] ; then echo 0 > build_num; fi
cat build_num
