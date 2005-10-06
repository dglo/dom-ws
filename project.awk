BEGIN { lib=level=tools=pkgs=0; }

/^\(/ { level++; }
/^\)/ { level--; }

/^\(libs$/ { lib++; }
/^\)libs$/ { lib--; }

/^\(tools$/ { tools++; }
/^\)tools$/ { tools--; }

/^\(packages$/ { pkgs++; }
/^\)packages$/ { pkgs--; }

/^-/ {
   data=substr($0, 2);
   
   if (data!="") {
      if (level==2 && pkgs>0) {
         print "package\t" data;
      }
      if (level==3) {
         if (lib==1) {
            print "lib\t" data;
         }
         else if (tools==1) {
            print "tools\t" data;
         }
      }
      else if (level==4) {
         if (lib==1) {
            print "test-lib\t" data;
         }
         else if (tools==1) {
            print "test-tools\t" data;
         }
      }
   }
}
