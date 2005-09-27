#
# mkws.awk, make a workspace...
#
BEGIN {
   cvslogin = ":ext:glacier.lbl.gov:/home/icecube/cvsroot";
   cvsroot = "..";
   print "creating...";
}

/^cvsroot[ \t]/ {
   cvsroot = $2;
}

/^cvslogin[ \t]/ {
   cvslogin = $2;
}

/^platforms[ \t]/ {
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
	  mkImport(platforms[p], $2, dirs[d]);
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
       if (lscope=="private") {
	   adj = "../../";
	   ploc = platforms[p] "/" location "/";
       }
       else {
	   adj = "../../../";
	   ploc = platforms[p] "/public/" location "/";
       }

       #
       # make sure location exists...
       #
       if ( system("mkdir -p " ploc) ) {
	   print "link: can't create directory: " loc;
	   exit(1);
       }

       #
       # get target path
       #
       ptarget = cvsroot "/" project "/" scope "/";
       ptarget = ptarget platforms[p] "/" directory;
       if ( system("/bin/sh -c '[[ -d " ptarget " ]]'") != 0) {
	   ptarget = cvsroot "/" project "/" scope "/" directory;
       }
       
       if (file== "*") {
	   #
	   # ls all files in this directory and link them
	   # to target, if there are dups, we get an error!
	   #
	   ff = findFiles(ptarget, adj);
       }
       else {
	   ff = adj ptarget "/" file;
	   if ( system("/bin/sh -c '[[ -f " ptarget "/" file " ]];'") ) {
	       ff = "";
	   }
       }

       if ( ff!="") {
	   lncmd = "ln -f -s " ff " " ploc;
#	   print "link: " lncmd;
	   if ( system(lncmd) ) {
	       print "can't link " ff " -> " ploc;
#	       exit(1);
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

   for (d=0; d<4; d++) {
       ret = system("/bin/sh -c '[[ -d " dirs[d] " ]]'");
       if (ret == 0) {
	   files = findFiles(dirs[d], adj[d]);
	   if (files!="") {
	       if ( system("mkdir -p " locs[d]) ) {
		   print "can't make directory: " locs[d];
		   exit(1);
	       }

	       lncmd = "ln -f -s " files " " locs[d];
#	       print "import: " lncmd;
	       if ( system(lncmd) ) {
		   print "can't link " files " <- " locs[d];
		   exit(1);
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
function findFiles(directory, adj,
                   cmd, files, name, filename) {
   cmd = "find " directory " -type f -maxdepth 1 -print";

   while ( (cmd | getline filename)>0 ) {
      files=files " " adj filename
   }
   close(cmd);
   return files;
}

#
# find all directories and return them as a colon separated list...
#
function findDirs(platform, project, 
		  cmd, ldirs) {
   cmd = "find " cvsroot "/" project " -mindepth 2 -type d -print";

   cmd = cmd " | sed 's/.*\\///g'";
   cmd = cmd " | grep -v '^CVS$'";
   for (pp in platforms) cmd = cmd " | grep -v '^" platforms[pp] "$'";
   cmd = cmd " | sort | uniq | tr '\\n' ':' | sed 's/:$//1'";
   cmd | getline ldirs;
   close(cmd);
   return ldirs;
}

function isProject(project) {
   return system("/bin/sh -c '[[ -d " cvsroot "/" project " ]]'")==0;
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








