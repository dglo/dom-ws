import java.net.URLClassLoader;
import java.net.URL;
import java.lang.reflect.Method;

public class dh {
   static public void main(String [] args) {
      Object [] al = new Object[1];
      al[0] = args;

      try {
	String [] tools = {
		"ant.jar", "jas.jar", "jython.jar", "xercesImpl.jar",
		"commons-logging.jar", "jboss-jmx.jar", "log4j.jar",
	        "xml-apis.jar", 
                "hep.jar", "jboss-system.jar", "optional.jar",
                "icecube-tools.jar", "junit.jar", "xalan.jar"
	};

	 URL [] urls = new URL[2 + tools.length];
	 String srv = "http://glacier.lbl.gov/~arthur/jars/";
	 urls[0] = new URL(srv + "domhub-common.jar");
	 urls[1] = new URL(srv + "domhub.jar");
	 for (int i=0; i<tools.length; i++) {
             urls[i+2] = new URL(srv + tools[i]);
	 }
	 URLClassLoader ucl = new URLClassLoader(urls);
	 Class cc = ucl.loadClass("icecube.daq.domhub.DOMHub");

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
	 System.out.println("unable to load domhub: " + ex);
	 ex.printStackTrace();
      }
   }
}
