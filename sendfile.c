/* send a file to a socket...
 *
 * we use this to send the intel hex file to the dom...
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>

int main(int argc, char *argv[]) {
   int fd, sfd;
   ssize_t ts = 0;
   off_t offset = 0;
   struct stat st;
   int nretries = 0;
   struct hostent *he;
   struct sockaddr_in serv_addr;

   if (argc!=4) {
      fprintf(stderr, "usage: sendfile file host port\n");
      return 1;
   }

   if ((fd=open(argv[1], O_RDONLY))<0) {
      perror("open");
      fprintf(stderr, "sendfile: can't open '%s'\n", argv[1]);
      return 1;
   }
   
   if (fstat(fd, &st)<0) {
      perror("fstat");
      return 1;
   }

   if ((he=gethostbyname(argv[2]))==NULL) {
      fprintf(stderr, "sendfile: can't lookup host: '%s'\n",  argv[2]);
      return 1;
   }
   
   memset(&serv_addr, 0, sizeof(serv_addr));
   memcpy(&serv_addr.sin_addr.s_addr, *he->h_addr_list, 
	  sizeof(serv_addr.sin_addr.s_addr));
   serv_addr.sin_family      = AF_INET;
   serv_addr.sin_port        = htons(atoi(argv[3]));
   
   if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      perror("socket");
      return 1;
   }
   
   if (connect(sfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      perror("connect");
      return 1;
   }
   
   while (ts<st.st_size) {
      ssize_t ns = sendfile(sfd, fd, &offset, st.st_size-ts);
      
      if (ns<0) {
	 perror("sendfile");
	 return 1;
      }
      else if (ns==0) {
	 sleep(1);
	 nretries++;
	 if (nretries==10) {
	    fprintf(stderr, "sendfile: error sending data!\n");
	    return 1;
	 }
      }
      else ts+=ns;
   }
   
   return 0;
}

