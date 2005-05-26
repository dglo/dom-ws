#!/bin/bash

./tobin.sh $1 | od -A x -t x4 | grep '^[0-9a-f]*[80]00 ' | gawk '{ print int(strtonum("0x" $4)/65536) % 2; }'
