# !/bin/sh

#
# mkrelease.sh, make a release .hex file
#

#
# start dom server
#
./domserv >& /dev/null &

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
rev release.hex > flash.hex
#
# awk { remove ^:02 dup }
#
rev flash.hex > release.hex

#
# cleanup...
#
kill `pgrep domserv`
wait
rm -f flash.hex flash.dump









