#!/bin/bash 

#
# setup directories here...
#
REL=devel-release
bindir=epxa10/bin
fsdir=../iceboot/resources
sbidir=../dom-fpga/stf/ComEPXA4DPM

if [[ -d ${REL} ]]; then
    echo "devel-release directory already exists, please remove it"
    exit 1
fi

mkdir ${REL}

BINS='iceboot.bin.gz stfserv.bin.gz menu.bin.gz domapp.bin.gz echomode.bin.gz'
SBI='simpletest.sbi'
SBIL='stf.sbi domapp.sbi iceboot.sbi'
FS='startup.fs az-setup.fs az-tests.fs'

#
# cp and unzip binaries...
#
for f in ${BINS}; do
    if [[ ! -f ${bindir}/${f} ]]; then
       echo "can not find: ${f}"
       exit 1
    fi
    cp ${bindir}/${f} ${REL}
    gunzip ${REL}/${f}
done

#
# cp and link sbi files...
#
# FIXME: we should be more flexible here...
#
if [[ ! -f ${sbidir}/${SBI} ]]; then
    echo "can not find sbi file: ${sbidir}/${SBI}"
    exit 1
fi
(cd ${REL}; for f in ${SBIL}; do ln -s ../${sbidir}/${SBI} ${f}; done )

#
# copy configboot fpga
#
cp ../dom-fpga/configboot/epxa4DPM/configboot.sbi ${REL}

#
# cp .fs files
#
(cd ${fsdir}; cp ${FS} ../../dom-ws/${REL})

#
# create release.hex files... 
#
if ! /bin/bash mkrelease.sh ${REL}/* ; then
    rm -rf devel-release
    echo "unable to create release.hex files..."
    exit 1
fi

rm -rf ${REL}

exit 0
