/* dortty.c, DOR slave side of pty, we
 * copy dor bits back and forth to tty 
 * side...
 */
#include <stdio.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

int main(int argc, char *argv[]) {
   int fd;
   struct pollfd fds[2];
   
   if (argc!=2) {
      fprintf(stderr, "usage: dortty device\n");
      return 1;
   }

   if ((fd = open(argv[1], O_RDWR))<0) {
      perror("open device file");
      return 1;
   }

   fds[0].fd = 0;
   fds[0].events = POLLIN;
   fds[1].fd = fd;
   fds[1].events = POLLIN;

   while (1) {
      unsigned char buffer[4096];
      int ret = poll(fds, 2, -1);
      
      if (ret<0) {
	 perror("poll");
	 return 1;
      }
      else if (ret>0) {

	 if (fds[0].revents) {
	    int ret = read(0, buffer, sizeof(buffer));
	    int wr, j;
	    
	    if (ret<0) {
	       perror("read");
	       return 1;
	    }
	    else if (ret==0) {
	       fprintf(stderr, "EOF on tty!\n");
	       return 1;
	    }
	    /* printf("dortty: read %d from tty\n", ret); */

	    /* HACK!!! */
	    for (j=0; j<ret; j++) {
	       if (buffer[j]==0x0a) buffer[j] = 0x0d;
	    }

	    /* send it off to DOR...
	     */
	    if ((wr=write(fd, buffer, ret))<0) {
	       perror("write");
	       return 1;
	    }
	    /* printf("dortty: wrote %d to DOR\n", wr); */
	 }
	 if (fds[1].revents) {
	    int ret = read(fd, buffer, sizeof(buffer));
	    int wr, j;
	    if (ret<0) {
	       perror("read");
	       return 1;
	    }
	    else if (ret==0) {
	       fprintf(stderr, "EOF on tty!\n");
	       return 1;
	    }
	    /* printf("dortty: read %d from DOR: ", ret); */

	    /*
	    for (j=0; j<ret; j++) {
	       printf("0x%02x ", buffer[j]);
	    }
	    printf("\r\n");
	    */

	    if ((wr = write(0, buffer, ret))<0) {
	       perror("write");
	       return 1;
	    }
	    /* printf("dortty: wrote %d to tty\n", wr); */
	 }
      }
   }
   return 0;
}






