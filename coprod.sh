#!/bin/bash

if (( $1 <= 0 || $1 >= 10000000 )); then
	echo "usage: coprod.sh prod_release_number"
	exit 1
fi

#
# coprod.sh, checkout a prod release by number
#
modules="configboot daq-db-common daq-db-stftest dom-cal dom-cpld dom-fpga dom-loader dom-ws domhub-common dor-test hal iceboot stf stfapp testdomapp"

cvs checkout -r az-prod-rel-$1 ${modules}

