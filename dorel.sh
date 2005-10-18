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

BINS="iceboot.bin.gz stfserv.bin.gz echomode.bin.gz"
BINS="${BINS} domapp-test.bin.gz wiggle.bin.gz domcal5.bin.gz"
BINGZ="domapp.bin.gz testdomapp.bin.gz"
SBI='simpletest.sbi'
CBSBI='configboot.sbi'
SBIL='iceboot.sbi'
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

for f in ${BINGZ}; do
    if [[ ! -f ${bindir}/${f} ]]; then
	echo "can not find: ${f}"
	exit 1
    fi
    
    cp ${bindir}/${f} ${REL}/`echo $f | sed 's/\.bin//1'`
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

if [[ ! -f ../dom-fpga/domapp/domapp.sbi ]]; then
    echo "can not find sbi file: ../dom-fpga/domapp/domapp.sbi"
    exit 1
fi
cp -l ../dom-fpga/domapp/domapp.sbi ${REL}/domapp.sbi

#
# cp .fs files
#
(cd ${fsdir}; cp -l ${FS} ../../dom-ws/${REL})

#
# create release.hex files... 
#
if ! /bin/bash mkrelease.sh ${REL}/* ; then
    rm -rf devel-release
    echo "unable to create release.hex files..."
    exit 1
fi

rm -rf ${REL}

