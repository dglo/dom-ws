/* serialslv.c, the serial port
 * slave for domserv...
 *
 * env:
 *   stdin, stdout, stderr remapped to
 * term...
 */
#include <stdio.h>

#include <sys/poll.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
   int sfd = -1;
   
   /* make sure we're talking to a tty...
    */
   if (!isatty(0) || !isatty(1)) {
      fprintf(stderr, "serialslv: expecting tty on stdin/out\n");
      return 1;
   }

   /* open serial port...
    */
   
   /* start poll loop...
    */
   while (1) {
      struct pollfd fds[3];
      fds[0].fd = 0;
      fds[0].events = POLLIN;
      fds[1].fd = sfd;
      fds[1].events = POLLIN;

      /* wait for an event...
       */

      /* forward it...
       */
   }

   return 0;
}
