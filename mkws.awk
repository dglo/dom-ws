#
# mkws.awk, make a workspace...
#
BEGIN {
   cvslogin = ":ext:glacier.lbl.gov:/home/icecube/cvsroot";
   cvsroot = "..";
   if (clean!=NULL) print "cleaning...";
   else print "creating...";
}

/^cvsroot[ \t]/ {
   cvsroot = $2;
}

/^cvslogin[ \t]/ {
   cvslogin = $2;
}

/^platforms[ \t]/ {
   if (clean!=NULL) {
      printf("FIXME: clean!\n");
      exit(1);
   }
   for (i=2; i<=NF; i++) platforms[i-2] = $i;
}

/^import[ \t]/ {
   if (chkProject($2)==0) {
      print "import: can't find project: '" $2 "', exiting...";
      exit(1);
   }

   for (p in platforms) {
      delete dirs;
   
      if (NF==2) {
	 pdirs = findDirs(platforms[p], $2);
	 split(pdirs, dirs, ":");
      }
      else {
	 for (i=3; i<=NF; i++) dirs[i-3] = $i;
      }

      for (d in dirs) {
	 if (clean!=NULL) {
	    cleanImport(platforms[p], $2, dirs[d]);
	 }
	 else {
	    mkImport(platforms[p], $2, dirs[d]);
	 }
      }
   }
}

/^link[ \t]/ {
   if (chkProject($2)==0) {
      print "import: can't find project: '" $2 "', exiting...";
      exit(1);
   }

   doLink($2, $3, $4, $5, $6, $7);
}

function doLink(project, directory, scope, file, lscope, location,
		p, loc, target, adj) {
   if (chkProject(project)==0) {
      print "link: can't find project: '" project "', exiting...";
      exit(1);
   }

   for (p in platforms) {
       loc = platforms[p] "/" location;
       if ( system("/bin/sh -c '[[ -d " loc " ]]'") ) {
	   if ( system("mkdir -p " loc) ) {
	       print "link: can't create directory: " loc;
	       exit(1);
	   }
       }

       if (lscope=="private") {
	   adj = "../../";
	   loc = platforms[p] "/" location "/" file;
       }
       else {
	   adj = "../../../";
	   loc = platforms[p] "/public/" location "/" file;
       }

       if (system("/bin/sh -c '[[ -h " loc " ]]'") == 0) {
	   if (system("rm -f " loc)) {
	       print "link: can't remove: " loc;
	       exit(1);
	   }
       }
       

       target = cvsroot "/" project "/" scope "/";
       target = target platforms[p] "/" directory "/" file;
       if ( system("/bin/sh -c '[[ -f " target " ]]'") == 0) {
	   if (system("ln -s " adj target " " loc)) {
	       print "link: can't link " adj target " <- " loc;
	       exit(1);
	   }
       }
       else {
	   target = cvsroot "/" scope "/" directory;
	   if ( system("/bin/sh -c '[[ -f " target " ]]'") == 0) {
	       if (system("ln -s " adj target " " loc)) {
		   print "link: can't link " adj target " <- " loc;
		   exit(1);
	       }
	   }
       }
   }
}

function mkImport(platform, project, directory,
		  ret, files, dirs, d, locs, ll, ff, f, lf, target) {
   dirs[0] = cvsroot "/" project "/private/" directory;
   locs[0] = platform "/" directory;
   adj[0] = "../../";

   dirs[1] = cvsroot "/" project "/private/" platform "/" directory;
   locs[1] = platform "/" directory;
   adj[1] = "../../";

   dirs[2] = cvsroot "/" project "/public/" directory;
   locs[2] = platform "/public/" directory;
   adj[2] = "../../../";

   dirs[3] = cvsroot "/" project "/public/" platform "/" directory;
   locs[3] = platform "/public/" directory;
   adj[3] = "../../../";

   for (d in dirs) {
       ret = system("/bin/sh -c '[[ -d " dirs[d] " ]]'");
       if (ret == 0) {
	   files = findFiles(dirs[d]);
	   if (files!="") {
	       if ( system("/bin/sh -c '[[ -d " locs[d] " ]]'")) {
		   if ( system("mkdir -p " locs[d]) ) {
		       print "can't make directory: " locs[d];
		       exit(1);
		   }
	       }

	       split(files, ff, ":");
	       for (f in ff) {
		   #
		   # look for existing file...
		   #
		   lf = locs[d] "/" ff[f];
		   ret = system("/bin/sh -c '[[ -h " lf " ]]'");
		   if (ret==0) {
		       if (system("rm -f " lf)) {
			   print "can't remove old file: " lf;
			   exit(1);
		       }
		   }

		   target = adj[d] dirs[d] "/" ff[f];
		   if (system("ln -s " target " " lf)) {
		       print "can't link file: " target " <- " lf;
		       exit(1);
		   }
	       }
	   }
       }
   }
}

function chkProject(project,
		    ret, cmd, cvscmd) {
   #
   # check for cvs root directory...
   #
   ret = system("/bin/sh -c '[[ -d " cvsroot " ]]'");
   if (ret) {
      print "mkdir -p " cvsroot;
      system("mkdir -p " cvsroot);
   }

   #
   # check for project...
   #
   ret = system("/bin/sh -c '[[ -d " cvsroot "/" project " ]]'");
   if (ret) {
      #
      # check it out -- if it doesn't exist yet...
      #
      cvscmd = "cvs -d " cvslogin " checkout " project;
      cmd = "(export CVS_RSH=ssh; cd " cvsroot "; " cvscmd ")";
      system("/bin/sh -c '" cmd "'");
   }

   return isDir(cvsroot "/" project);
}

function isDir(dir) { return system("/bin/sh -c '[[ -d " dir " ]]'")==0; }

#
# find all files and return colon separated list...
#
function findFiles(directory, 
		   cmd, files, name, filename) {
   cmd = "find " directory " -type f -maxdepth 1 -print";

   while ( (cmd | getline filename)>0 ) {
      bcmd = "basename " filename;
      bcmd | getline name;
      close(bcmd);

      if (files=="") { files = name; }
      else if (!isInColonList(files, name)) {
	  files = files ":" name;
      }
   }
   close(cmd);
   return files;
}

#
# find all directories and return them as a colon seperated list...
#
function findDirs(platform, project, 
		  cmd, bcmd, ldirs, name) {
   cmd = "find " cvsroot "/" project " -mindepth 2 -type d -print";
   cmd = cmd " | grep -v '\\/CVS$'";
   
   while ( (cmd | getline filename)>0 ) {
      bcmd = "basename " filename;
      bcmd | getline name;
      close(bcmd);
      if (!isPlatform(name)) {
	 if (ldirs==NULL) {
	    ldirs = name;
	 }
	 else if (!isInColonList(ldirs, name)) {
	    ldirs = ldirs ":" name;
	 }
      }
   }
   close(cmd);
   return ldirs;
}

function isProject(project) {
   return system("/bin/sh -c '[[ -d " cvsroot "/" project " ]]'")==0;
}

function isPlatform(platform, 
		    pp) {
   for (pp in platforms) if (platforms[pp]==platform) return 1;
   return 0;
}

function isInColonList(ldirs, mdir, 
		     sdirs, sd) {
   split(ldirs, sdirs, ":");
   for (sd in sdirs) if (sdirs[sd]==mdir) return 1;
   return 0;
}

END {
   for (p in platforms) {
       loc = platforms[p] "/bin";
       if ( system("/bin/sh -c '[[ -d " loc " ]]'") ) {
	   if ( system("mkdir -p " loc) ) {
	       print "can't create directory: " loc;
	       exit(1);
	   }
       }
       loc = platforms[p] "/lib";
       if ( system("/bin/sh -c '[[ -d " loc " ]]'") ) {
	   if ( system("mkdir -p " loc) ) {
	       print "can't create directory: " loc;
	       exit(1);
	   }
       }
   }
}

END {
   for (p in platforms) {
       loc = platforms[p] "/bin";
       if ( system("/bin/sh -c '[[ -d " loc " ]]'") ) {
	   if ( system("mkdir -p " loc) ) {
	       print "can't create directory: " loc;
	       exit(1);
	   }
       }

       loc = platforms[p] "/lib";
       if ( system("/bin/sh -c '[[ -d " loc " ]]'") ) {
	   if ( system("mkdir -p " loc) ) {
	       print "can't create directory: " loc;
	       exit(1);
	   }
       }
   }
}
