/* dhclient.c, client for dhserv...
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

static int connectTCP(const char *hostname, short port) {
   struct sockaddr_in serv_addr;
   int sockfd;
   struct hostent *he;
   
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

int main(int argc, char *argv[]) {
  int ai, fd;

  if (argc<2) {
    fprintf(stderr, "usage: dhclient [-stf | -off] host\n");
    return 1;
  }

  if ((fd=connectTCP(argv[argc-1], 2500))<0) {
    fprintf(stderr, "can't connect to '%s'\n", argv[argc-1]);
    return 1;
  }

  for (ai=1; ai<argc-1; ai++) {
    char buffer[4096];

    if (strcmp(argv[ai], "-stf")==0) {
      write(fd, "stf\r", strlen("stf\r"));
      while (1) {
	const int nr = read(fd, buffer, sizeof(buffer));
	
	if (nr<0) {
	  perror("read");
	  return 1;
	}
	else if (nr==0) {
	  /* done... */
	  break;
	}
	else {
	  /* FIXME: filter '\r' */
	  write(1, buffer, nr);
	}
      }
    }
    else if (strcmp(argv[ai], "-off")==0) {
      write(fd, "off\r", strlen("off\r"));
      while (1) {
	const int nr = read(fd, buffer, sizeof(buffer));
	
	if (nr<0) {
	  perror("read");
	  return 1;
	}
	else if (nr==0) {
	  /* done... */
	  break;
	}
	else {
	  /* FIXME: filter '\r' */
	  write(1, buffer, nr);
	}
      }
    }
    else {
      fprintf(stderr, "usage: dhclient [-stf | -off] host\n");
      return 1;
    }
  }

  return 0;
}
