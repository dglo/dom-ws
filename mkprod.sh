#!/bin/bash

#
# mkprod.sh, make up a copy of the production
# environment...
#

#
# required packages...
#
pkgs="icecube.daq.stf icecube.daq.domhub"

#
# production build number
#
rev=REV4

#
# get and increment build number
#
if [[ ! -f prod.num ]]; then
    echo "1" > prod.num
fi

bldn=`cat prod.num`
bldn=`expr ${bldn} + 1`
echo ${bldn} > prod.num

#
# get dir
#
dir=prod-${rev}-${bldn}
if [[ -d ${dir} ]]; then
    echo "mkprod.sh: ${dir} already exists"
    exit 1
fi

#
# cp std-tests (standard tests) -- read-only...
#
mkdir ${dir}
(cd ../stf/private; tar cf - std-tests/*.xml) | (cd ${dir}; tar xf -)
chmod ugo-w ${dir}/std-tests

#
# cp templates
#
mkdir ${dir}/templates
gzip -dc epxa10/stf-apps/templates.tar.gz | (cd ${dir}/templates; tar xf -)

#
# cp jar files 
#
mkdir ${dir}/jars
cp `./getjars.sh ${pkgs} | sort | uniq` ${dir}/jars

#
# daq-db has a mysql connector to cp
#
if [[ -f ${dir}/jars/daq-db.jar ]]; then
    cp ../daq-db/resources/mysql-connector-java.jar ${dir}/jars
fi

#
# cp run script
#
cp ./stf-client ${dir}
chmod +x ${dir}/stf-client

#
# cp firmware files
#
cp release.hex release.hex.0 release.hex.1 ${dir}
cp ../dom-cpld/eb_interface_rev2.jed ${dir}

#
# now tar it all up...
#
tar cf - ${dir} | gzip -c > ${dir}.tar.gz

#
# clean up...
#
rm -rf ${dir}


