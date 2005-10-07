#!/bin/bash

#
# tag the last release...
#
release=`cat prod.num`

modules="daq-db-common daq-db-stftest dom-cal dom-cpld dom-fpga dom-loader dom-ws domhub-common dor-test hal iceboot stf stfapp testdomapp"

(cd ..; cvs tag domapp-${release} ${modules} )

