#include <stdio.h>
#include <string.h>

#include "hal/DOM_MB_fpga.h"
#include "hal/DOM_MB_pld.h"

static const char *timeString(void) {
   unsigned long long clk = hal_FPGA_DOMAPP_get_local_clock();
   return "0x123412345678";
}

static inline int send(const char *b, int n) { 
   return hal_FPGA_send(0, n, b);
}

static inline int receive(char *b) { 
   int nr, type;
   hal_FPGA_receive(&type, &nr, b);
   return nr;
}

static void moniMsg(void) {
   char msg[4092];
   int len = snprintf(msg, sizeof(msg), "MONI time=%s", timeString());

#define ADDADC(a) \
   len += snprintf(msg + len, sizeof(msg) - len, \
                   " %s=%d", #a, halReadADC(DOM_HAL_##a))

   ADDADC(ADC_VOLTAGE_SUM);
   ADDADC(ADC_5V_POWER_SUPPLY);
   ADDADC(ADC_PRESSURE);
   ADDADC(ADC_5V_CURRENT);
   ADDADC(ADC_3_3V_CURRENT);
   ADDADC(ADC_2_5V_CURRENT);
   ADDADC(ADC_1_8V_CURRENT);
   ADDADC(ADC_MINUS_5V_CURRENT);
   ADDADC(ADC_DISC_ONESPE);
   ADDADC(ADC_1_8V_POWER_SUPPLY);
   ADDADC(ADC_2_5V_POWER_SUPPLY);
   ADDADC(ADC_3_3V_POWER_SUPPLY);
   ADDADC(ADC_DISC_MULTISPE);
   ADDADC(ADC_FADC_0_REF);
   ADDADC(ADC_SINGLELED_HV);
   ADDADC(ADC_ATWDA_TRIGGER_BIAS_CURRENT);
   ADDADC(ADC_ATWDA_RAMP_TOP_VOLTAGE);
   ADDADC(ADC_ATWDA_RAMP_BIAS_CURRENT);
   ADDADC(ADC_ANALOG_REF);
   ADDADC(ADC_ATWDB_TRIGGER_BIAS_CURRENT);
   ADDADC(ADC_ATWDB_RAMP_TOP_VOLTAGE);
   ADDADC(ADC_ATWDB_RAMP_BIAS_CURRENT);
   ADDADC(ADC_PEDESTAL);
   ADDADC(ADC_FE_TEST_PULSE_AMPL);

#undef ADDADC
   
   len += snprintf(msg + len, sizeof(msg) - len,
                   " baseHV=%d", halReadBaseADC());

   /* FIXME: use last read temp to avoid delays? */
   len += snprintf(msg + len, sizeof(msg) - len,
                   " temperature=%d", halReadTemp());
   msg[len] = '\n';
   send(msg, len+1);
}

static void dataMsg(void) {
   /* if no data waiting, is it time to readout a temperature value? */

   /* if no trigger set -- return 0 */
   send("DATA 0\n", 7);

   /* wait for data... */
}

static char *parseNameVal(char *b, char *nm, int nlen, char *val, int vlen) {
   int i;

   /* skip whitespace... */
   while (*b && (*b==' ' || *b=='\t') ) b++;

   /* slurp up name... */
   memset(nm, 0, nlen);
   i=0;
   while (*b && *b!='=' && i<nlen-1) { nm[i]=*b; b++; i++; }
   while (*b && *b!='=') { b++; }
   if (*b=='=') b++;
   
   /* slurp up value... */
   memset(val, 0, vlen);
   i=0;
   while (*b && *b!=' ' && *b!='\t' && i<vlen-1) { val[i]=*b; b++; i++; }
   while (*b && *b!=' ' && *b!='\t') b++;
   
   return b;
}

static int setConf(const char *name, const char *val) {
   if (strcmp(name, "hi")==0) {
      return 0;
   }
   return 1;
}

static const char *fpgaType(DOM_HAL_FPGA_TYPES type) {
   if (type==DOM_HAL_FPGA_TYPE_STF_COM) { return "stf"; }
   else if (type==DOM_HAL_FPGA_TYPE_DOMAPP) { return "domapp"; }
   else if (type==DOM_HAL_FPGA_TYPE_CONFIG) { return "configboot"; }
   else if (type==DOM_HAL_FPGA_TYPE_ICEBOOT) { return "iceboot"; }
   else if (type==DOM_HAL_FPGA_TYPE_STF_NOCOM) { return "stf-no-comm"; }
   return "unknown";
}

static void versMsg(void) {
   char b[512];
   int len = snprintf(b, sizeof(b), "VERS");
   len += snprintf(b + len, sizeof(b) - len, " sw-build=%s",
                   ICESOFT_BUILD);
   len += snprintf(b + len, sizeof(b) - len, " fpga-type=%s",
                   fpgaType(hal_FPGA_query_type()));
   len += snprintf(b + len, sizeof(b) - len, " fpga-build=%d",
                   hal_FPGA_query_build());
   len += snprintf(b + len, sizeof(b) - len, " fpga-matches=%s",
                   hal_FPGA_query_versions(DOM_HAL_FPGA_TYPE_DOMAPP,
                                           DOM_HAL_FPGA_COMP_ALL)?"no":"yes");
   b[len]='\n';
   send(b, len+1);
}

static void confMsg(char *buf) {
   char msg[4092];
   int len = snprintf(msg, sizeof(msg), "CONF time=%s", timeString());

   /* parse name value pairs and output old/new values... */
   while (*buf) {
      char name[128], val[128];
      
      /* parse name/value... */
      buf = parseNameVal(buf, name, sizeof(name), val, sizeof(val));

      /* set the value... */
      if (setConf(name, val)==0) {
         len += snprintf(msg + len, sizeof(msg) - len, " %s=%s", name, val);
      }
      else {
         int n = snprintf(msg, sizeof(msg), 
                          "EXCP unable to set '%s' to '%s'\n", name, val);
         send(msg, n);
         return;
      }
   }
   msg[len] = '\n';
   send(msg, len+1);
}

int main(void) {
   while (1) {
      char buf[4096];
      int nr = receive(buf);
      
      if (nr<=0) {
         char msg[] = "EXCP unable to receive message\n";
         send(msg, sizeof(msg));
      }
      else {
         char *eol = strchr(buf, '\n'); /* get end of header marker */
         
         if (eol==NULL) {
            char msg[] = "EXCP invalid header -- no eol terminator\n";
            send(msg, sizeof(msg));
         }
         else {
            *eol = 0; /* terminate header string... */
            if (memcmp(buf, "MONI", 4)==0) {
               moniMsg();
            }
            else if (memcmp(buf, "DATA", 4)==0) {
               dataMsg();
            }
            else if (memcmp(buf, "CONF", 4)==0) {
               confMsg(buf+4);
            }
            else if (memcmp(buf, "VERS", 4)==0) {
               versMsg();
            }
            else if (memcmp(buf, "REBT", 4)==0) {
               /* FIXME: reboot... */
               return 0;
            }
            else if (memcmp(buf, "ECHO", 4)==0) {
               send("ECHO\n", 5);
               while (1) {
                  char b[4092];
                  const int nr = receive(b);
                  send(b, nr);
               }
            }
            else {
               char msg[] = "EXCP invalid header -- unrecognized message\n";
               send(msg, sizeof(msg));
            }
         }
      }
   }

   return 0;
}
