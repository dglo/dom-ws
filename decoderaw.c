/* decoderaw.c, decode raw fpga data...
 */
#include <stdio.h>
#include <string.h>

static void dumpPacked(FILE *fptr, unsigned *buf, int n) {
   int i;

   for (i=0; i<n; i++) {
      const unsigned v = buf[i];
      fprintf(fptr, "%hu %hu%c", v&0x3ff, (v>>16)&0x3ff, 
              (i==n-1) ? '\n' : ' ');
   }
}

static void dumpATWD(FILE *fptr, unsigned *buf, int ch) {
   dumpPacked(fptr, buf+0x210/sizeof(unsigned) + ch*64, 64);
}

static void dumpFADC(FILE *fptr, unsigned *buf) {
   dumpPacked(fptr, buf+0x10/sizeof(unsigned), 128);
}

int main(int argc, char *argv[]) {
   FILE *fadc=NULL, *ch0=NULL, *ch1=NULL, *ch2=NULL, *ch3=NULL;
   int ai=1, hdr=0;
   
   for (ai=1; ai<argc; ai++) {
      if (strcmp(argv[ai], "-fadc")==0 && ai + 1 < argc) {
         if ((fadc=fopen(argv[ai+1], "w"))==NULL) {
            fprintf(stderr, "decoderaw: unable to open: %s\n", argv[ai+1]);
            return 1;
         }
         ai++;
      }
      else if (strcmp(argv[ai], "-ch0")==0 && ai + 1 < argc) {
         if ((ch0=fopen(argv[ai+1], "w"))==NULL) {
            fprintf(stderr, "decoderaw: unable to open: %s\n", argv[ai+1]);
            return 1;
         }
         ai++;
      }
      else if (strcmp(argv[ai], "-ch1")==0 && ai + 1 < argc) {
         if ((ch1=fopen(argv[ai+1], "w"))==NULL) {
            fprintf(stderr, "decoderaw: unable to open: %s\n", argv[ai+1]);
            return 1;
         }
         ai++;
      }
      else if (strcmp(argv[ai], "-ch2")==0 && ai + 1 < argc) {
         if ((ch2=fopen(argv[ai+1], "w"))==NULL) {
            fprintf(stderr, "decoderaw: unable to open: %s\n", argv[ai+1]);
            return 1;
         }
         ai++;
      }
      else if (strcmp(argv[ai], "-ch3")==0 && ai + 1 < argc) {
         if ((ch3=fopen(argv[ai+1], "w"))==NULL) {
            fprintf(stderr, "decoderaw: unable to open: %s\n", argv[ai+1]);
            return 1;
         }
         ai++;
      }
      else if (strcmp(argv[ai], "-h")==0) {
         hdr=1;
      }
      else {
         fprintf(stderr, "usage: decoderaw -fadc file -ch[0-3] file\n");
         return 1;
      }
   }
   
   while (!feof(stdin) && !ferror(stdin)) {
      unsigned buf[2048/sizeof(unsigned)];
      const int nr = fread(buf, 
                           sizeof(unsigned), sizeof(buf)/sizeof(unsigned), 
                           stdin);

      /* eof... */
      if (nr==0) break;
      
      /* partial read... */
      if (nr!=sizeof(buf)/sizeof(unsigned)) {
         fprintf(stderr, "decoderaw: warning: partial read\n");
         break;
      }

      if (hdr) {
         unsigned long long ts = 
            ((unsigned long long) buf[1]<<16 ) | 
            ((unsigned long long) buf[0]&0xffff);
         printf("%d %llu %04hx %hd", buf[0]>>16, ts, buf[2]&0xffff,
                buf[3]&0xffff);
	 if (buf[2]&(1<<18)) {
            printf(" ATWD%c:%d", (buf[2]&(1<<16)) ? 'B' : 'A', (buf[2]>>19)&3);
         }
         if (buf[2]&(1<<17)) printf(" FADC");
         if (buf[2]&(1<<24)) printf(" LCDOWN");
         if (buf[2]&(1<<25)) printf(" LCUP");
         printf("\n");
      }

      if (fadc!=NULL) dumpFADC(fadc, buf);
      if (ch0!=NULL)  dumpATWD(ch0, buf, 0);
      if (ch1!=NULL)  dumpATWD(ch1, buf, 1);
      if (ch2!=NULL)  dumpATWD(ch2, buf, 2);
      if (ch3!=NULL)  dumpATWD(ch3, buf, 3);
   }

   return 0;
}
