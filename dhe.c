/* dhe.c, domhub event notifier...
 *
 * usage:
 * dhe [-port portn]
 *   open 00A
 *   open 00A (32)
 *   close 00A
 *   close 00A (32)
 *   open 00C
 *   ERR 'invalid dom 00C'
 *
 * dhe opens a port (default is 5200), it listens
 * for a list of doms on this port:
 *
 * 64 bytes:
 * 
 * where:
 *   flags  : 1 listen for reads
 *          : 2 listen for writes
 *
 * it writes 64 bytes back with the dom info of
 * any doms for which changes were requested and
 * which have been previously opened...
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/poll.h>
#include <unistd.h>
#include <fcntl.h>

/* return dom index or -1 on error...
 */
static int domIdx(const char *txt) {
   /* FIXME: do this right! */
   int idx = atoi(txt);
   if (idx==0 && (txt[0]!='0' || strlen(txt)!=1) ) {
      idx=-1;
   }
   else if (idx<0 || idx>63) {
      idx=-1;
   }
   
   return idx;
}

static inline int getCard(int idx) { return idx/8; }
static inline int getPair(int idx) { return (idx%8)/4; }
static inline char getAnB(int idx) { return (idx%2) ? 'B' : 'A'; }
static inline int getIdx(unsigned short v) { return v>>16; }

static void openDOM(const char *txt, int *dfds, int ndfds) {
   const int idx = domIdx(txt);

   if (idx<0) {
      printf("ERR 'invalid dom (0..63)'\n");
   }
   else if (dfds[idx]!=-1) {
      printf("ERR 'dom %d already open'\n", idx);
   }
   else {
      char path[128];
      
      snprintf(path, sizeof(path), "/dev/dtc%dw%dd%c",
               getCard(idx), getPair(idx), getAnB(idx));
               
      if ((dfds[idx]=open(path, O_RDWR))<0) {
         dfds[idx]=-1;
         printf("ERR 'unable to open '%s': %s\n", path, strerror(errno));
      }
      else {
         printf("open %d\n", idx);
      }
   }
}

static void closeDOM(const char *txt, int *dfds, int ndfds) {
   const int idx = domIdx(txt);

   if (idx<0) {
      printf("ERR 'invalid dom (0..63)'\n");
   }
   else if (dfds[idx]==-1) {
      printf("ERR 'dom %d already closed'\n", idx);
   }
   else {
      if (close(dfds[idx])<0) {
         printf("ERR 'unable to close %d: %s\n", idx, strerror(errno));
      }
      else printf("close %d\n", idx);
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

static void closeSocket(int sfd) {
   /* the famous lingering_close...
    *
    * we shutdown our side, and wait for the
    * other side to shutdown before closing up
    * shop...
    *
    * this code is necessary so that we're sure
    * everything gets cleanup up on the other
    * side...
    */
   shutdown(sfd, 1);

   while (1) {
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
	 if (nr<=0) break;
         if (verbose) write(1, buf, nr);
      }
   }

   close(sfd);
}

#define MAXDOMS 64

int main(int argc, char *argv[]) {
   int ai, i;
   int port = 5200;
   int dfds[MAXDOMS];
   unsigned short pollinfo[MAXDOMS];
   int npollinfo=0;
   unsigned short outinfo[MAXDOMS];
   int noutinfo=0;
   const int ndfds = sizeof(dfds)/sizeof(dfds[0]);
   struct pollfd fds[MAXDOMS + 2]; /* each dom + stdin + control socket */
   int nfds = 0;
   int needfds = 1;
   int sockfd;
   int isConnected = 0;
   
   for (i=0; i<ndfds; i++) dfds[i] = -1;

   for (ai=1; ai<argc; ai++) {
      if (strcmp(argv[ai], "-port")==0 && ai+1<argc) {
         port = atoi(argv[ai+1]);
         ai++;
      }
      else break;
   }
   
   if (ai!=argc) {
      fprintf(stderr, "usage: dhe [-port port#]\n");
      return 1;
   }

   /* FIXME: open socket... */
   if ((sockfd=openListener(port))<0) {
      fprintf(stderr, "dhe: unable to open socket...\n");
      return 1;
   }

   fds[nfds].fd = 0;
   fds[nfds].events = POLLIN;
   nfds++;

   fds[nfds].fd = sockfd;
   fds[nfds].events = POLLIN;
   nfds++;
   
   /* main loop... */
   while (1) {
      int ret;

      /* add fds of interest... 
       */
      {  int ii;
         nfds = 2;
         for (ii=0; ii<ndoms; ii++) {
            if (dfds[ii]!=-1 && reqflags[ii]) {
               if (reqflags[ii]&1) {
                  fds[nfds].fd = dfds[ii];
               }
               else if (reqflags[ii]&2) {
               }
            }
         }
      }

      if ((ret=poll(fds, nfds, -1))<0) {
         perror("poll");
         return 1;
      }

      /* check for stdin... */
      if (fds[0].revents&POLLIN) {
         char line[128];
         int nr;
         
         /* deal with input... */
         memset(line, 0, sizeof(line));
         
         if ((nr=read(0, line, sizeof(line)-1))<0) {
            perror("read from stdin");
            return 0;
         }
         else if (nr==0) {
            /* stdin is closed! */
            fprintf(stderr, "dhe: stdin is closed!\n");
            return 1;
         }
         else {
            if (strncmp(line, "open ", 5)==0) {
               openDOM(line+5, dfds, ndfds);
            }
            else if (strncmp(line, "close ", 6)==0) {
               closeDOM(line+6, dfds, ndfds);
            }
            else {
               printf("ERR 'invalid command (open|close)'\n");
            }
         }
      }
      else if (fds[1].revents&POLLIN) {
         if (isConnected) {
            /* get data from control socket... */
            unsigned char buf[64];
            int tr = 0;
            
            while (tr<sizeof(buf)) {
               const int nr = read(sockfd, buf+tr, sizeof(buf)-tr);
            
               if (nr<0) {
                  closeSocket(sockfd);
                  break;
               }
               else if (nr==0) {
                  closeSocket(sockfd);
                  break;
               }
            }

            if (tr==sizeof(buf) {
               /* process data... */
               int ii;
               for (ii=0; ii<sizeof(buf); ii++) {
                  /* 
               }
            }
         }
         else {
            /* accept connection, close listener... */
         }
      }

      noutinfo = 0;
      for (i=2; i<nfds; i++) {
         const unsigned short pi = pollinfo[i-2];
         const int idx = getIdx(pi);
         
         if (fds[i].revents) {
            outinfo[noutinfo] = 0;
            if ( (pi&1) && (fds[i].revents&POLLIN) ) {
               outinfo[noutinfo]|=1;
            }
            else if ( (pi&2) && (fds[i].revents&POLLOUT) ) {
               outinfo[noutinfo]|=2;
            }
            if (outinfo[noutinfo]) noutinfo++;
         }
      }

      if (noutinfo>0) {
         unsigned short buf[64+1];
         buf[0] = noutinfo;
         for (i=0; i<noutinfo; i++) buf[i+1] = outinfo[i];
      }
      
   }
   
   return 0;
}






