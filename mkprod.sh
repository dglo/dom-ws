#!/bin/bash

#
# mkprod.sh, make up a copy of the production
# environment...
#

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
# always build and cp software/firmware files
#
if ! ( make clean && make PROJECT_TAG=az-prod ICESOFT_BUILD=${bldn} ) > \
		/dev/null; then
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
cp INSTALL ${dir}

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
rm -rf ${dir}
