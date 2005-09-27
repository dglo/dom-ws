/* talk to the watlow 942 controller -- requires controller
 * to be in XON/XOFF communications mode...
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>

#include <poll.h>

static int connectTCP(const char *hostname, short port) {
   struct sockaddr_in serv_addr;
   int sockfd;
   struct hostent *he;
   char **p;
   
   if ((he=gethostbyname(hostname))==NULL) {
      fprintf(stderr, "can't get ip for host name '%s'!\n", hostname);
      return -1;
   }
   
   p = he->h_addr_list;
   
   memset(&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family      = AF_INET;
   memcpy(&serv_addr.sin_addr.s_addr, *p, sizeof(serv_addr.sin_addr.s_addr));
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

/* soak up data on fd... */
static int soak(int fd, int ms) {
   char buffer[256];
   struct pollfd fds;

   fds.fd=fd;
   fds.events=POLLIN;
   while (poll(&fds, 1, ms)==1) {
      int nr = read(fd, buffer, sizeof(buffer)-1);
      if (nr<0) {
         perror("read");
         return 1;
      }
      else if (nr==0) {
         fprintf(stderr, "eof on read!\n");
         return 1;
      }
   }
   return 0;
}

int main(int argc, char *argv[]) {
   int fd = connectTCP("icecubelab1.lbl.gov", 10002);
   char line[128];
   
   if (fd<0) {
      fprintf(stderr, "w942: unable to connect\n");
      return 1;
   }

   /* clear out pending fd data... */
   if (soak(fd, 500)) {
      fprintf(stderr, "w942: unable to clear incoming...\n");
      return 1;
   }

   while (fgets(line, sizeof(line), stdin)!=NULL) {
      const int len=strlen(line);
      char buffer[sizeof(line)*2];
      enum { CMD_QRY, CMD_SET } cmd;

      if (len==0) break;
      if (line[len-1]=='\n') line[len-1]=0;
      if (strlen(line)==0) continue; /* ignore empty lines... */

      snprintf(buffer, sizeof(buffer), "%s\r", line);
      
#if 0
      {
         int j;
         printf("[");
         for (j=0; j<strlen(buffer); j++) {
            printf(" %02x", buffer[j]);
         }
         printf("]\n");
      }
#endif
      if (buffer[0]=='?') cmd = CMD_QRY;
      else if (buffer[0]=='=') cmd = CMD_SET;
      else if (strcmp(buffer, "quit\r")==0) {
         break;
      }
      else {
         fprintf(stderr, 
                 "w942: invalid command, expecting ? or =, got 0x%02x\n",
                 buffer[0]);
         return 1;
      }

      if (write(fd, buffer, strlen(buffer)) != strlen(buffer)) {
         perror("write");
         return 1;
      }
      
      {
         enum { 
            STATE_XOFF, STATE_XON, STATE_DATA, STATE_TERM
         } state = STATE_XOFF;
         char data[128];
         int idx=0;

         while (state!=STATE_TERM) {
            char b[128];
            int nr = read(fd, b, sizeof(b));
            int i;
            
            if (nr<0) {
               perror("read");
               return 1;
            }
            else if (nr==0) {
               return 0;
            }
            
            for (i=0; i<nr; i++) {
               /* printf("nr=%d, state=%d, val=0x%02x\n", nr, state, b[i]); */
               
               if (state==STATE_XOFF) {
                  if (b[i]==0x13) {
                     state=STATE_XON;
                  }
                  else {
                     fprintf(stderr, "w942: expecting XOFF, got 0x%02x\n",
                             b[i]);
                     return 1;
                  }
               }
               else if (state==STATE_XON) {
                  if (b[i]==0x11) {
                     state = (cmd==CMD_QRY) ? STATE_DATA : STATE_TERM;
                  }
                  else {
                     fprintf(stderr, "w942: expecting XOFF, got 0x%02x\n", 
                             b[i]);
                     return 1;
                  }
               }
               else if (state==STATE_DATA) {
                  if (b[i]=='\r') state = STATE_TERM;
                  else {
                     if (idx<sizeof(data)) {
                        data[idx] = b[i];
                        idx++;
                     }
                  }
               }
            }
         }

         /* echo qry data... */
         if (cmd==CMD_QRY) {
            write(1, data, idx);
            write(1, "\n", 1);
         }
      }
   }

   return 0;
}
