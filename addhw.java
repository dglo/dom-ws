import java.net.URLClassLoader;
import java.net.URL;
import java.lang.reflect.Method;

public class addhw {
   static public void main(String [] args) {
      Object [] al = new Object[1];
      al[0] = args;

      try {
	 URL [] urls = new URL[6];
	 String srv = "http://glacier.lbl.gov/~arthur/jars/";
         urls[0] = new URL(srv + "icecube-tools.jar");
	 urls[1] = new URL(srv + "xalan.jar");
	 urls[2] = new URL(srv + "xmi-apis.jar");
	 urls[3] = new URL(srv + "xercesImpl.jar");
	 urls[4] = new URL(srv + "daq-db.jar");
	 urls[5] = new URL(srv + "mysql-connector-java.jar");
	 URLClassLoader ucl = new URLClassLoader(urls);
	 Class cc = ucl.loadClass("icecube.daq.db.app.AddHardware");

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
      catch (java.lang.reflect.InvocationTargetException ite) {
	 System.out.println("invocation target exception: " + ite);
      }
      catch (Exception ex) {
	 System.out.println("unable to load AddResult: " + ex);
      }
   }
}
