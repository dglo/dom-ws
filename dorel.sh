#!/bin/bash 

#
# setup directories here...
#
REL=devel-release
bindir=epxa10/bin
fsdir=../iceboot/resources
sbidir=../dom-fpga/stf/ComEPXA4DPM
cbsbidir=../dom-fpga/configboot/epxa4DPM
ncsbidir=../dom-fpga/stf/NoComEPXA4

if [[ -d ${REL} ]]; then
    echo "devel-release directory already exists, please remove it"
    exit 1
fi

if ! mkdir ${REL}; then
    echo "unable to make directory ${REL}"
    exit 1
fi

BINS='iceboot.bin.gz stfserv.bin.gz menu.bin.gz domapp.bin.gz echomode.bin.gz'
BINS="${BINS} wiggle.bin.gz"
SBI='simpletest.sbi'
CBSBI='configboot.sbi'
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
# cp domcal...
#
if ! cp ${bindir}/domcal5.bin.gz ${REL}/domcal.bin.gz; then
    echo "unable to cp ${bindir}/domcal5.bin.gz..."
    exit 1
fi

#
# cp and link sbi files...
#
# FIXME: we should be more flexible here...
#
if [[ ! -f ${sbidir}/${SBI} ]]; then
    echo "can not find sbi file: ${sbidir}/${SBI}"
    exit 1
fi
(cd ${REL}; for f in ${SBIL}; do ln ../${sbidir}/${SBI} ${f}; done )

if [[ ! -f ${cbsbidir}/${CBSBI} ]]; then
    echo "can not find configboot sbi file: ${cbsbidir}/${CBSBI}"
    exit 1
fi
cp -l ${cbsbidir}/${CBSBI} ${REL}

if [[ ! -f ${ncsbidir}/${SBI} ]]; then
    echo "can not find sbi file: ${ncsbidir}/${SBI}"
    exit 1
fi
cp -l ${ncsbidir}/${SBI} ${REL}/stf-nocomm.sbi

#
# cp .fs files
#
(cd ${fsdir}; cp -l ${FS} ../../dom-ws/${REL})

#
# create release.hex files... 
#
if ! /bin/bash mkrelease.sh ${REL}/*; then
    rm -rf devel-release
    echo "unable to create release.hex files..."
    exit 1
fi

rm -rf ${REL}

