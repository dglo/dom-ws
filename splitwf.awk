#
# split dom/dor waveform...
#
{
    if (NF==4800) {
	for (i=0; i<100; i++) {
	    printf "(";
	    for (j=1; j<=48; j++) {
		printf $(j+i*48) ", ";
	    }
	    for (j=49; j<64; j++) {
		printf "0, ";
	    }
	    printf "0)\n";
	}
    }
    else {
	print "splitwf.awk: unexpected NF=" NF;
    }
}
