# !/bin/sh

#
# mkrelease.sh, make a release .hex file
#

#
# start dom server -- only open one port...
#
echo 'open sim 1' | domserv -dh >& /tmp/ds.$$ &
sleep 2

# make sure everything was ok...
ret=`head -1 /tmp/ds.$$`
rm -f /tmp/ds.$$
if [[ "${ret}" != "open 5001 5501" ]]; then
    echo "mkrelease.sh: unable to start domserv"
    kill -TERM `pgrep -P $$ domserv`
    wait
    rm -f /tmp/ds.$$
    exit 1
fi

#
# initialize flash filesystem...
#
if ! expect init.exp localhost 5001; then
    echo "mkrelease.sh: unable to initialize flash filesystem"
    exit 1
fi

#
# for each file to install...
#
for f in $*; do
    echo "installing ${f}..."

    #
    # give domserv some time to re-open port...
    #
    sleep 1

    #
    # start ymodem
    #
    if ! expect ymodem.exp localhost 5001; then
	echo "mkrelease.sh: unable to start ymodem"
	exit 1
    fi

    #
    # give domserv some time to reset ports...
    #
    sleep 1

    #
    # upload the program
    #
    if ! sz -k --ymodem --tcp-client localhost:5501 $f; then
	echo "mkrelease.sh: unable to send $f"
        exit 1
    fi

    #
    # give domserv time to reset ports...
    #
    sleep 1

    #
    # is it a gzip file?
    #
    if echo $f | grep -q '\.gz$'; then
	nm=`basename $f | sed 's/\.gz$//1'`
	
	#
	# strip .bin if it exists...
	#
	nm=`echo ${nm} | sed 's/\.bin$//1'`

	if ! expect creategz.exp localhost 5001 $nm; then
	    echo "mkrelease.sh: unable to create $nm"
	    exit 1
	fi
    else
	nm=`echo ${f} | sed 's/\.bin$//1'`
	nm=`basename ${nm}`
	if ! expect create.exp localhost 5001 $nm; then
	    echo "mkrelase.sh: unable to create $nm"
	    exit 1
	fi
    fi
done

#
# signal iceboot to dump flash image...
#
sleep 1
rm -f flash.dump
kill -USR1 `pgrep iceboot`

#
# wait for dump to finish...
#
i=0
while (( i < 30 )); do
    fn=`/usr/sbin/lsof | grep '^iceboot ' | \
	awk '{ if (substr($9, 1, 1)=="/") print $9; }' | \
	grep '/flash.dump$'`
    if [[ -f flash.dump && "${#fn}" == "0" ]]; then break; fi
    sleep 1
    i=`expr ${i} + 1`
    echo "flash.dump: try ${i} of 30..."
done

#
# never finished?
#
if (( ${i} == 100 )); then
    echo "unable to read dump file..."
    exit 1
fi

echo "convert dump file..."

#
# convert dump file to hex...
#
arm-elf-objcopy -I binary -O ihex flash.dump flash.hex

#
# remove redundant (empty) entries...
#
# grep-2.5 is broken using UTF-8, set lang to workaround this...
#
env LANG=en_US grep -E -v \
    '^:[0-9A-Z]+FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF[0-9A-Z]+.$' \
    flash.hex > release.hex

#
# now do the ebi0 and ebi1 flash files...
#
dd bs=4M count=1 if=flash.dump of=flash.dump.0
dd bs=4M count=1 skip=1 if=flash.dump of=flash.dump.1

for i in 0 1; do
	arm-elf-objcopy -I binary -O ihex flash.dump.${i} flash.hex.${i}

	# grep-2.5 is broken using UTF-8, set lang to workaround this...
	env LANG=en_US grep -E -v \
    		'^:[0-9A-Z]+FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF[0-9A-Z]+.$' \
    		flash.hex.${i} > release.hex.${i}
done

#
# cleanup...
#
echo "killing domserv..."
kill -TERM `pgrep -P $$ domserv`
wait

echo "cleaning up..."
rm -f /tmp/ds.$$
rm -f flash.hex flash.dump
rm -f flash.hex.[01] flash.dump.[01]






