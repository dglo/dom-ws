# !/bin/sh 

REL=DOM-MB.11

mkdir ${REL}
cd ${REL}

BINS='iceboot.bin.gz stfserv.bin.gz menu.bin.gz domapp.bin.gz'
SBI='simpletest_com_13.sbi'
SBIL='stf.sbi.gz domapp.sbi.gz iceboot.sbi.gz'
FS=startup.fs

for f in ${BINS}; do
    scp glacier:/home/icecube/product-builds/${REL}/epxa10/bin/${f} .
done

scp glacier:/home/icecube/product-builds/${REL}/epxa10/rsrc/dom-fpga/${SBI} .
scp glacier:/home/icecube/product-builds/${REL}/rsrc/iceboot/${FS} .

gzip -c ${SBI} > ${SBI}.gz

for f in ${SBIL}; do
    ln -s ${SBI}.gz $f
done

cd ..
/bin/sh mkrelease.sh ${REL}/iceboot.bin.gz ${REL}/stfserv.bin.gz ${REL}/startup.fs ${REL}/iceboot.sbi.gz ${REL}/stf.sbi.gz ${REL}/domapp.sbi.gz ${REL}/domapp.bin.gz ${REL}/menu.bin.gz 

rm -rf ${REL}

