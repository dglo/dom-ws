#
# splitts.awk, split timestamps up.  there was
# a bug in the tcal-stf script that caused timestamps
# to be all pushed together into one string.  this
# script parses that data, or just passes to good data
# along...
#
# FIXME: this script does _not_ handle the hard case
# of rollover...
#
{   
    if (NF==1) {
        len=length($0);
#	if ( (len % 100) == 0 ) {
#	    # easy case...
#            elen=len / 100
#  	    for (i=0; i<100; i++) {
#		printf substr($0, 1+i*elen, elen) ((i==99) ? "\n" : " ");
#	    }
#	}
#	else {
            # extra chars...
	    exc=len % 100;
	    elen=int(len / 100);
            for (i=0; i<100-exc; i++) {
		printf substr($0, 1+i*elen, elen) ((i==99) ? "\n" : " ");
	    }
            idx=(100-exc)*elen; # index of first "new" string...
	    elen++;
	    for (i=0; i<exc; i++) {
		printf substr($0, 1+idx+i*elen, elen) ((i==exc-1) ? "\n" : " ");
	    }
#	}
    }
    else print $0;
}
