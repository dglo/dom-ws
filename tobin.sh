#!/bin/bash

# convert PULS -> DATA data to binary...

cat $1 | tr -d '\r' | grep -v '^PULS now time=[0-9a-z]*$' | \
    grep -v '^DATA$' | ./Linux-i386/bin/decode64

