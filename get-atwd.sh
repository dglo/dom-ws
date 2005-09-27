#!/bin/bash

#od -A x -t x4 $1 | grep '^[0-9a-f]*[80]00 ' | gawk '{ print int(strtonum("0x" $4)/65536) % 2; }'

cat $* | ./decoderaw -h | awk '{ print substr($5, 5, 1); }'
