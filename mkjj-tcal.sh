#!/bin/bash

#
# make a jj tcal file from data output
# from the database result.xml file...
#
if (( $# != 1 )); then
    echo "usage: mkjj-tcal.sh result.xml"
    exit 1
fi

function atexit () {
    rm -f /tmp/jj.$$.*
}
trap atexit EXIT

function xmlex () {
    printf '/^\\(%s$/ { wf=1; }\n' $1 > /tmp/jj.$$.awk
    printf '/^\\)%s$/ { printf "\\n"; wf=0; }\n' $1 >> /tmp/jj.$$.awk
    printf '/^-/ { if (wf) printf substr($0, 2); }\n' >> /tmp/jj.$$.awk
    echo /tmp/jj.$$.awk
}

xmln $1 | awk -f `xmlex dor_waveforms` | awk -f splitwf.awk | \
    awk '{ print "dor_wf" $0; }' > /tmp/jj.$$.dorwf
xmln $1 | awk -f `xmlex dom_waveforms` | awk -f splitwf.awk | \
    awk '{ print "dom_wf" $0; }' > /tmp/jj.$$.domwf

xmln $1 | awk -f `xmlex dor_tx_lo` | awk -f splitts.awk | tr ' ' '\n' > \
    /tmp/jj.$$.dortxlo
xmln $1 | awk -f `xmlex dor_tx_hi` | awk -f splitts.awk | tr ' ' '\n' > \
    /tmp/jj.$$.dortxhi
paste /tmp/jj.$$.dortxhi /tmp/jj.$$.dortxlo | \
    awk '{ print $1 " 2 32 ^ * " $2 " + 16 o p"; }' | dc | \
    tr '[A-Z]' '[a-z]' > /tmp/jj.$$.dortx

xmln $1 | awk -f `xmlex dor_rx_lo` | awk -f splitts.awk | tr ' ' '\n' > \
    /tmp/jj.$$.dorrxlo
xmln $1 | awk -f `xmlex dor_rx_hi` | awk -f splitts.awk | tr ' ' '\n' > \
    /tmp/jj.$$.dorrxhi
paste /tmp/jj.$$.dorrxhi /tmp/jj.$$.dorrxlo | \
    awk '{ print $1 " 2 32 ^ * " $2 " + 16 o p"; }' | dc | \
    tr '[A-Z]' '[a-z]' > /tmp/jj.$$.dorrx

xmln $1 | awk -f `xmlex dom_rx_lo` | awk -f splitts.awk | tr ' ' '\n' > \
    /tmp/jj.$$.domrxlo
xmln $1 | awk -f `xmlex dom_rx_hi` | awk -f splitts.awk | tr ' ' '\n' > \
    /tmp/jj.$$.domrxhi
paste /tmp/jj.$$.domrxhi /tmp/jj.$$.domrxlo | \
    awk '{ print $1 " 2 32 ^ * " $2 " + 16 o p"; }' | dc | \
    tr '[A-Z]' '[a-z]' > /tmp/jj.$$.domrx

xmln $1 | awk -f `xmlex dom_tx_lo` | awk -f splitts.awk | tr ' ' '\n' > \
    /tmp/jj.$$.domtxlo
xmln $1 | awk -f `xmlex dom_tx_hi` | awk -f splitts.awk | tr ' ' '\n' > \
    /tmp/jj.$$.domtxhi
paste /tmp/jj.$$.domtxhi /tmp/jj.$$.domtxlo | \
    awk '{ print $1 " 2 32 ^ * " $2 " + 16 o p"; }' | dc | \
    tr '[A-Z]' '[a-z]' > /tmp/jj.$$.domtx

paste /tmp/jj.$$.dortx /tmp/jj.$$.dorrx /tmp/jj.$$.domrx /tmp/jj.$$.domtx | \
    awk '{ print "cal(" (NR-1) ") dor_tx(0x" $1 ") dor_rx(0x" $2 ") dom_rx(0x" $3 ") dom_tx(0x" $4 ")"; }' > /tmp/jj.$$.times

echo "Will_use_time_delay_of_0_microseconds_between_calibrations."
for (( i=0; i<100; i++ )); do printf '\n'; done > /tmp/jj.$$.nl
paste -d '\n' /tmp/jj.$$.times /tmp/jj.$$.dorwf /tmp/jj.$$.domwf /tmp/jj.$$.nl


















