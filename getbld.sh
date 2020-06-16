#!/bin/bash 

#
# autoincrementing build number...
#
bnf=build.num

#
# create file if it doesn't exist yet...
#
if [[ ! -f $bnf ]] ; then echo 0 > ${bnf}; fi

bn=`cat ${bnf}`

#
# now increment it...
#
nbn=`expr $bn + 1`
echo $nbn > $bnf

echo $bn

