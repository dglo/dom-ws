#!/bin/bash

#
# iceboot.sh, move all doms that are talking into
# iceboot.  assume that they are currently in
# configboot...
#

if (( $# != 1 )); then
    echo "usage: iceboot.sh all|spec"
    echo "  where spec is: card pair dom (e.g. 00A)"
    exit 1
fi

if [[ $1 == "all" ]]; then
    find /proc/driver/domhub -name is-communicating -exec cat {} \; | \
	grep -v NOT | awk -vcmd=$0 '{ system(cmd " " $2 $4 $6); }'
else
    echo "spec=$1"

    card=`echo $1 | awk '{ print substr($1, 1, 1); }'`
    pair=`echo $1 | awk '{ print substr($1, 2, 1); }'`
    dom=`echo $1 | awk '{ print substr($1, 3, 1); }' | tr [a-z] [A-Z] | \
         tr AB 12`
    
    if (( $card < 0 || $card > 7 || $pair < 0 || $pair > 1 \
	|| dom < 1 || dom > 2 )); then
	echo "iceboot.sh: invalid dom spec"
	exit 1
    fi

    domn=`dc -e "$card 8 * $pair 2 * + $dom + p"`

    #
    # start domserv...
    #
    echo "open dom $1" | domserv -dh > /tmp/ds.$$ &
    sleep 2

    # make sure everything was ok...
    if ! head -1 /tmp/ds.$$ | egrep -q 'open 40[0-9][0-9] 45[0-9][0-9]'; then
	echo "iceboot.sh: unable to start domserv"
	kill -TERM `pgrep -P $$ domserv`
	wait
	rm -f /tmp/ds.$$
	exit 1
    fi
    rm -f /tmp/ds.$$

    if ! expect reboot.exp localhost `printf 40%02d ${domn}`; then
	echo "iceboot.sh: unable to reboot dom"
	kill -TERM `pgrep -P $$ domserv`
	wait
	rm -f /tmp/ds.$$
	exit 1
    fi

    # all done...
    kill -TERM `pgrep -P $$ domserv`
    wait
    rm -f /tmp/ds.$$
fi
