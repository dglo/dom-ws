# !/bin/sh 

REL=devel-2003-10-14

mkdir ${REL}
cd ${REL}

BINS='iceboot.bin.gz stfserv.bin.gz menu.bin.gz domapp.bin.gz echomode.bin.gz'
SBI='simpletest_rev3_epxa4_com.sbi'
SBIL='stf.sbi.gz domapp.sbi.gz iceboot.sbi.gz'
FS=startup.fs

for f in ${BINS}; do
    if [[ ! -f ../epxa10/bin/${f} ]]; then
       echo "can not find: ${f}"
       exit 1
    fi
    cp ../epxa10/bin/${f} .
done

cp ../../dom-fpga/resources/epxa10/${SBI} .
cp ../../iceboot/resources/${FS} .

gzip -c ${SBI} > ${SBI}.gz

for f in ${SBIL}; do
    ln -s ${SBI}.gz $f
done

cd ..
/bin/sh mkrelease.sh ${REL}/iceboot.bin.gz ${REL}/stfserv.bin.gz ${REL}/startup.fs ${REL}/iceboot.sbi.gz ${REL}/stf.sbi.gz ${REL}/domapp.sbi.gz ${REL}/domapp.bin.gz ${REL}/menu.bin.gz ${REL}/echomode.bin.gz

mv ~/public_html/dom/release.*.gz ~/public_html/dom/old
gzip -c release.hex > ~/public_html/dom/release.hex.gz
gzip -c release.hex.0 > ~/public_html/dom/release.hex.0.gz
gzip -c release.hex.1 > ~/public_html/dom/release.hex.1.gz

rm -rf ${REL}

