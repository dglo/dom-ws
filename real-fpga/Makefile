.c.o:
	gcc -Wall -c $<

real-app: real-app.c
	gcc -Wall -I../Linux-i386/public -L../Linux-i386/lib \
		-DICESOFT_BUILD=\"100\" \
		-o real-app real-app.c -lhal

