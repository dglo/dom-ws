/* dumpin.c, dump standard input to standard output...
 */
#include <stdio.h>

#include <unistd.h>

int main(int argc, char *argv[]) {
   while (1) {
      char ch;
      int ret = read(0, &ch, 1);
      
      if (ret==0) {
	 fprintf(stderr, "EOF!\n");
	 return 1;
      }
      else if (ret<0) {
	 perror("read");
	 return 1;
      }
      else if (ch=='q') {
	 break;
      }
      else {
	 printf(" %02x: '%c'\n", ch, (ch>=0x20 && ch<0x7f) ? ch : '?');
      }
   }
   return 0;
}

