#!/bin/bash

#
# checkout the given release...
#
release=`cat prod.num`

modules="daq-db-common daq-db-stftest dom-cal dom-cpld dom-fpga dom-loader dom-ws domhub-common hal iceboot stf stfapp testdomapp"

(cd ..; cvs -z9 checkout -raz-prod-rel-${release} ${modules} )

