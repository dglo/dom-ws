BEGIN {
    FS = "; ";
}

/^Working file: / {
    workingPath = substr($0, 15);
    len = split(workingPath, ar, "/");
    workingFile = ar[len];
    workingPackage = ar[len-1];
}

/^revision / {
    revision = substr($0, 10);
}

/^date: / {
    cmd = "date --utc --date '" substr($1, 7) "' '+%s'";
    cmd | getline seconds;
    close(cmd);

    yr = strftime("%Y", seconds);
    mn = strftime("%m", seconds);
    if ( yr == year  && month == mn ) {
	if ( day == 0 || day == strftime("%d") ) {
	    docat = 1;
	    
	    if ( revision=="1.1" ) {
		#
		# this is an initial checkin, we
		# need to go find out how many lines
		# were added by checking it out...
		#
		cmd = "cd ../; cvs -q checkout -p -r 1.1 " workingPath " | \
                   wc -l ";
                cmd | getline modified;
	    }
	    else {
		modified = substr($4, 9);
	    }

	    print seconds "\t" \
		project "\t" workingPackage "\t" workingFile "\t" \
		substr($2, 10) "\t" modified "\t" \
		strftime("%d", seconds) >> \
		"/tmp/exlog/all.entries";
	}
    }
}

/^----------------------------$/ {
    docat = 0;
}

/^======================================================================/ {
    docat = 0;
}

{
    if ( docat ) {
	if (docat>1) {
	    print "    " $0 >> mfname;
	}
	else {
	    mfname = "/tmp/exlog/" seconds ".msg";

	    if ( system("[[ -e " mfname " ]]") ) {
		system("rm -f " mfname);
		system("touch " mfname);
	    }
	    else {
                #
		# if the file exists, no more messages needed, but
		# we should update the header...
		#
		docat = 0;
	    }
	}
	if (docat) docat++;
    }
}
