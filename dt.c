#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sched.h>
#include <signal.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/pci.h>
#include <sys/io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

/* dor registers... */
#define CTRL  (0*4)
#define GSTAT (1*4)
#define DSTAT (2*4)
#define TTSIC (3*4)
#define RTSIC (4*4)
#define INTEN (5*4)
#define DOMS  (6*4)
#define MRAR  (7*4)
#define MRTC  (8*4)
#define MWAR  (9*4)
#define MWTC  (10*4)
#define UTCRD0 (11*4)
#define UTCRD1 (12*4)
#define CURL  (13*4)
#define DCUR  (14*4)
#define FLASH (15*4)
#define MBF   (16*4)
#define DOMC  (24*4)
#define TCSIC (26*4)
#define TCBUF (27*4)
#define DOMID (29*4)
#define DCREV (30*4)
#define FREV  (31*4)

/* 2 wire pairs per cable, 2 doms per wire pair max */
#define MAXCABLES (1)
#define MAXWIRES  (MAXCABLES*2)
#define MAXDOMS   (MAXWIRES*2)

static int getCard(int dor);
static const char *getSignalFile(int dor);


static int confGetByte(const unsigned char *conf, int offset) {
  return conf[offset];
}

static int confGetWord(const unsigned char *conf, int offset) {
  return (confGetByte(conf, offset+1)<<8) | confGetByte(conf, offset);
}

static int confGetDWord(const unsigned char *conf, int offset) {
  return (confGetWord(conf, offset+2)<<16) | confGetWord(conf, offset);
}

static void commReset(int dor, int mask) {
  int i;
  unsigned ctrl;

  for (i=0; i<MAXDOMS; i++) {
    if (mask&(1<<i)) {
      /* clear msg received... */
      outl(1<<(16+i), dor+RTSIC);
      
      /* send comm reset... */
      outl(1<<(8+i), dor+DOMC);
    }
  }
  
  /* wait for comm reset... */
  poll(NULL, 0, 2000);
  
  /* clear comm reset bits... */
  ctrl = inl(dor+CTRL);
  outl(ctrl|(1<<20), dor+CTRL);
  poll(NULL, 0, 1);
  outl(ctrl, dor+CTRL);
}

static void resetWirePairs(int dor) {
  /* clear domc */
  outl(0, dor+DOMC);

  /* wire pair reset */
  outl((inl(dor+CTRL) & ~(0xf<<4))|(0xf<<4), dor+CTRL);

  /* clear wire pair reset */
  outl((inl(dor+CTRL) & ~(0xf<<4)), dor+CTRL);

  /* if power is on, we need to issue comm reset... */
  if ( (inl(dor+CTRL) & 0xf) != 0 ) commReset(dor, 0xff);
}

static int reloadFPGA(int dor, const char *dev, int page) {
  int fd = open(dev, O_RDWR);
  unsigned char conf[256];
  int i;

  if (fd<0) {
    perror("open dor config space");
    return 1;
  }

  /* check for required page... */
  if (page<0 || page>3) {
    fprintf(stderr, "dt: invalid page: %d (expecting 0..3)\n", page);
    return 1;
  }

  /* first -- get the current configuration */
  if (read(fd, conf, sizeof(conf))<0) {
    perror("read configuration");
    close(fd);
    return 1;
  }

  /* now set new fpga -- ... */
  outl(0x04000000|(page<<24), dor + CTRL);

  /* wait for flash to cp */
  poll(NULL, 0, 500);
  
  for (i=0; i<20; i++) {
    char nconf[256]; /* new configuration read... */
    
    /* wait for it to come back... */
    if (lseek(fd, 0, SEEK_SET)<0 || read(fd, nconf, sizeof(nconf))<0) {
      perror("read new configuration");
      close(fd);
      return -1;
    }
    
    /* are we back yet? */
    if (confGetWord(nconf, PCI_VENDOR_ID)==0x1234 &&
	confGetWord(nconf, PCI_DEVICE_ID)==0x5678) break;
    
    /* wait 1/2 second */
    poll(NULL, 0, 500);
  }

  if (i==20) {
    fprintf(stderr, "dt: timeout waiting for fpga to reload...\n");
    close(fd);
    return -1;
  }
  
  /* reset configuration space... */
  if (lseek(fd, 0, SEEK_SET)<0 || write(fd, conf, sizeof(conf))<0) {
    perror("write configuration");
    close(fd);
    return -1;
  }
  
  /* setup ctrl */
  outl(0, dor+CTRL);
  poll(NULL, 0, 500);
  outl(0xc0000000, dor+CTRL);
  
  /* reset wire pairs... */
  resetWirePairs(dor);

  return 0;
}

/* configure DOR.
 *
 *   make sure the device exists and is a dor card.
 *   set bar0?
 *   enable io access
 *
 * FIXME: maybe we should use pciutils here?  for now we'll assume
 * linux 2.4.4+ and just map things directly...
 */
static int confDOR(const char *dev) {
  int fd = open(dev, O_RDWR);
  unsigned char conf[256];
  unsigned bar0;
  unsigned short cmd;
  int base;

  if (fd<0) {
    perror("open configuration");
    return -1;
  }

  if (read(fd, conf, sizeof(conf))!=sizeof(conf)) {
    perror("read configuration");
    close(fd);
    return -1;
  }

  if (confGetWord(conf, PCI_VENDOR_ID)!=0x1234 ||
      confGetWord(conf, PCI_DEVICE_ID)!=0x5678) {
    fprintf(stderr, "dt: device '%s' is not a dor card!\n", dev);
    close(fd);
    return -1;
  }

  bar0 = confGetDWord(conf, PCI_BASE_ADDRESS_0);
  if ( (bar0&PCI_BASE_ADDRESS_SPACE_IO) == 0) {
    fprintf(stderr, "dt: we only support io remapping\n");
    close(fd);
    return -1;
  }
  base = bar0&0xfffc;

  cmd = PCI_COMMAND_IO;
  if (lseek(fd, PCI_COMMAND, SEEK_SET)<0 || write(fd, &cmd, sizeof(cmd))<0) {
    perror("write pci_command");
    close(fd);
    return -1;
  }

  /* elevate io permissions level... */
  if (iopl(3)<0) {
    perror("elevate iopl");
    close(fd);
    return -1;
  }

  /* done with configuration space... */
  close(fd);

  return base;
}

static int powerOn(int dor) {
  unsigned ctrl;
  unsigned dstat = inl(dor+DSTAT);

  if ( (dstat&1) == 0) {
    /* not plugged, nothing to do... */
    return 0;
  }
  
  outl(0, dor+DOMC);
  ctrl = inl(dor+CTRL);

  if ( (dstat&0x30)==0 ) {
    ctrl &= ~(0xff);
    outl(ctrl|0xf3, dor+CTRL);
    poll(NULL, 0, 100);
    outl(ctrl|0xf3, dor+CTRL);
    outl(ctrl|3, dor+CTRL);
    dstat = inl(dor+DSTAT);
    if ((dstat&0x30)!=0x30) {
      fprintf(stderr, "dt: power did not come on!\n");
      return 1;
    }

    commReset(dor, 0xff);
  }

  return 0;
}

static unsigned char *rcvPkt(int dor, int dom, int *len, int *type) {
  int n;
  unsigned hdr;
  unsigned char *ptr = "";

  /* wait for pkt recv */
  while ( (inl(dor + RTSIC)&(0x10000<<dom)) == 0 ) ;

  /* get length */
  hdr = inl(dor + MBF + dom*4);

  n = *len = hdr&0xffff;
  *type = (hdr&0xff)>>16;

  if (n>0) {
    unsigned *buf = (unsigned *) malloc(((n+3)/4) * sizeof(unsigned));

    /* slurp up data */
    insl(dor+MBF+dom*4, buf, (n+3)/4);

    ptr = (unsigned char *) buf;
  }

  /* ack */
  outl(0x10000<<dom, dor+RTSIC);

  return ptr;
}


static int sendPkt(int dor, int dom, 
		   int len, int type, const unsigned char *data) {
  const int nw = 1 + (len+3)/4;
  unsigned *pkt = (unsigned *) malloc( sizeof(unsigned) * nw);
  unsigned char *pkta = (unsigned char *) pkt;

  if (len>500) {
    fprintf(stderr, "dt: sendPkt: packet too big: %d (max is %d)\n",
	    len, 500);
    return 1;
  }

  /* packetize data (assume little endian) */
  pkt[nw-1] = 0;
  pkt[0] = (0<<24) | ((type&0xff)<<16) | (len&0xffff);
  memcpy(pkta + 4, data, len);

  /* wait for room */
  while ((inl(dor+TTSIC)&(1<<dom)) == 0) ;

  /* send pkt */
  outsl(dor+MBF+dom*4, pkt, nw);

  /* clean up */
  free(pkt);

  return 0;
}

static void clrPkts(int dor) {
  int i;

  /* read any previous messages... */
  for (i=0; i<MAXDOMS; i++) {
    while ((inl(dor+RTSIC)&(0x10000<<i)) != 0 ) {
      int len, type;
      unsigned char *pkt = rcvPkt(dor, i, &len, &type);
      free(pkt);
    }
  }
}

/* t2 - t1 in s */
static double diffsec(const struct timeval *t1, const struct timeval *t2) {
  long long usec1 = t1->tv_usec + t1->tv_sec*1000000LL;
  long long usec2 = t2->tv_usec + t2->tv_sec*1000000LL;
  return (double) (usec2 - usec1)*1e-6;
}

static int echoTest(int dor, int pktSize, int npackets) {
  const unsigned doms = inl(dor+DOMS);
  int i;
  struct PktInfo {
    int dom; /* dom who holds this packet */
    int num; /* packet number (per dom) */
    int len; /* packet length */
    void *data; /* packet data */
  } pkts[128];
  const int npkts = sizeof(pkts)/sizeof(pkts[0]);

  struct WireInfo {
    long long nbytes;  /* bytes transfered */
    int nrcv;          /* number of packets left to rcv on this pair */
    struct timeval start, end; /* start and end times */
  } wireInfo[MAXWIRES];

  struct {
    int isAlive;  /* this dom exists? */
    int nsnd;     /* number of pkts left to send */
    int nrcv;     /* number of pkts left to rcv */
    int nerrors;  /* number of errors */
    long long nbytes; /* number of bytes read */
    struct WireInfo *wire; /* wire info */
    
  } domInfo[MAXDOMS];
  unsigned char dommask = 0;

  for (i=0; i<npkts; i++) pkts[i].dom=-1;

  for (i=0; i<MAXWIRES; i++) { 
    wireInfo[i].nbytes = 0LL; 
    wireInfo[i].nrcv = 0;
  }

  for (i=0; i<MAXDOMS; i++) {
    domInfo[i].isAlive = doms&(1<<i);
    domInfo[i].nsnd = npackets;
    domInfo[i].nrcv = npackets;
    domInfo[i].nerrors = 0;
    domInfo[i].nbytes = 0LL;
    domInfo[i].wire = wireInfo + i/2;
    if (domInfo[i].isAlive) {
      domInfo[i].wire->nrcv+=npackets;
      dommask |= (1<<i);
    }
  }

  if (dommask==0) {
    /* no doms to test... */
    return 0;
  }

  /* wait for iceboot messages to come through... */
  poll(NULL, 0, 500);

  /* clear current msgs */
  clrPkts(dor);

  /* put doms into echo mode... */
  for (i=0; i<MAXDOMS; i++) {
    const unsigned char *msg = (const unsigned char *) "echo-mode\r";
    if (domInfo[i].isAlive) sendPkt(dor, i, strlen(msg), 0, msg);
  }

  /* wait a bit... */
  poll(NULL, 0, 1000);

  for (i=0; i<MAXDOMS; i++) {
    if (domInfo[i].isAlive) {
      const unsigned rtsic = inl(dor+RTSIC);
      unsigned char *b = NULL;
      int len, type;

      if ( (rtsic&(0x10000<<i)) == 0 ) {
	fprintf(stderr, "dt: warning: dom %d did not respond to echo-mode\n",
		i);
	domInfo[i].isAlive = 0;
	continue;
      }

      /* should be one packet waiting... */
      b = rcvPkt(dor, i, &len, &type);
      if (len!=strlen("echo-mode\r\n") || memcmp(b, "echo-mode\r\n", len)!=0) {
	fprintf(stderr, 
		"dt: warning dom %d did not respond properly to echo-mode\n",
		i);
	domInfo[i].isAlive = 0;
	free(b);
	continue;
      }
      free(b);
    }
  }

  /* go! */
  for (i=0; i<MAXWIRES; i++) gettimeofday(&wireInfo[i].start, NULL);

  while (1) {
    unsigned ttsic = inl(dor+TTSIC);
    unsigned rtsic = inl(dor+RTSIC);

    /* no room to send or nothing to rcv? yield cpu */
    while ( ((ttsic | (rtsic>>16)) & dommask) == 0 ) {
      /* sched_yield(); */
      ttsic = inl(dor+TTSIC);
      rtsic = inl(dor+RTSIC);
    }

    /* any room to send? */
    if (ttsic&dommask) {
      for (i=0; i<MAXDOMS; i++) {
	if (domInfo[i].isAlive && domInfo[i].nsnd>0 && (ttsic&(1<<i)) ) {
	  int j;
	  const int len =  (pktSize<=0) ? (1 + (random() % 500)) : pktSize;
	  const int nw = (len+3)/4;
	  unsigned *buf = NULL;

	  /* create a pkt */
	  if (nw>0) {
	    buf = (unsigned *) malloc(sizeof(unsigned)*nw);
	    for (j=0; j<nw; j++) buf[j] = random();
	  }

	  /* send it */
	  sendPkt(dor, i, len, 0, (unsigned char *) buf);

	  /* find a location in pkts */
	  for (j=0; j<npkts && pkts[j].dom!=-1; j++) ;
	  if (j==npkts) {
	    fprintf(stderr, "dt: internal error: can't find empty packet\n");
	    return 1;
	  }

	  pkts[j].dom = i;
	  pkts[j].num = domInfo[i].nsnd;
	  pkts[j].len = len;
	  pkts[j].data = buf;

	  /* decrement nsnd */
	  domInfo[i].nsnd--;

	  /* account for bytes across the wire... */
	  domInfo[i].wire->nbytes+=len;
	}
      }
    }

    /* any pkt to rcv? */
    if ((rtsic>>16)&dommask) {
      for (i=0; i<MAXDOMS; i++) {
	if (domInfo[i].isAlive && domInfo[i].nrcv>0) {
	  if (rtsic&(0x10000<<i)) {
	    /* rcv packet */
	    int j;
	    int type, len;
	    unsigned char *pkt = rcvPkt(dor, i, &len, &type);

	    /* search pkts for the proper one */
	    for (j=0; j<npkts; j++) {
	      if (pkts[j].dom==i && pkts[j].num==domInfo[i].nrcv) break;
	    }
	    
	    if (j==npkts) {
	      fprintf(stderr, "dt: internal error, can't find pkt!\n");
	      return 1;
	    }

	    /* compare pkt data (if error inc dom.nerrors) */
	    if (pkts[j].len!=len || memcmp(pkt, pkts[j].data, len)!=0) {
	      domInfo[i].nerrors++;
	    }

	    /* clear pkts */
	    if (len>0) free(pkt);
	    if (pkts[j].len>0) {
	      free(pkts[j].data);
	      pkts[j].data = NULL;
	    }
	    pkts[j].dom = -1;
	    pkts[j].len = 0;

	    /* decrement nrcvs */
	    domInfo[i].nbytes+=len;
	    domInfo[i].nrcv--;
	    domInfo[i].wire->nrcv--;
	    domInfo[i].wire->nbytes+=len;
	    if (domInfo[i].wire->nrcv==0) {
	      gettimeofday(&domInfo[i].wire->end, NULL);
	    }
	  }
	}
      }
      /* are we done yet? */
      {  int nleft = 0;
         for (i=0; i<MAXDOMS; i++) {
	   if (domInfo[i].isAlive) nleft+=domInfo[i].nsnd + domInfo[i].nrcv;
	 }
	 if (nleft==0) break;
      }
    }
  }

  /* echotest domn npackets nerrors kbytes wthroughput */
  for (i=0; i<MAXDOMS; i++) {
    if (domInfo[i].isAlive) {
      printf("echotest\t%d\t%d\t%d\t%d\t%d\n", 
	     i,
	     npackets,
	     domInfo[i].nerrors,
	     (int) (domInfo[i].nbytes/1024),
	     (int) 
	     (domInfo[i].wire->nbytes/
	      diffsec(&domInfo[i].wire->start, &domInfo[i].wire->end)));
    }
  }
  return 0;
}

static int tcalTest(int dor, int mask, int iters) {
  int i, ii;
  int nsel = 0;
  int sel[8];
  
  /* find selected doms... */
  for (i=0; i<8; i++) {
    if ((mask&(1<<i))!=0) {
      sel[nsel] = i;
      nsel++;
    }
  }

  for (i=0; i<iters; i++) {
    unsigned ptr[57];
    const int nptr = sizeof(ptr)/sizeof(ptr[0]);
    unsigned short *dorsamples, *domsamples;
    int j, dptr;
    unsigned long long dortx, dorrx, domtx, domrx;
    
    /* start tcal (FIXME: only one dom at a time?)... */
    outl(mask<<24, dor + DOMC );

    /* wait for tcal done... */
    while ( (inl(dor + DOMC) &0xff000000) != 0 ) sched_yield();
    
    for (ii=0; ii<nsel; ii++) {
      printf("DOM_%d%c_TCAL_round_trip_%05d\n", 
	     sel[ii]/2, (sel[ii]%2) ? 'b' : 'a', i+1);

      /* select proper tcbuf... */
      outl( (inl(dor+CTRL)& ~(7<<12)) | (sel[ii]<<12), dor+CTRL);

      /* get tcal packet ... */
      insl(dor + TCBUF, ptr, nptr);
      
      /* check count... */
      if ((ptr[0]&0xffff)!=56*4) {
	fprintf(stderr, "dt: tcal: invalid tcal packet length!\n");
	return 1;
      }
      dortx = ptr[2]; dortx<<=32; dortx |= ptr[1];
      dorrx = ptr[4]; dorrx<<=32; dorrx |= ptr[3];
      dorsamples = (unsigned short *) (ptr + 5);
      
      printf("dor_tx_time %llu\n", dortx);
      printf("dor_rx_time %llu\n", dorrx);
      for (j=0; j<48; j++) {
	printf("dor_%02d %d\n", j, dorsamples[j]);
      }
      
      dptr = 1 + (nptr-1)/2;
      
      domrx = ptr[ 1 + dptr ]; domrx<<=32; domrx |= ptr[dptr];
      domtx = ptr[ 3 + dptr ]; domtx<<=32; domtx |= ptr[2 + dptr];
      domsamples = (unsigned short *) (ptr + 4 + dptr);
      printf("dom_tx_time %llu\n", domtx);
      printf("dom_rx_time %llu\n", domrx);
      for (j=0; j<48; j++) {
	printf("dom_%02d %d\n", j, domsamples[j]);
      }
    }
  }

  return 0;
}

static int rebootDOMS(int dor, int mask) {
  int ntries = 0;
  unsigned doms = inl(dor+DOMS);

  /* send sys res */
  outl(mask, dor+DOMC);
  
  /* wait for the reset bit to clear... */
  for (ntries=0; ntries<400; ntries++) {
    if ( (inl(dor+DOMC)&0xff) == 0 ) break;
    poll(NULL, 0, 10);
  }
  
  /* make sure they come back */
  doms = inl(dor+DOMS);
  if ( (doms&mask) != mask ) {
    fprintf(stderr, "dt: dom(s) did not come back after reboot!\n");
    if ( ((doms>>8)&mask) != 0 ) {
      fprintf(stderr, "dt: hardware timeout on one of more doms!\n");
    }
    return 1;
  }
  return 0;
}

static void flashReset(int dor) {
  outl(1<<22, dor+FLASH);
  poll(NULL, 0, 1);
  outl(0, dor+FLASH);
  poll(NULL, 0, 1);
}

static int flashReadByte(int dor, int addr) {
  if (addr<0x40000 || addr>=(0x10000*12)) {
    fprintf(stderr, "dt: flash read: invalid address 0x%08x\n",
	    addr);
    return -1;
  }

  /* address flash */
  outl(addr, dor+FLASH);

  /* return data... */
  return (inl(dor+FLASH)>>24)&0xff;
}

static int flashEraseSector(int dor, int sector) {
  int ntries, i;

  if (sector<4 || sector>11) {
    fprintf(stderr, 
	    "dt: ease flash sector: invalid sector %d (expecting 4..11)\n", 
	    sector);
    return 1;
  }

  /* send magic cmd... */
  outl(0xaa100aaa, dor+FLASH);
  outl(0x55100555, dor+FLASH);
  outl(0x80100aaa, dor+FLASH);
  outl(0xaa100aaa, dor+FLASH);
  outl(0x55100555, dor+FLASH);
  outl(0x30100000|(sector<<16), dor+FLASH);

  poll(NULL, 0, 1);

  /* wait for erase done... */
  for (ntries=0; ntries<1000; ntries++) {
    const unsigned fl1 = inl(dor+FLASH);
    const unsigned fl2 = inl(dor+FLASH);
    const unsigned fl3 = inl(dor+FLASH);
    const unsigned fl4 = inl(dor+FLASH);
    if ( (fl1&0x40000000)==(fl2&0x40000000) &&
	 (fl3&0x4000000)==(fl4&0x4000000) ) break;

    poll(NULL, 0, 10);
  }

  if (ntries==1000) {
    fprintf(stderr, "dt: erase flash: timeout waiting for erase!\n");
    return 1;
  }

  /* check ready bit... */
  for (ntries=0; ntries<1000; ntries++) {
    if ( inl(dor+FLASH)&0x80000000 ) break;
    sched_yield();
  }

  if (ntries==1000) {
    fprintf(stderr, "dt: erase flash: timeout waiting for ready bit!\n");
    return 1;
  }

  outl(0, dor+FLASH);

  /* now verify... */
  for (i=0; i<65536; i++) {
    const int byte = flashReadByte(dor, 0x10000*sector + i);
    if (byte<0) {
      fprintf(stderr, "dt: erase flash: verify failed, unable to read\n");
      return 1;
    }
    else if (byte!=0xff) {
      fprintf(stderr, 
	      "dt: erase flash: verify failed, 0x%02x != 0xff (0x%08x)\n",
	      byte, 0x10000*sector+i);
      return 1;
    }
  }

  return 0;
}

static int flashProgramByte(int dor, int addr, int byte) {
  if (addr<0x10000*4 || addr>=(0x10000*12)) {
    fprintf(stderr, "dt: flash program: invalid address 0x%08x\n",
	    addr);
    return 1;
  }

  if (flashReadByte(dor, addr)!=byte) {
    const unsigned flash = (byte<<24) | (1<<20) | addr;
    int ntries;

    outl(0xaa100aaa, dor+FLASH);
    outl(0x55100555, dor+FLASH);
    outl(0xa0100aaa, dor+FLASH);
    outl(flash, dor+FLASH);

    /* wait for DQ7 */
    for (ntries=0; ntries<1000; ntries++) {
      if ( ((inl(dor+FLASH)^flash) & 0x80000000) == 0) break;
    }

    if (ntries==1000) {
      fprintf(stderr, "dt: flash program: timeout waiting for ready\n");
      return 1;
    }

    outl(0, dor+FLASH);
  }

  return 0;
}

static int burnFlash(int dor, int page, const char *rbffile) {
  int fd = open(rbffile, O_RDONLY);
  struct stat st;
  unsigned char *rbf;
  int i, j, sector;

  if (page<1 || page>2) {
    fprintf(stderr, "dt: invalid flash page %d: expecting 1 or 2\n",
	    page);
    close(fd);
    return 1;
  }

  if (fd<0) {
    perror("dt: open rbf file");
    return 1;
  }

  if (fstat(fd, &st)<0) {
    perror("dt: fstat rbf file");
    close(fd);
    return 1;
  }

  if (st.st_size!=246002) {
    fprintf(stderr, 
	    "dt: burnFlash: invalid rbf file size (expecting 246002)!\n");
    close(fd);
    return 1;
  }
  
  if ((rbf=(unsigned char *) mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, 
				  fd, 0))==MAP_FAILED) {
    perror("dt: mmap rbf");
    close(fd);
    return 1;
  }

  /* don't need fd anymore... */
  close(fd);

  flashReset(dor);

  for (sector = page*4; sector < (page+1)*4; sector++) {
    if (flashEraseSector(dor, sector)) {
      fprintf(stderr, "dt: burn flash: can't erase sector %d\n", sector);
      munmap(rbf, st.st_size);
      return 1;
    }
  }

  /* program from rbf */
  for (j=0, sector = page*4; sector < (page+1)*4; sector++) {
    for (i=0; i<0x10000 && j<st.st_size; i++, j++) {
      if (flashProgramByte(dor, sector*0x10000 + i, rbf[j])) {
	fprintf(stderr, "dt: burn flash: can't erase sector %d\n", sector);
	munmap(rbf, st.st_size);
	return 1;
      }
    }
  }

  /* now verify... */
  for (j=0, sector = page*4; sector < (page+1)*4; sector++) {
    for (i=0; i<0x10000 && j<st.st_size; i++, j++) {
      const int byte = flashReadByte(dor, sector*0x10000 + i);
      if (byte!=rbf[j]) {
	fprintf(stderr, 
		"dt: burn flash: can't verify 0x%02x!=0x%02x (0x%08x)\n", 
		byte, rbf[j], sector*0x10000 + i);
	munmap(rbf, st.st_size);
	return 1;
      }
    }
  }

  munmap(rbf, st.st_size);
  return 0;
}

static void killServer(int dor) {
  char cmdline[4096], proc[128];
  pid_t pid;
  int fd, nr;

  if ((fd=open(getSignalFile(dor), O_RDONLY))<0) return;

  /* uh, oh, server might exist! */
  nr=read(fd, &pid, sizeof(pid));
  if (nr!=sizeof(pid)) {
    unlink(getSignalFile(dor));
    return;
  }
  close(fd);

  /* check for process info... */
  snprintf(proc, sizeof(proc), "/proc/%d/cmdline", pid);
  if ((fd=open(proc, O_RDONLY))<0) {
    unlink(getSignalFile(dor));
    return;
  }

  memset(cmdline, 0, sizeof(cmdline));
  nr = read(fd, cmdline, sizeof(cmdline)-1);
  if (nr<3 || strlen(cmdline)<2 ||
      strcmp(cmdline + strlen(cmdline)-2, "dt")!=0)  {
    unlink(getSignalFile(dor));
    return;
  }

  /* well, it's found and it seems to be a dt, lets
   * kill it!
   */
  if (kill(pid, SIGTERM)<0) {
    perror("kill dt server");
  }

  unlink("/tmp/.dt.pid");
}

static const char *getSignalFile(int dor) {
  static int init = 0;
  static char path[128];
  if (init==0) {
    snprintf(path, sizeof(path), "/tmp/.dt.%d.pid", getCard(dor));
    init=1;
  }
  return path;
}

static int signalServer(int dor) {
  int fd;
  int nw;
  pid_t pid = getpid();

  if ((fd=open(getSignalFile(dor), O_CREAT|O_TRUNC|O_WRONLY, 0600))<0) {
    perror("create /tmp/.dt.pid");
    return 1;
  }

  if ((nw=write(fd, &pid, sizeof(pid)))<0) {
    perror("write pid");
    unlink(getSignalFile(dor));
    return 1;
  }
  else if (nw!=sizeof(pid)) {
    unlink(getSignalFile(dor));
    fprintf(stderr, "dt: can't write pid to file!\n");
    return 1;
  }
  close(fd);

  return 0;
}

static int sockListener(int port) {
   struct sockaddr_in server;
   int one = 1;
   int sock = socket(AF_INET, SOCK_STREAM, 0);
   if (sock<0) {
      perror("dt: opening stream socket");
      return -1;
   }

   setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
  
   server.sin_family      = AF_INET;
   server.sin_addr.s_addr = INADDR_ANY;
   server.sin_port        = ntohs(port);
   if (bind(sock, (struct sockaddr *) &server, sizeof server)<0) {
      perror("dt: bind socket");
      close(sock);
      return -1;
   }
  
   /* listen on new client socket...
    */
   if (listen(sock, 1)<0) {
      perror("listen");
      close(sock);
      return -1;
   }

   return sock;
}

/* FIXME: i think this should be better... */
static int closeListener(int fd) {
  if (shutdown(fd, SHUT_RDWR)<0) {
    perror("dt: shutdown listener");
    return 1;
  }
  close(fd);
  return 0;
}

static int getCard(int dor) {
  static int card = -1;
  if (card==-1) {
    card = (inl(dor+DSTAT)&0xf000) >> 12;
  }
  return card;
}

/* negotiate telnet parameters...
 */
static void telnetNeg(int fd) {
   unsigned char buf[3];
   buf[0] = 255;
   buf[1] = 251;
   buf[2] = 1; /* iac will echo ... */
   write(fd, buf, 3);

   buf[0] = 255;
   buf[1] = 251;
   buf[2] = 3; /* suppress go ahead... */
   write(fd, buf, 3);
}

static int scanTelnet(unsigned char *buffer, int len) {
   int i;
   static int iac = 0;
		     
   i = 0;
   while (i<len) {
      if (iac) {
	 if (buffer[i]==0xff) {
	    iac=0;
	    i++;
	 }
	 else {
	    iac++;
	    memmove(buffer+i, buffer+i+1, len-i);
	    len--;
	    if (iac==3) iac=0;
	 }
      }
      else {
	 if (buffer[i]==0xff) {
	    iac = 1;
	    if (i<len-1) {
	       memmove(buffer+i, buffer+i+1, len-i);
	       len--;
	    }
	 }
	 else if (buffer[i]==0) {
	    /* remove nop... */
	    memmove(buffer+i, buffer+i+1, len-i);
	    len--;
	 }
	 else i++;
      }
   }
   return len;
}

static int domservMain(int dor) {
  unsigned doms = inl(dor+DOMS);
  int dl[MAXDOMS];
  int fd[MAXDOMS][2];
  int newstate[MAXDOMS], state[MAXDOMS];
  int ndoms = 0;
  int i;

  /* enumerate doms... */
  for (i=0; i<MAXDOMS; i++) 
    if ( doms&(1<<i) ) { 
      dl[ndoms] = i;
      newstate[ndoms] = 1;
      state[ndoms] = 0;
      ndoms++;
    }

  if (ndoms==0) {
    fprintf(stderr, "dt: dom serv: no doms available!\n");
    return 1;
  }

  /* a dom connection is in one of these states:
   *
   * 0 : uninitialized fd[0] = x, fd[1] = x
   * 1 : awaiting a connection fd[0] = telnet listner, fd[1] = raw listener
   * 2 : telnet connection accepted, fd[0] = telnet socket
   * 3 : raw connection accepted, fd[0] = raw socket
   *
   * transitions:
   *   0 -> 1 : initialization
   *   1 -> 2 : accept a telnet connection
   *   1 -> 3 : accept a raw connection
   *   1 -> 0 : error accepting telnet
   *   2 -> 0 : error reading/writing telnet, telnet closed
   *   3 -> 0 : error reading/writing raw, raw closed
   */
  while (1) {
    struct pollfd fds[MAXDOMS*2];
    int nfds = 0;
    int dommask = 0;
    int ret, j;

    /* setup states... */
    for (i=0; i<ndoms; i++) {

#if defined(VERBOSE)
      if (newstate[i]!=state[i]) 
	printf("[%d]: state -> newstate: %d -> %d\n",
	       i, state[i], newstate[i]);
#endif

      if (newstate[i]==1) {
	if (state[i]==2 || state[i]==3) {
	  /* FIXME: cleanup, close data connection correctly... */
	  close(fd[i][0]);
	}

	/* open listeners... */
	if (state[i]==0 || state[i]==2 || state[i]==3) {
	  const int port = getCard(dor)*8 + dl[i] + 1;
	  fd[i][0] = sockListener(5000+port); /* telnet socket ... */
	  fd[i][1] = sockListener(5500+port); /* raw listener... */
	  if (fd[i][0]<0 || fd[i][1]<0) {
	    fprintf(stderr, "dt: dom serv: disabling dom %d!\n", i);
	    state[i] = newstate[i] = 0;
	  }

	  if (state[i]==2) {
	    /* do telnet neg here... */
	  }
	  state[i] = newstate[i];
	}
      }
      else if (newstate[i]==2 && state[i]==1) {
	/* accept telnet listener... */
	const int nfd = accept(fd[i][0], NULL, NULL);
	
	if (nfd<0) {
	  perror("dt: accept telnet");
	  newstate[i] = 1;
	}
	else {
	  /* close listeners */
	  closeListener(fd[i][0]);
	  closeListener(fd[i][1]);
	  telnetNeg(nfd);
	  fd[i][0] = nfd;
	  state[i] = newstate[i];
	}
      }
      else if (newstate[i]==3 && state[i]==1) {
	/* accept raw listener... */
	const int nfd = accept(fd[i][1], NULL, NULL);
	
	if (nfd<0) {
	  perror("dt: accept telnet");
	  newstate[i] = 1;
	}
	else {
	  /* close listeners */
	  closeListener(fd[i][0]);
	  closeListener(fd[i][1]);
	  fd[i][0] = nfd;
	  state[i] = newstate[i];
	}
      }
    }

    /* now we're in a consistant state, setup listeners... */
    for (i=0; i<ndoms; i++) {
      if (state[i]==1) {
	fds[nfds].fd = fd[i][0]; /* telnet listener*/
	fds[nfds].events = POLLIN;
	nfds++;
	fds[nfds].fd = fd[i][1]; /* raw listener*/
	fds[nfds].events = POLLIN;
	nfds++;
      }
      else if (state[i]==2 || state[i]==3) {
	fds[nfds].fd = fd[i][0]; /* data listener*/
	fds[nfds].events = POLLIN;
	nfds++;

	/* set dom mask here... */
	dommask |= (1<<dl[i]);
      }
    }
    
    /* look for events... */
    if ((ret = poll(fds, nfds, dommask ? 10 : 100))<0) {
      perror("poll");
      return 1;
    }
    else if (ret==0) {
      const unsigned rtsic = inl(dor+RTSIC);
      for (j=0; j<ndoms; j++) {
	if (rtsic&(1<<(16+dl[j]))) {
	  int type, len;
	  void *data = rcvPkt(dor, dl[j], &len, &type);

#if defined(VERBOSE)
	  { int ii;
	    printf("got data: %d: ", len);
	    for (ii=0; ii<len && ii<10; ii++) 
	      printf("%02x ", ((char *)data)[ii]);
	    printf("\n");
	  }
#endif

	  /* got data, pass it on... */
	  if (len>0 && (state[j]==2 || state[j]==3)) {
	    const int nw = write(fd[j][0], data, len);
	    if (nw<0) {
	      perror("dt: unable to write");
	      newstate[j] = 1;
	    }
	    else if (nw==0) {
	      fprintf(stderr, "can't write to socket\n");
	      newstate[j] = 1;
	    }
	    else if (nw!=len) {
	      /* FIXME: iterate here... */
	      fprintf(stderr, "dt: short write!\n");
	      newstate[j] = 1;
	    }
	  }
	  free(data);
	}
      }
    }
    else if (ret>0) {
      /* check for fd events... */
      for (j=0; j<nfds; j++) {
	if (fds[j].revents&POLLIN) {
	  for (i=0; i<ndoms; i++) {
	    if (state[i]==1 && fd[i][0]==fds[j].fd) {
	      newstate[i] = 2; /* move from accept -> telnet */
	    }
	    else if (state[i]==1 && fd[i][1]==fds[j].fd) {
	      newstate[i] = 3; /* move from accept -> raw */
	    }
	    else if ((state[i]==2 || state[i]==3)&& fds[j].fd==fd[i][0]) {
	      char buffer[400]; /* max packet size... */
	      /* ferry telnet/raw data */
	      const int nr = read(fd[i][0], buffer, sizeof(buffer));

	      if (nr<0) {
		perror("dt: read from socket");
		newstate[i] = 1;
	      }
	      else if (nr==0) {
		newstate[i] = 1;
	      }
	      else {
		const int ret = (state[i]==2) ? scanTelnet(buffer, nr) : nr;

#if defined(VERBOSE)
		{ int ii;
 	          printf("read data: %d [%d]: ", nr, ret);
		  for (ii=0; ii<10 && ii<nr; ii++) 
		    printf("0x%02x ", (unsigned char) buffer[ii]);
		  printf("\n");
		}
#endif

		if (ret>0) {
		  if (sendPkt(dor, dl[i], ret, 0, buffer)) {
		    fprintf(stderr, "dt: can't send to dom!\n");
		    newstate[i] = 0;
		  }
		}
	      }
	    }
	  }
	}
      }
    }
  }

  return 0;
}

static int domserv(int dor) {
  pid_t pid;
  unsigned doms = inl(dor+DOMS);

  /* check for doms to serve... */
  if ( (doms&0xff) == 0 ) {
    fprintf(stderr, "dt: dom server: no doms to serve!\n");
    return 1;
  }

  if ((pid=fork())<0) {
    perror("dt: dom server: fork");
    return 1;
  }
  else if (pid==0) {
    int ret;
#if !defined(VERBOSE)
    /* FIXME: figure out if we really want this! */
    if (daemon(1, 0)<0) {
      perror("daemon");
      return 1;
    }
#endif
    signalServer(dor);
    ret = domservMain(dor);
    unlink(getSignalFile(dor));
    exit(ret);
  }

  return 0;
}

/* iceboot -> stf */
static void stfMode(int dor) {
  const unsigned doms = inl(dor+DOMS);
  int i;

  /* any doms to put into stf mode? */
  if ( (doms&0xff) == 0 ) return;

  /* clear any current msgs */
  clrPkts(dor);

  /* send stf */
  for (i=0; i<MAXDOMS; i++) {
    const char *msg = (const unsigned char *) "stf\r";
    if ( (doms&(1<<i)) ) {
      sendPkt(dor, i, strlen(msg), 0, msg);
    }
  }

  poll(NULL, 0, 1500);

  /* clear anything in there... */
  clrPkts(dor);

  /* send a ping... */
  for (i=0; i<MAXDOMS; i++) {
    const char *msg = (const unsigned char *) "OK\r";
    if ( (doms&(1<<i)) ) {
      sendPkt(dor, i, strlen(msg), 0, msg);
    }
  }

  poll(NULL, 0, 500);

  /* make sure we get the pkt back... */
  for (i=0; i<MAXDOMS; i++) {
    if ( inl(dor + RTSIC)&(0x10000<<i) ) {
      int len, type;
      const char *msg = (const unsigned char *) "OK\r\n";
      char *pkt = rcvPkt(dor, i, &len, &type);
      
      if (len==strlen(msg) && strncmp(pkt, msg, len)==0) {
	char hostname[128];
	gethostname(hostname, sizeof(hostname));
	printf("%s\t%d\t%d\t%d\t%d\n",
	       hostname,
	       2500 + getCard(dor) * 8 + i + 1,
	       getCard(dor),
	       i/4,
	       i%2);
      }
      free(pkt);
    }
  }
}

int main(int argc, char *argv[]) {
  char devpath[128];
  int dor;
  int bus, slot, func;
  int ai = 1;

  if ((argc-ai)<1) {
    fprintf(stderr, "usage: dt bus:slot.func [cmds...]\n");
    fprintf(stderr, "  cmds:\n");
    return 1;
  }

  if (sscanf(argv[ai], "%02x:%02x.%1x", &bus, &slot, &func)!=3) {
    fprintf(stderr, "unable to parse bus:slot.func\n");
    return 1;
  }
  ai++;

  snprintf(devpath, sizeof(devpath), "/proc/bus/pci/%02x/%02x.%1x",
	   bus, slot, func);

  if ((dor=confDOR(devpath))<0) {
    fprintf(stderr, "unable to configure dor.\n");
    return 1;
  }

  /* initialize...
   */
  usleep(1000);

  /* check for a server process and kill it if it
   * exists...
   */
  killServer(dor);

  /* commands ... */
  for ( ; ai<argc; ai++) {
    if (strcmp(argv[ai], "pwr")==0 && ai<argc-1) {
      ai++;
      if (strcmp(argv[ai], "on")==0) {
	if (powerOn(dor)) {
	  fprintf(stderr, "dt: unable to power on!\n");
	  return 1;
	}
      }
      else if (strcmp(argv[ai], "off")==0) {
	outl((inl(dor+CTRL)&~0xff)|0xf0, dor+CTRL);
	poll(NULL, 0, 10);
	outl((inl(dor+CTRL)&~0xff), dor+CTRL);
      }
      else {
	fprintf(stderr, "dt: invalid pwr command: '%s'\n", argv[ai]);
	return 1;
      }
    }
    else if (strcmp(argv[ai], "stf")==0) {
      /* put doms in stf mode... */
      stfMode(dor);
    }
    else if (strcmp(argv[ai], "echotest")==0 && ai<argc-2) {
      /* run echo test with pktSize and x packets per dom... */
      echoTest(dor, atoi(argv[ai+1]), atoi(argv[ai+2]));
      ai+=2;
    }
    else if (strcmp(argv[ai], "tcal")==0 && ai<argc-2) {
      const unsigned doms = inl(dor+DOMS);
      int mask = 0;

      if (strcmp(argv[ai+1], "all")==0) {
	mask = 0xff&doms;
      }
      else {
	mask = doms&(1<<atoi(argv[ai+1]));
      }
      ai++;

      if ( mask == 0) {
	fprintf(stderr, "dt: no doms available for tcal!\n");
      }
      else {
	/* get the number of iterations... */
	tcalTest(dor, mask, atoi(argv[ai+1]));
	ai++;
      }
    }
    else if (strcmp(argv[ai], "reboot")==0 && ai<argc-1) {
      unsigned mask = 0;
      unsigned doms = inl(dor+DOMS);
      ai++;
      if (strcmp(argv[ai], "all")==0) {
	mask = (doms&0xff);
      }
      else {
	mask = doms&(1<<atoi(argv[ai]));
      }
      if (mask==0) {
	fprintf(stderr, "dt: warning: dom(s) not avail to reboot!\n");
      }
      else {
	if (rebootDOMS(dor, mask)) {
	  fprintf(stderr, "dt: reboot: unable to reboot one or more doms!\n");
	}
      }
    }
    else if (strcmp(argv[ai], "card")==0) {
      printf("card\t%d\n", getCard(dor));
    }
    else if (strcmp(argv[ai], "doms")==0) {
      const unsigned doms = inl(dor+DOMS);
      int i;
      char domid[32];

      for (i=0; i<MAXDOMS; i++) {
	int j;

	/* is the dom there? */
	if ((doms&(1<<i)) == 0) continue;
	
	/* request the domid... */
	outl( (0x10000<<i), dor+DOMC);

	for (j=0; j<100; j++) {
	  /* wait a bit... */
	  poll(NULL, 0, 10);
	  
	  /* check bit... */
	  if ( (inl(dor+DOMC) & (0x10000<<i)) == 0 ) break;
	}
	
	if (j==100) {
	  fprintf(stderr, "warning: timeout getting domid from dom %d\n", i);
	}
	else {
	  unsigned long long v;
	  memset(domid, 0, sizeof(domid));
	  outl( 0x01000000 | (i<<25), dor+DOMID);
	  v = inl(dor+DOMID)&0xffffff;  v<<=24;
	  outl( 0x00000000 | (i<<25), dor+DOMID);
	  v |= inl(dor+DOMID)&0xffffff;
	  snprintf(domid, sizeof(domid), "%llx", v);
	  /* print it out... */
	  printf("dom\t%d\t%s\n", i, domid);
	}
      }
    }
    else if (strcmp(argv[ai], "wires")==0) {
      const unsigned dstat = inl(dor+DSTAT);
      int i;
      for (i=0; i<MAXWIRES; i++) {
	printf("wire\t%d\t%s\t%s\t%s\t%s\t%s\n",
	       i, 
	       (dstat&(1<<i)) ? "plugged" : "unplugged",
	       (dstat&(0x10<<i)) ? "powered" : "unpowered",
	       (dstat&(0x100<<i)) ? "limited" : "unlimited",
	       (dstat&(0x10000<<i)) ? "long" : "",
	       (dstat&(0x100000<<i)) ? "short" : "");
      }
    }
    else if (strcmp(argv[ai], "regs")==0) {
      printf("cntrl : 0x%08x\n", inl(dor+CTRL));
      printf("gstat : 0x%08x\n", inl(dor+GSTAT));
      printf("dstat : 0x%08x\n", inl(dor+DSTAT));
      printf("ttsic : 0x%08x\n", inl(dor+TTSIC));
      printf("rtsic : 0x%08x\n", inl(dor+RTSIC));
      printf("inten : 0x%08x\n", inl(dor+INTEN));
      printf("doms  : 0x%08x\n", inl(dor+DOMS));
      printf("mrar  : 0x%08x\n", inl(dor+MRAR));
      printf("mrtc  : 0x%08x\n", inl(dor+MRTC));
      printf("mwar  : 0x%08x\n", inl(dor+MWAR));
      printf("mwtc  : 0x%08x\n", inl(dor+MWTC));
      printf("utcrd0: 0x%08x\n", inl(dor+UTCRD0));
      printf("utcrd1: 0x%08x\n", inl(dor+UTCRD1));
      printf("curl  : 0x%08x\n", inl(dor+CURL));
      printf("dcur  : 0x%08x\n", inl(dor+DCUR));
      printf("flash : 0x%08x\n", inl(dor+FLASH));
      /* skip data buffers... */
      printf("domc  : 0x%08x\n", inl(dor+DOMC));
      /* skip unassigned register */
      printf("tcsic : 0x%08x\n", inl(dor+TCSIC));
      /* skip tcal data buffer */
      /* skip temp register... */
      printf("domid : 0x%08x\n", inl(dor+DOMID));
      printf("dcrev : 0x%08x\n", inl(dor+DCREV));
      printf("frev  : 0x%08x\n", inl(dor+FREV));
    }
    else if (strcmp(argv[ai], "burn")==0 && ai<argc-2) {
      /* page: 1..2 (sectors: 4..7, 8..11, addr: 0x40000, 0x80000),
       * file: rbf filename...
       */
      if (burnFlash(dor, atoi(argv[ai+1]), argv[ai+2])) {
	fprintf(stderr, "dt: can't burn flash!\n");
      }
      ai+=2;
    }
    else if (strcmp(argv[ai], "reload")==0 && ai<argc-1) {
      if (reloadFPGA(dor, devpath, atoi(argv[ai+1]))) {
	fprintf(stderr, "dt: can't load fpga\n");
      }
      ai++;
    }
    else if (strcmp(argv[ai], "rev")==0) {
      unsigned frev = inl(dor + FREV);
      unsigned dcrev = inl(dor + DCREV);
    
      printf("firmware: %03d.%03d.%03d %c\n",
	     (frev>>24)&0xff, (frev>>16)&0xff, (frev>>8)&0xff,
	     frev&0xff);
      printf("comm: %d\n", dcrev&0xffff);
    }
    else if (strcmp(argv[ai], "server")==0 && ai==argc-1) {
      /* server mode, forward packets between card and
       * sockets...
       *
       * 500xx where xx is card*8 + dom + 1
       *
       * only doms which are talking are exported via
       * sockets...
       */
      return domserv(dor);
    }
    else {
      fprintf(stderr, "dt: unknown cmd: '%s'\n", argv[ai]);
      return 1;
    }
  }

  return 0;
}
