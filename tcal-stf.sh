#!/bin/bash

#
# tcal-stf.sh, time calibration stf test...
#

#
# first check parameter...
#
if (( $# != 1)); then
    echo "usage: tcal-stf CWD"
    echo "  where C=card number, W=wire pair number, D=dom A or B"
    exit 1
fi

if ! echo $1 | grep -q '[0-7][0-3][AB]'; then 
    echo "invalid card, wire pair or dom A/B"
    exit 1
fi

card=`echo $1 | awk '{ print substr($0, 1, 1); }'`
wp=`echo $1 | awk '{ print substr($0, 2, 1); }'`
dom=`echo $1 | awk '{ print substr($0, 3, 1); }'`
procd=`printf '/proc/driver/domhub/card%c/pair%c/dom%c/' $card $wp $dom`
procf=${procd}/tcalib

if [[ ! -f ${procf} ]]; then
    echo "tcal-stf: can not find proc file ${procf}"
    exit 1
fi

#
# get board id...
#
id=`cat ${procd}/id | awk '{ printf $NF; }'`

if [[ $#id == "" ]]; then
    echo "tcal-stf: can not get device id"
    exit 1
fi

#
# run the test...
#
rm -f /tmp/$$.tcal
if ! ./tcalcycle -iter 100 $1 > /tmp/$$.tcal; then
    echo "tcal-stf: unable to run test"
    rm -f $$.tcal
    exit 1
fi

#
# get results...
#
if ! res=`./tcal.sh /tmp/$$.tcal`; then
    echo "tcal-stf: unable to get results of test"
    rm -f $$.tcal
fi

rtt=`echo $res | awk '{ print $1 * 1000; }' | sed 's/\..*$//1'`
rte=`echo $res | awk '{ print $2 * 1000; }' | sed 's/\..*$//1'`

#
# set pass/no pass
#
if (( $rte < 5000 )); then passed="true"; else passed="false"; fi

#
# get dor/dom waveforms...
#
dorwf=`awk -f dor.awk /tmp/$$.tcal | head -4800 | tr '\n' ' '`
domwf=`awk -f dom.awk /tmp/$$.tcal | head -4800 | tr '\n' ' '`

#
# done with results file
#
rm -f /tmp/$$.tcal

#
# create results file...
#
cat <<EOF > /tmp/$$.stf.xml
<?xml version="1.0" encoding="UTF-8" ?>
<stf:result xmlns:stf="http://glacier.lbl.gov/icecube/daq/stf"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xsi:schemaLocation="http://glacier.lbl.gov/icecube/daq/stf stf.xsd">
 <tcal>
  <description>
Time Calibration tests
  </description>
  <version major="1" minor="0"/>
  <parameters>
   <round_trip_time>${rtt}</round_trip_time>
   <round_trip_error>${rte}</round_trip_error>
   <dom_waveforms>${domwf}</dom_waveforms>
   <dor_waveforms>${dorwf}</dor_waveforms>
   <passed>${passed}</passed>
   <testRunnable>true</testRunnable>
   <boardID>${id}</boardID>
  </parameters>
 </tcal>
</stf:result>
EOF

#
# stf-client: run the stf client...
#
if (( ${#JAVA_HOME} == 0 )); then
    echo "tcal-stf: JAVA_HOME must be set"
    rm -f /tmp/$$.stf.xml
    exit 1
fi

jbin=${JAVA_HOME}/bin
cp=`/usr/bin/lessecho ./jars/*.jar | tr ' ' ':' | sed 's/:$//1'`

if ! ${jbin}/java icecube.daq.db.app.AddResult /tmp/$$.stf.xml; then
    echo "tcal-stf: can not load results into database"
    rm -f /tmp/$$.stf.xml
    exit 1
fi

rm -f /tmp/$$.stf.xml

echo ${rtt} ${rte}





