import java.net.URLClassLoader;
import java.net.URL;
import java.lang.reflect.Method;

public class alloff {
   static public void main(String [] args) {
      Object [] al = new Object[1];
      al[0] = args;

      try {
	 URL [] urls = new URL[5];
	 String srv = "http://glacier.lbl.gov/~arthur/jars/";
	 urls[0] = new URL(srv + "domhub-common.jar");
	 urls[1] = new URL(srv + "domhub.jar");
	 urls[2] = new URL(srv + "stfclient.jar");
	 urls[3] = new URL(srv + "icebucket.jar");
	 urls[4] = new URL(srv + "stf.jar");
	 URLClassLoader ucl = new URLClassLoader(urls);
	 Class cc = ucl.loadClass("icecube.daq.stf.AllOff");

	 /* go through methods until we find main...
	  *
	  * FIXME: use getMethod() instead
	  */
	 Method [] ms = cc.getMethods();
	 for (int i=0; i<ms.length; i++) {
	    if (ms[i].getName().equals("main")) {
	       ms[i].invoke(null, al);
	       break;
	    }
	 }
      }
      catch (Exception ex) {
	 System.out.println("unable to load stfmode: " + ex.getMessage());
      }
   }
}
