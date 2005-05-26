#!/bin/bash

# acquire pedestal data for atwd B

cat dacs.se peds-b.se start.se dump.se | se 50a

for ((i=0; i<100; i++)); do
   cat acq-forced-10.se | se 50a
done > peds-b.out

cat dump.se stop.se | se 50a

