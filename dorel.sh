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

BINS="iceboot domcal5"
BINS="${BINS} domapp-test"
BINGZS="menu stfserv wiggle echomode testdomapp domapp"
SBI='simpletest.sbi'
CBSBI='configboot.sbi'
FS='startup.fs az-setup.fs'

#
# cp and unzip binaries...
#
for f in ${BINS}; do
    if [[ ! -f ${bindir}/${f}.bin.gz ]]; then
       echo "can not find: ${f}.bin.gz"
       exit 1
    fi
    cp ${bindir}/${f}.bin.gz ${REL}/${f}.bin.gz
    gunzip ${REL}/${f}.bin.gz
    if [[ ${f} == "domcal5" ]]; then
	mv ${REL}/${f}.bin ${REL}/domcal
    else
	mv ${REL}/${f}.bin ${REL}/${f}
    fi
done

#
# zipped binaries...
#
for f in ${BINGZS}; do
    if [[ ! -f ${bindir}/${f}.bin.gz ]]; then
       echo "can not find: ${f}.bin.gz"
       exit 1
    fi
    cp ${bindir}/${f}.bin.gz ${REL}/${f}.bin.gz
    mv ${REL}/${f}.bin.gz ${REL}/${f}.gz
done

#
# cp iceboot.sbi...
#
if ! cp -l ${sbidir}/${SBI} ${REL}/iceboot.sbi; then
    echo "can not cp iceboot sbi file: ${sbidir}/${SBI}"
    exit 1
fi

#
# cp configboot.sbi (and compress it...)
#
if ! cp -l ${cbsbidir}/${CBSBI} ${REL}; then
    echo "can not cp configboot sbi file: ${cbsbidir}/${CBSBI}"
    exit 1
fi

if ! gzip -c ${REL}/${CBSBI} > ${REL}/cb.sbi.gz; then
    echo "can not compress configboot sbi file: ${REL}/${CBSBI}"
    exit 1
fi

rm -f ${REL}/${CBSBI}

#
# cp stf-nocomm.sbi and compress it...
#
if ! cp -l ${ncsbidir}/${SBI} ${REL}/stf-nocomm.sbi; then
    echo "can not cp no-comm sbi file: ${ncsbidir}/${SBI}"
    exit 1
fi

if ! gzip -c ${REL}/stf-nocomm.sbi > ${REL}/stf-nc.sbi.gz; then
    echo "can not compress: ${REL}/stf-nocomm.sbi"
    exit 1
fi

rm -f ${REL}/stf-nocomm.sbi

#
# cp domapp.sbi...
#
if ! cp -l ../dom-fpga/domapp/domapp.sbi ${REL}/domapp.sbi; then
    echo "can not find sbi file: ../dom-fpga/domapp/domapp.sbi"
    exit 1
fi

#
# cp .fs files
#
(cd ${fsdir}; cp -l ${FS} ../../dom-ws/${REL})

#
# cp fb cpld file
#
if ! gzip -c ../fb-cpld/CPLD_V12/FB_CPLDV12.xsvf > ${REL}/fb-cpld.xsvf.gz; then
    echo "`basename $0`: unable to copy fb-cpld"
    exit 1
fi

#
# create release.hex files... 
#
if ! /bin/bash mkrelease.sh ${REL}/* ; then
    rm -rf devel-release
    echo "unable to create release.hex files..."
    exit 1
fi

rm -rf ${REL}

