#!/bin/bash

# acquire pedestal data for atwd B
dom=10a

cat dacs.se peds-b.se start.se dump.se | se ${dom}

for ((i=0; i<100; i++)); do
   cat acq-forced-10.se | se ${dom}
done > peds-b.out

cat dump.se stop.se | se ${dom}

