/* simslv.c, simulation slave...
 */
#include <stdio.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

static struct termios savedterm;

static void catchsig(int sig) {
   tcsetattr(0, TCSAFLUSH, &savedterm);
   exit(sig);
}

#if 0
/* put terminal in raw mode - see termio(7I) for modes 
 */
void tty_raw(void) {
   struct termios raw;
   
   raw = orig_termios;  /* copy original and then modify below */
   
   /* input modes - clear indicated ones giving: no break, no CR to NL, 
      no parity check, no strip char, no start/stop output (sic) control */
   raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
   
   /* output modes - clear giving: no post processing such as NL to CR+NL */
   raw.c_oflag &= ~(OPOST);
   
   /* control modes - set 8 bit chars */
   raw.c_cflag |= (CS8);
   
   /* local modes - clear giving: echoing off, canonical off (no erase with 
      backspace, ^U,...),  no extended functions, no signal chars (^Z,^C) */
   raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
   
   /* control chars - set return condition: min number of bytes and timer */
   raw.c_cc[VMIN] = 5; raw.c_cc[VTIME] = 8; /* after 5 bytes or .8 seconds
					       after first byte seen      */
   raw.c_cc[VMIN] = 0; raw.c_cc[VTIME] = 0; /* immediate - anything       */
   raw.c_cc[VMIN] = 2; raw.c_cc[VTIME] = 0; /* after two bytes, no timer  */
   raw.c_cc[VMIN] = 0; raw.c_cc[VTIME] = 8; /* after a byte or .8 seconds */
   
   /* put terminal in raw mode after flushing */
   if (tcsetattr(ttyfd, TCSAFLUSH, &raw) < 0) fatal("can't set raw mode");
}
#endif

int main(int argc, char *argv[]) {
   struct termios buf;
   pid_t iceboot;
   int status;
   
   /* save current terminal settings...
    */
   if (tcgetattr(0, &savedterm)<0) {
      perror("tcgetattr");
      return 1;
   }

   /* setup signal catcher...
    */
   signal(SIGTERM, catchsig);
   signal(SIGINT, catchsig);

   /* setup iceboot terminal.
    */
   buf = savedterm;
   buf.c_lflag = 0;
   buf.c_iflag = IGNPAR | IGNBRK;
   buf.c_oflag = 0;
   buf.c_cflag = CLOCAL | CS8;
   buf.c_cc[VMIN] = 1;
   buf.c_cc[VTIME] = 0;
   
   if (tcsetattr(0, TCSAFLUSH, &buf)<0) {
      perror("tcsetattr");
      return 1;
   }
   
   /* run iceboot.
    */
   if ((iceboot=fork())<0) {
      perror("fork");
      tcsetattr(0, TCSAFLUSH, &savedterm);
      return 1;
   }
   else if (iceboot==0) {
      /* child */
      if (execl("./Linux-i386/bin/iceboot", "iceboot", NULL)<0) {
	 perror("execl");
	 return 1;
      }
   }

   /* wait for iceboot to finish.
    */
   waitpid(iceboot, &status, 0);

   /* restore terminal.
    */
   tcsetattr(0, TCSAFLUSH, &savedterm);
   
   return status;
}

