#!/bin/bash

#
# mkprod.sh, make up a copy of the production
# environment...
#

#
# required packages...
#
pkgs="icecube.daq.stf icecube.daq.domhub icecube.daq.db.app"

#
# production build number
#
rev=REV5

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
# make dir
#
mkdir ${dir}

#
# cp configboot image...
#
cp configboot.pof ${dir}

#
# cp std-tests (standard tests) -- read-only...
#
(cd ../stf/private; tar cf - std-tests/*.xml) | (cd ${dir}; tar xf -)
chmod ugo-w ${dir}/std-tests

#
# cp templates
#
mkdir ${dir}/templates
(cd epxa10/stf-apps; make templates.tar.gz)
gzip -dc epxa10/stf-apps/templates.tar.gz | (cd ${dir}/templates; tar xf -)

#
# make clean for packages...
#
if ! target=clean ./bldpkg.sh `./deppkgs.sh icecube.daq.stf`; then
    echo "mkprod.sh: unable to make dep in java directories"
    exit 1
fi

#
# make deps for packages...
#
if ! target=dep ./bldpkg.sh `./deppkgs.sh icecube.daq.stf`; then
    echo "mkprod.sh: unable to make dep in java directories"
    exit 1
fi

#
# build then cp jar files 
#
if ! ./bldpkgs.sh icecube.daq.stf; then
    echo "mkprod.sh: unable to build jar files, exiting..."
    exit 1
fi

echo "copying..."

mkdir ${dir}/jars
cp `./getjars.sh ${pkgs} | sort | uniq` ${dir}/jars

#
# daq-db has a mysql connector to cp
#
if [[ -f ${dir}/jars/daq-db-common.jar ]]; then
    cp ../daq-db-common/resources/mysql-connector-java.jar ${dir}/jars
fi

#
# cp run script
#
cp ./stf-client ${dir}
chmod +x ${dir}/stf-client

#
# always build and cp software/firmware files
#
if ! ( make clean && make PROJECT_TAG=az-prod ICESOFT_BUILD=${bldn} ) then
	echo "mkprod.sh: can't build..."
	exit 1
fi

if ! /bin/bash dorel.sh; then
	echo "can't create release.hex"
	exit 1
fi

#
# cp release files...
#
cp release.hex release.hex.0 release.hex.1 ${dir}
cp ../dom-cpld/eb_interface_rev2.jed ${dir}

#
# cp add-schema files...
#
cp add-schema ${dir}
chmod +x ${dir}/add-schema
mkdir ${dir}/stf-schema
if ! (cd epxa10/stf-apps; find . -name '*.xml' -print | \
   grep -v -- '-template\.xml$' | cpio -p -L -d ../../${dir}/stf-schema); then
   echo "unable to cp schema files"
   exit 1
fi

#
# now tar it all up...
#
echo "tarring..."
tar cf - ${dir} | gzip -c > ${dir}.tar.gz

#
# clean up...
#
echo "cleaning up..."
chmod u+w ${dir}/std-tests
rm -rf ${dir}



