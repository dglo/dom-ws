# !/bin/sh

#
# mkrelease.sh, make a release .hex file
#

#
# start dom server -- only open one port...
#
echo 'open sim 1' | ./domserv -dh >& /tmp/ds.$$ &
sleep 2

# make sure everything was ok...
ret=`head -1 /tmp/ds.$$`
rm -f /tmp/ds.$$
if [[ "${ret}" != "open 5001 5501" ]]; then
    echo "mkrelease.sh: unable to start domserv"
    kill -TERM `pgrep -P $$ domserv`
    wait
    exit 1
fi

#
# initialize flash filesystem...
#
expect init.exp localhost 5001

#
# for each file to install...
#
for f in $*; do
    echo "installing ${f}..."

    #
    # start ymodem
    #
    expect ymodem.exp localhost 5001

    #
    # give some time for domserv to reset ports...
    #
    sleep 1

    #
    # upload the program
    #
    sz -k --ymodem --tcp-client localhost:5501 $f

    #
    # is it a gzip file?
    #
    if echo $f | grep -q '\.gz$'; then
	nm=`basename $f | sed 's/\.gz$//1'`
	
	#
	# strip .bin if it exists...
	#
	nm=`echo ${nm} | sed 's/\.bin$//1'`

	expect creategz.exp localhost 5001 $nm;
    else
	nm=`basename $f`
	expect create.exp localhost 5001 $nm;
    fi
done

#
# send sigusr1
#
kill -USR1 `pgrep iceboot`

#
# wait for dump to finish...
#
sleep 20

#
# convert dump file to hex...
#
arm-elf-objcopy -I binary -O ihex flash.dump flash.hex

#
# remove redundant (empty) entries...
#
grep -E -v \
    '^:[0-9A-Z]+FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF[0-9A-Z]+.$' \
    flash.hex > release.hex

#
# FIXME: remove repeated address offsets...
#
#
# awk { remove ^:02 dup }
#

#
# now do the ebi0 and ebi1 flash files...
#
dd bs=4M count=1 if=flash.dump of=flash.dump.0
dd bs=4M count=1 skip=1 if=flash.dump of=flash.dump.1

for i in 0 1; do
	arm-elf-objcopy -I binary -O ihex flash.dump.${i} flash.hex.${i}

	grep -E -v \
    		'^:[0-9A-Z]+FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF[0-9A-Z]+.$' \
    		flash.hex.${i} > release.hex.${i}
done

#
# cleanup...
#
kill -TERM `pgrep -P $$ domserv`
wait
rm -f flash.hex flash.dump
rm -f flash.hex.[01] flash.dump.[01]






