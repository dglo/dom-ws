#!/bin/sh 

#
# increment build number
#
if [[ ! -f build_num ]] ; then echo 0 > build_num; fi

bn=`cat build_num`
bn=`expr $bn + 1`
echo $bn > build_num
