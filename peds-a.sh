#!/bin/bash

# acquire pedestal data for atwd A
dom=10a

cat dacs.se peds-a.se start.se dump.se | se ${dom}

for ((i=0; i<100; i++)); do
   cat acq-forced-10.se | se ${dom}
done > peds-a.out

cat dump.se stop.se | se ${dom}

