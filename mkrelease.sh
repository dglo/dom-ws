# !/bin/sh

#
# mkrelease.sh, make a release.hex file
#
cp init.exp /tmp/$$.se

#
# for each file to install...
#
for f in $*; do
    #
    # start ymodem
    #
    echo 'send "ymodem1k\r"' >> /tmp/$$.se
    echo 'expect "C"' >> /tmp/$$.se
    echo 'send "^A"' >> /tmp/$$.se
    echo 'expect "^domterm> "' >> /tmp/$$.se
    echo "send \"sz -k --ymodem $f\r\"" >> /tmp/$$.se 
    echo 'expect "^domterm> "' >> /tmp/$$.se
    echo 'send "status\n"' >> /tmp/$$.se
    echo 'expect "^0"' >> /tmp/$$.se
    echo 'send "\n"' >> /tmp/$$.se
    echo 'sleep 1' >> /tmp/$$.se
    echo 'send "\r"' >> /tmp/$$.se
    echo 'expect "^> "' >> /tmp/$$.se

    nm=`echo ${f} | sed 's/\.bin$//1'`
    nm=`basename ${nm}`
    echo "send \"s\\\" $nm\\\" create\r" >> /tmp/$$.se

    echo 'expect "^> "' >> /tmp/$$.se
done

echo "send \"dump-flash\r\"" >> /tmp/$$.se
echo "expect \"^> \"" >> /tmp/$$.se
echo "send \"ls\r\"" >> /tmp/$$.se
echo "expect \"^> \"" >> /tmp/$$.se

if ! se sim < /tmp/$$.se; then
    rm -f /tmp/$$.se
    echo "mkrelease.sh: unable to run install script"
    exit 1
fi

rm -f /tmp/$$.se

echo "convert dump file..."

#
# convert dump file to hex...
#
if ! arm-elf-objcopy -I binary -O ihex flash.dump flash.hex; then
	echo "mkrelease.sh: unable to create flash hex image"
	exit 1
fi

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
# display md5sum...
#
md5sum flash.dump

echo "cleaning up..."
rm -f /tmp/ds.$$
rm -f flash.hex flash.dump
rm -f flash.hex.[01] flash.dump.[01]






