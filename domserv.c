/* domserv.c, open a network accessible pseudo-tty for
 * iceboot-sim or iceboot-dor or a ttyS? for iceboot on 
 * the serial port.  this gives us a consistent view of
 * iceboot from the outside, we then route this view through
 * a tcp socket (ports):
 *
 *   2000 serial telnet
 *   3000 serial raw
 *   4000 DOR telnet
 *   4500 DOR raw
 *   5000 SIM telnet
 *   5500 SIM raw
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/un.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <sys/types.h>
#include <unistd.h>
#include <pty.h>
#include <poll.h>

/* port definitions... */
#define SERIAL_PORT     2000
#define SERIAL_PORT_RAW 3000
#define DOR_PORT        4000
#define DOR_PORT_RAW    4500
#define SIM_PORT        5000
#define SIM_PORT_RAW    5500

static int servSerial = 1;
static int servDOR = 1;
static int servSim = 1;

/* return number of devices we support...
 */
static int nserial(void) {
   /* FIXME: check for serial ports... */
   return (servSerial) ? 2 : 0;
}

static int ndors(void) {
   /* FIXME: check for dors */
   return (servDOR) ? 64 : 0;
}

static int nsims(void) {
   /* FIXME: check for sims? */
   return (servSim) ? 64 : 0;
}

static int ndevs(void) {
   static int n = -1;
   if (n==-1) {
      n = nserial() + ndors() + nsims();
   }
   return n;
}

/* FIXME: listen for sigchld and update the slv field...
 */
typedef struct DOMListStruct {
   const char *ptyname;  /* NULL if no pty yet... */
   const char *type;     /* "serial", "DOR", "sim" */
   pid_t slv;            /* slave pid */
   int sfd;              /* telnet listen socket fd... */
   int rsfd;             /* raw listen socket fd... */
   int efd;              /* external descriptor (telnet socket) */
   int rfd;              /* external descriptor (raw socket) */
   int ifd;              /* internal descriptor (master side of (p)tty) */
   int port;             /* telnet socket port */
   int rawport;          /* raw socket port */
} DOMList;

static DOMList *newDOMList(void) {
   const int n = ndevs();
   int i, di;
   
   DOMList *dl = (DOMList *) calloc(n, sizeof(DOMList));
   if (dl==NULL) {
      fprintf(stderr, "domserv: can't allocate %d DOMs\n", n);
      return NULL;
   }
   
   for (i=di=0; i<nserial(); i++, di++) {
      char nm[16];
      sprintf(nm, "/dev/ttyS%d", i);
      dl[di].ptyname = strdup(nm);
      dl[di].type = "serial";
      dl[di].slv = (pid_t) -1;
      dl[di].sfd = -1;
      dl[di].rsfd = -1;
      dl[di].efd = -1;
      dl[di].rfd = -1;
      dl[di].ifd = -1;
      dl[di].port = SERIAL_PORT + 1 + i;
      dl[di].rawport = SERIAL_PORT_RAW + 1 + i;
   }

   for (i=0; i<ndors(); i++, di++) {
      dl[di].ptyname = NULL;
      dl[di].type = "DOR";
      dl[di].slv = (pid_t) -1;
      dl[di].sfd = -1;
      dl[di].rsfd = -1;
      dl[di].efd = -1;
      dl[di].rfd = -1;
      dl[di].ifd = -1;
      dl[di].port = DOR_PORT + 1 + i;
      dl[di].rawport = DOR_PORT_RAW + 1 + i;
   }

   for (i=0; i<nsims(); i++, di++) {
      dl[di].ptyname = NULL;
      dl[di].type = "sim";
      dl[di].slv = (pid_t) -1;
      dl[di].sfd = -1;
      dl[di].rsfd = -1;
      dl[di].efd = -1;
      dl[di].rfd = -1;
      dl[di].ifd = -1;
      dl[di].port = SIM_PORT + 1 + i;
      dl[di].rawport = SIM_PORT_RAW + 1 + i;
   }
   
   return dl;
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
   if (bind(sock, (struct sockaddr *) &server, sizeof server)<0) {
      perror("bind socket");
      return -1;
   }
  
   /* listen on new client socket...
    */
   listen(sock, 1);

   return sock;
}

static int openListeners(DOMList *dl) {
   if (dl->sfd==-1 && dl->rsfd==-1 && dl->efd==-1 && dl->rfd==-1) {
      /* both ports are quiet, open listeners... */
      dl->sfd = sockListener(dl->port);
      dl->rsfd = sockListener(dl->rawport);
   }
   return 0;
}

static void closeListeners(DOMList *dl) {
   if (dl->sfd!=-1) {
      close(dl->sfd);
      dl->sfd = -1;
   }
   if (dl->rsfd!=-1) {
      close(dl->rsfd);
      dl->rsfd = -1;
   }
}

static int startSerial(DOMList *dl) {
   /* open tty, fork... */
   return 1;
}

static int forkDOM(DOMList *dl, const char *prog) {
   char name[64];
   pid_t pid;
   int mfd;
   struct termios buf;

   /* setup iceboot terminal.
    *
    * FIXME: what about raw?!?
    */
   buf.c_lflag = 0;
   buf.c_iflag = IGNPAR | IGNBRK;
   buf.c_oflag = 0;
   buf.c_cflag = CLOCAL | CS8;
   buf.c_cc[VMIN] = 1;
   buf.c_cc[VTIME] = 0;

   if ((pid=forkpty(&mfd, name, &buf, NULL))<0) {
      perror("forkpty");
      return 1;
   }
   else if (pid==0) {
      /* child... */
      if (execl(prog, prog, NULL)<0) {
	 perror("execl");
	 return 1;
      }
   }
   
   dl->ptyname = strdup(name);
   dl->slv = pid;
   dl->ifd = mfd;
   return 0;
}

static int startDOR(DOMList *dl) {
   forkDOM(dl, "./Linux-i386/bin/dorslv");
   return 1;
}

static int startSim(DOMList *dl) {
   return forkDOM(dl, "./Linux-i386/bin/iceboot");
}

/* negotiate telnet parameters...
 */
static void telnetNeg(DOMList *dl) {
   int fd = dl->efd;
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

/* startup the slave if necessary...
 *
 * FIXME: should we close sockets on error?
 */
static int openSlave(DOMList *dl) {
   if ( (dl->efd!=-1 || dl->rfd!=-1) && dl->slv==(pid_t) -1) {
      if (strcmp(dl->type, "sim")==0) {
	 if (startSim(dl)) {
	    fprintf(stderr, "domserv: unable to start sim!\n");
	    return 1;
	 }
      }
      else {
	 fprintf(stderr, "domserv: unknown type\n");
	 return 1;
      }
   }
   
   return 0;
}

static int scanTelnet(unsigned char *buffer, int len) {
   int i;
   static int iac = 0;
		     
   i = 0;
   while (i<len) {
#if 0
      printf("i, len, iac, ch: %d %d %d 0x%02x\n", 
	     i, len, iac, buffer[i]);
#endif
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

int main(int argc, char *argv[]) {
   DOMList *dl = NULL;
   struct pollfd *fds = NULL;
   int i;

   if ((dl=newDOMList()) == NULL) {
      fprintf(stderr, "domserv: can't get DOR list...\n");
      return 1;
   }

   if ((fds=(struct pollfd *)
	calloc(ndevs()*3, sizeof(struct pollfd)))==NULL) {
      fprintf(stderr, "domserv: can't allocate pollfds\n");
      return 1;
   }

   /* FIXME: daemonize
   if (daemon(1, 1)<0) {
      perror("daemon");
      return 1;
   }
   */

   /* event loop...
    */
   while (1) {
      int nfds = 0;
      int ret, j;

      for (i=0; i<ndevs(); i++) {
	 if (openSlave(dl + i)) {
	    fprintf(stderr, "can't start slave for DOM %d\n", i);
	    return 1;
	 }

	 /* do we need to start listeners... */
	 if (openListeners(dl + i)) {
	    fprintf(stderr, "can't open sockets for DOM %d\n", i);
	    return 1;
	 }
      }
      
      /* setup fd list... */
      for (i=0; i<ndevs(); i++) {
	 if (dl[i].sfd!=-1) {
	    fds[nfds].fd = dl[i].sfd;
	    fds[nfds].events = POLLIN;
	    nfds++;
	 }
	 if (dl[i].rsfd!=-1) {
	    fds[nfds].fd = dl[i].rsfd;
	    fds[nfds].events = POLLIN;
	    nfds++;
	 }
	 if (dl[i].efd!=-1) {
	    fds[nfds].fd = dl[i].efd;
	    fds[nfds].events = POLLIN;
	    nfds++;
	 }
	 if (dl[i].rfd!=-1) {
	    fds[nfds].fd = dl[i].rfd;
	    fds[nfds].events = POLLIN;
	    nfds++;
	 }
	 if (dl[i].ifd!=-1) {
	    fds[nfds].fd = dl[i].ifd;
	    fds[nfds].events = POLLIN;
	    nfds++;
	 }
      }

      /* wait for events... */
      if ((ret=poll(fds, nfds, -1))<0) {
	 perror("poll");
	 return 1;
      }

      /* search out events... */
      for (j=0; j<ret; j++) {
	 for (i=0; i<nfds; i++) {
	    if (fds[i].revents) {
	       int k;

	       /* find the device... */
	       for (k=0; k<ndevs(); k++) {
		  unsigned char buffer[4096];

		  if (dl[k].sfd==fds[i].fd) {
		     if ((dl[k].efd = accept(dl[k].sfd, NULL, NULL))<0) {
			perror("accept");
		     }
		     closeListeners(dl+k);
		     telnetNeg(dl+k);
		     printf("accept: %d\n", dl[k].port);
		  }
		  else if (dl[k].rsfd==fds[i].fd) {
		     if ((dl[k].rfd = accept(dl[k].rsfd, NULL, NULL))<0) {
			perror("accept");
		     }
		     closeListeners(dl+k);

		     printf("accept: %d\n", dl[k].rawport);
		  }
		  else if (dl[k].efd==fds[i].fd) {
		     /* data coming in on telnet... */
		     int ret = read(fds[i].fd, buffer, sizeof(buffer));

		     printf("read %d from telnet\n", ret);

		     if (ret<0) {
			/* FIXME: eintr?!?!? */
			perror("read");
			close(dl[k].efd);
			dl[k].efd = -1;
		     }
		     else if (ret==0) {
			/* terminal closed... */
			close(dl[k].efd);
			dl[k].efd = -1;
		     }
		     else {
			ret = scanTelnet(buffer, ret);
			
			/* FIXME: data dump file! */
			if (dl[k].ifd!=-1) {
			   int nw = 0;
			   while (nw<ret) {
			      int n = write(dl[k].ifd, buffer + nw, ret - nw);
			      if (n<0) {
				 close(dl[k].ifd);
				 dl[k].ifd = -1;
				 perror("write");
			      }
			      else if (n==0) {
				 close(dl[k].ifd);
				 dl[k].ifd = -1;
				 break;
			      }
			      else nw+=n;
			   }
			}
		     }
		  }
		  else if (dl[k].rfd==fds[i].fd) {
		     /* data coming in on raw... */
		     int ret = read(fds[i].fd, buffer, sizeof(buffer));

		     printf("read %d from raw\n", ret);
		     
		     if (ret<0) {
			/* FIXME: eintr?!?!? */
			perror("read");
			close(dl[k].rfd);
			dl[k].rfd = -1;
		     }
		     else if (ret==0) {
			/* terminal closed... */
			close(dl[k].rfd);
			dl[k].rfd = -1;
		     }
		     else {
			/* FIXME: data dump file! */

			if (dl[k].ifd!=-1) { 
			   int nw = 0;
			   while (nw<ret) {
			      int n = write(dl[k].ifd, buffer + nw, ret - nw);
			      if (n<0) {
				 close(dl[k].ifd);
				 dl[k].ifd = -1;
				 perror("write");
			      }
			      else if (n==0) {
				 close(dl[k].ifd);
				 dl[k].ifd = -1;
				 break;
			      }
			      else nw+=n;
			   }
			}
		     }
		  }
		  else if (dl[k].ifd==fds[i].fd) {
		     /* data coming in on terminal -- push it along... */
		     int ret = read(fds[i].fd, buffer, sizeof(buffer));

		     
		     printf("read %d from pty\n", ret);
		     
		     if (ret<0) {
			/* FIXME: eintr?!?!? */
			perror("read");
			close(dl[k].ifd);
			dl[k].ifd = -1;
		     }
		     else if (ret==0) {
			/* terminal closed... */
			close(dl[k].ifd);
			dl[k].ifd = -1;
		     }
		     else {
			int nw = 0;
			int fd = -1;
			int *fdp = &fd;

			/* FIXME: data dump file! */

			if (dl[k].efd!=-1)      { fdp = &dl[k].efd; }
			else if (dl[k].rfd!=-1) { fdp = &dl[k].rfd; }
			if (*fdp>0) {
			   while (nw<ret) {
			      int n = write(*fdp, buffer + nw, ret - nw);
			      if (n<0) {
				 close(*fdp);
				 *fdp = -1;
				 perror("write");
			      }
			      else if (n==0) {
				 close(*fdp);
				 *fdp = -1;
				 break;
			      }
			      else nw+=n;
			   }
			}
		     }
		  }
	       }
	    }
	 }
      }
   }

   /* FIXME: wait for children before exiting...
    */

   
   return 0;
}
