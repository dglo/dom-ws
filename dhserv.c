/* dhserv.c, domhub server, we can:
 *
 * we need a server to put the
 * doms in the following modes:
 * 
 *   1) pwrall off
 *   2) pwrall on -> stf server -> print status [server port card channel dom]
 *
 * we export this functionality on a socket (2500)...
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <unistd.h>
#include <pty.h>
#include <poll.h>
#include <sys/time.h>

static long long getms(void) {
   struct timeval tv;
   gettimeofday(&tv, NULL);
   return (long long)tv.tv_sec * 1000LL + tv.tv_usec/1000;
}

static void tsPwroff(void) {
   system("/home/arthur/bin/pwrall off");
}

static void tsPwron(void) {
   system("/home/arthur/bin/pwrall on");
}

static int pwrall(const char *cmd) {
   int fd = open("/proc/driver/domhub/pwrall", O_WRONLY);
   if (fd==-1) return -1;
   write(fd, cmd, strlen(cmd));
   close(fd);
   return 0;
}

typedef enum DOMStateEnum {
   eStateNone,
   eStateConfigboot,
   eStateIceboot,
   eStateSTF
} DOMState;

typedef struct DOMStruct {
   int      card;
   int      channel;
   char     dom;
   short    port;
   DOMState state;
   int      sfd;
   char     buffer[256];
   int      bidx;
} DOM;

static void initDOMS(DOM *doms, int ndoms) {
   int i;
   
   for (i=0; i<ndoms; i++) {
      doms[i].state = eStateNone;
      doms[i].sfd = -1;
      memset(doms[i].buffer, 0, sizeof(doms[i].buffer));
      doms[i].bidx = 0;
   }
}

static void domInput(DOM *dom) {
   /* found! */
   printf("domInput [%d]: %d [%u]\n", dom->sfd, dom->bidx, sizeof(dom->buffer));
   if (dom->bidx==sizeof(dom->buffer)) {
      /* chuck it...
       */
      char b[128];
      read(dom->sfd, b, sizeof(b));
      close(dom->sfd);
      dom->sfd = -1;
      fprintf(stderr, "dhserv: warning buffer full for dom %d %d %c\n",
	      dom->card, dom->channel, dom->dom);
   }
   else {
      const int nr = 
	 read(dom->sfd, dom->buffer + dom->bidx, 
	      sizeof(dom->buffer) - dom->bidx);
      if (nr<0) {
	 perror("read");
	 close(dom->sfd);
	 dom->sfd = -1;
      }
      else if (nr==0) {
	 close(dom->sfd);
	 dom->sfd = -1;
      }
      else {
	int j;
	 dom->bidx+=nr;
	 for (j=0; j<dom->bidx; j++) {
	   printf("%02x ", dom->buffer[j]);
	   if ( (j+1)%16 == 0 ) printf("\n");
	 }
	 printf("\n");
      }
   }
}

/* fill dom buffers... */
static int fillDOMs(DOM *doms, int ndoms, int wms) {
   struct pollfd *fds;
   int nfds = 0;
   const long long ms = getms() + wms;
   int j;

   /* pick fds to look at... */
   fds = (struct pollfd *) calloc(ndoms, sizeof(struct pollfd));
   if (fds==NULL) {
      fprintf(stderr, "dhserv: unable to calloc(%d, %d)\n",
	      ndoms, sizeof(struct pollfd));
      return 1;
   }

   nfds=0;
   for (j=0; j<ndoms; j++) {
      if (doms[j].sfd!=-1) {
	 fds[nfds].fd = doms[j].sfd;
	 fds[nfds].events = POLLIN;
	 nfds++;
      }
      doms[j].bidx = 0;
      memset(doms[j].buffer, 0, sizeof(doms[j].buffer));
   }

   /* nothing there? */
   if (nfds==0) {
      free(fds);
      return 0;
   }
   
   /* find doms that are communicating */
   while (1) {
      const long long nms = getms();
      int ret;
      
      /* check remaining time... */
      if (nms>=ms) break;
      
      /* wait for input... */
      if ((ret=poll(fds, nfds, ms-nms))<0) {
	 perror("poll");
	 free(fds);
	 return 1;
      }
      
      /* check for input */
      if (ret>0) {
	 for (j=0; j<nfds; j++) {
	    if (fds[j].revents&POLLIN) {
	       int k;
	       
	       for (k=0; k<ndoms; k++) {
		  if (doms[k].sfd==fds[j].fd) {
		     domInput(doms+k);
		     break;
		  }
	       }
	    }
	 }
      }
   }
   
   free(fds);
   return 0;
}

static void icebootToSTF(DOM *doms, int ndoms) {
   int i;
   const char *q = "s\" stfserv\" find if exec endif\r";

   /* write stf command... */
   for (i=0; i<ndoms; i++) {
      if (doms[i].state==eStateIceboot) {
	 write(doms[i].sfd, q, strlen(q));
      }
   }

   /* get current dom input */
   fillDOMs(doms, ndoms, 1000);

   /* check for cmd */
   for (i=0; i<ndoms; i++) {
      if (doms[i].state==eStateIceboot) {
	printf("iceboot->stf: '%s'\n", doms[i].buffer);
	 if (strncmp(doms[i].buffer, q, strlen(q))!=0) {
	    fprintf(stderr, "dhserv: dom %d did not go into STF!\n", i);
	    close(doms[i].sfd);
	    doms[i].sfd = -1;
	    doms[i].state = eStateNone;
	 }
	 doms[i].state = eStateSTF;
      }
   }
}

static void validateSTF(DOM *doms, int ndoms) {
   int i;
   
   for (i=0; i<ndoms; i++) {
      if (doms[i].state==eStateSTF) {
	 const char *q = "OK\r";
	 write(doms[i].sfd, q, strlen(q));
	 printf("dom: %d is in STF...\n", i);
      }
   }
   
   /* let them chomp on it a bit... */
   poll(NULL, 0, 10);
   
   /* get current dom input */
   fillDOMs(doms, ndoms, 1000);

   for (i=0; i<ndoms; i++) {
      if (doms[i].state==eStateSTF) {
	printf("validateSTF: '%s'\n", doms[i].buffer);
	 if (strcmp(doms[i].buffer, "OK\r\n")!=0) {
	    fprintf(stderr, "dhserv: dom %d is no longer in STF!\n", i);
	    close(doms[i].sfd);
	    doms[i].sfd = -1;
	    doms[i].state = eStateNone;
	 }
      }
   }
}

static int sockListener(int port) {
   struct sockaddr_in server;
   int one = 1;
   int sock = socket(AF_INET, SOCK_STREAM, 0);
   if (sock<0) {
      perror("opening stream socket");
      return -1;
   }

   setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
  
   server.sin_family      = AF_INET;
   server.sin_addr.s_addr = INADDR_ANY;
   server.sin_port        = ntohs(port);
   printf("bind...\n");
   
   if (bind(sock, (struct sockaddr *) &server, sizeof server)<0) {
      perror("bind socket");
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

static void dummy(int s) {}

static int connectTCP(const char *hostname, short port) {
   struct sockaddr_in serv_addr;
   int sockfd;
   struct hostent *he;
   
   printf("connectTCP: '%s:%d'\n", hostname, port);

   if ((he=gethostbyname(hostname))==NULL) {
      fprintf(stderr, "connectTCP: can't get host ent!\n");
      return -1;
   }

   memset(&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family      = AF_INET;
   serv_addr.sin_addr.s_addr = *(unsigned long *) he->h_addr_list[0];
   serv_addr.sin_port        = htons(port);
   
   if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      perror("socket");
      return -1;
   }
   
   if (connect(sockfd, 
	       (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      char str[100];
      sprintf(str, "connect %s %d", hostname, port);
      perror(str);
      close(sockfd);
      return -1;
   }
   
   return sockfd;
}

static void connectDOMS(DOM *doms, int ndoms, const char *host) {
   int i;
   
   /* open all dom sockets... */
   for (i=0; i<ndoms; i++) {
      doms[i].sfd = connectTCP(host, doms[i].port);
      doms[i].bidx = 0;
      memset(doms[i].buffer, 0, sizeof(doms[i].buffer));
   }
}

static int disconnect(int sfd) {
   int i;
   shutdown(sfd, 1);
   for (i=0; i<10; i++) {
      struct pollfd fds[1];
      int ret;
      
      fds[0].fd = sfd;
      fds[0].events = POLLIN;
      
      if ((ret=poll(fds, 1, 2000))<0) {
	 perror("poll");
	 return 1;
      }

      if (ret==1 && fds[0].revents&POLLIN) {
	 char buf[1024];
	 int nr = read(sfd, buf, sizeof(buf));
	 if (nr==0) break;
      }
   }
   close(sfd);
   return 0;
}

static void disconnectDOMS(DOM *doms, int ndoms) {
   int i;
   
   /* open all dom sockets... */
   for (i=0; i<ndoms; i++) {
      disconnect(doms[i].sfd);
      doms[i].sfd = -1;
      doms[i].bidx = 0;
   }
}

int main(int argc, char *argv[]) {
   pid_t pid;
   int dsc, dsr; /* domserv commands and reads... */
   int ai;
   int nports = 64;
   const char *host = "localhost";
   int ts = 0; /* use terminal server (else dor) */
   DOM *doms;
   int ndoms;
   int mode = 0;
   int i;
   
   /* parse command line options...
    */
   for (ai=1; ai<argc; ai++) {
      if (strcmp(argv[ai], "-ts")==0 && ai<argc-1) {
	 ts=1;
	 host=argv[ai+1];
	 ai++;
      }
      else if (strcmp(argv[ai], "-nports")==0 && ai<argc-1) {
	 nports = atoi(argv[ai+1]);
	 ai++;
      }
      else {
	 fprintf(stderr, "usage: dhserv [-ts hostname | -nports n]\n");
	 return 1;
      }
   }

   if (ts) {
      ndoms=nports;
   }
   else {
      ndoms=64;
   }
   
   /* allocate dom memory...
    */
   if ((doms = calloc(ndoms, sizeof(DOM)))==NULL) {
      fprintf(stderr, "dhserv: can't allocate %d doms\n", ndoms);
      return 1;
   }

   for (i=0; i<ndoms; i++) {
      if (ts) {
	 doms[i].card = 1;
	 doms[i].channel = i+1;
	 doms[i].dom = 'A';
	 doms[i].port = 3001 + i;
      }
      else {
	 /* FIXME: domhub here! */
      }
   }
   
   /* set doms to a known state -- pwrall off
    */
   printf("powering down doms...\n");
   if (ts) tsPwroff(); 
   else pwrall("off");
   initDOMS(doms, ndoms);

   /* start domserv in dh mode...
    */
   if (!ts) {
      /* open read/write pipes */
      if ((pid=fork())<0) {
	 perror("fork");
	 return 1;
      }
      else if (pid==0) {
	 /* FIXME: exec domserv... */
      }
      /* FIXME: assign dsc, dsr from pipes... */
   }
   
   /* FIXME: daemonize... */

   /* main loop */
   while (1) {
      /* wait for a connection on socket...
       */
      const int sock = sockListener(2500);
      int sfd;

      if (sock<0) {
	 fprintf(stderr, "dhserv: can't open socket\n");
	 return 1;
      }

      printf("waiting for line: mode: %d\n", mode);

      if ((sfd = accept(sock, NULL, NULL))<0) {
	 perror("accept");
	 return 1;
      }

      /* no more listeners... */
      close(sock);
      
      /* read a line... */
      {  char line[128];
	 const int nb = sizeof(line)-1;
	 int idx = 0;
	 char *t;
	 int found = 0;
	 
	 memset(line, 0, sizeof(line));

	 while (sfd>=0 && idx<nb && !found) {
	    const int nr = read(sfd, line+idx, nb-idx);
	    
	    if (nr<0) {
	       perror("read from socket");
	       close(sfd);
	       sfd=-1;
	    }
	    else if (nr==0) {
	       close(sfd);
	       sfd=-1;
	    }
	    
	    idx+=nr;

	    if ((t=strchr(line, '\r'))!=NULL) {
	       *t = 0;
	       found = 1;
	    }
	 }

	 if (found) {
	    printf("line: '%s'\n", line); fflush(stdout);
	 
	    /* parse it...
	     */
	    if (strcmp(line, "stf")==0) {
	      /* connect doms... */
	       connectDOMS(doms, ndoms, host);

	       /* if was modeoff: */
	       if (mode==0) {
		  /* pwrall on */
		  if (ts) { tsPwron(); }
		  else { pwrall("on"); }

		  /* wait a little... */
		  poll(NULL, 0, 1000);

		  /* get data... */
		  fillDOMs(doms, ndoms, 2000);

		  /* check for proper boot string */
		  for (i=0; i<ndoms; i++) {
		     if (doms[i].bidx>2) {
		       int j;
		       /* first, check for garbage in the
			* first few characters...
			*/
		       for (j=0; j<doms[i].bidx; j++) {
			 if ( (doms[i].buffer[j]&0x80) ||
			      doms[i].buffer[j]==0) {
			   memmove(doms[i].buffer + j,
				   doms[i].buffer + j+1,
				   doms[i].bidx - (j+1));
			   doms[i].bidx--;
			 }
		       }
		       
		       printf("dom: %d output: '%s'\n", i, doms[i].buffer);

		       if (strncmp(doms[i].buffer + doms[i].bidx-2, 
				   "> ", 2)==0) {
			 doms[i].state = eStateIceboot;
			 printf("iceboot!\n");
		       }
		     }
		  }
		  
		  /* put them into stf mode */
		  icebootToSTF(doms, ndoms);
	       }

	       printf("validateSTF...\n");

	       /* validate stf */
	       validateSTF(doms, ndoms);

	       printf("disconnecting...\n");

	       /* disconnect doms... */
	       disconnectDOMS(doms, ndoms);

	       /* now wait a bit for ts to reset... */
	       poll(NULL, 0, 500);

	       printf("done, printing status list...\n");
	       
	       /* print status list */
	       for (i=0; i<ndoms; i++) {
		  if (doms[i].state==eStateSTF) {
		     char line[128];
		     sprintf(line, "%s\t%d\t%d\t%d\t%c\n",
			     host, doms[i].port-3000, doms[i].card,
			     doms[i].channel, doms[i].dom);
		     write(sfd, line, strlen(line));
		  }
	       }
	       
	       mode=1;
	    }
	    else if (strcmp(line, "off")==0) {
	      printf("off!\n");
	       if (mode!=0) {
		  if (ts) tsPwroff();
		  else pwrall("off");
		  initDOMS(doms, ndoms);
		  mode=0;
	       }
	    }
	    else {
	       /* unknown?!?! */
	    }
	 }

	 /* shutdown client... 
	  */
	 shutdown(sfd, 1);
	 while (1) {
	    char buf[128];

	    /* FIXME: we need a timeout here...
	     */
	    const int nr = read(sfd, buf, sizeof(buf));
	    if (nr<=0) break;
	 }
	 close(sfd);
	 sfd=-1;
      }
   }
}



















