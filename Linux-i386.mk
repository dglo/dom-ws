#
# files we import...
#
WD=..
export BINDIR=$(WD)/bin
export SFIBIN=$(BINDIR)/sfi
export ICEBOOTBIN=$(BINDIR)/iceboot
export STFSERVBIN=$(BINDIR)/stfserv
export MENUBIN=$(BINDIR)/menu

export EPXAHD = $(WD)/public
export EPXAH = 

export CPPFLAGS = -I../public -I.. -I../../../tools/expat-1.2/expat $(GENDEFS)
export CFLAGS = -g -Wall $(CPPFLAGS)

export CPP = cpp
export CC = gcc
export AS = as
export LD = ld
export OBJCOPY = objcopy

export SYSLIBS = -lz -lm

export LIBEXPAT=/usr/lib/libexpat.a

.c.o:
	$(CC) -c $(CFLAGS) $<

all: iceboot stfserv menu newbuild

iceboot:
	cd Linux-i386/hal; make all
	cd Linux-i386/iceboot; make all

stfserv:
	cd Linux-i386/hal; make all
	cd Linux-i386/stf; make all
	cd Linux-i386/stf-apps; make $(STFSERVBIN)

menu:
	cd Linux-i386/hal; make all
	cd Linux-i386/stf; make all
	cd Linux-i386/stf-apps; make $(MENUBIN)

clean:
	cd $(PLATFORM)/iceboot; make clean
	cd $(PLATFORM)/hal; make clean
	cd $(PLATFORM)/stf-apps; make clean
	cd $(PLATFORM)/stf; make clean
	rm -f $(PLATFORM)/bin/* $(PLATFORM)/lib/*









