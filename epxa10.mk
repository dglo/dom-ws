#
# files we import...
#
WD=..
export BINDIR=$(WD)/bin
export SFIBIN=$(BINDIR)/sfi.bin
export ICEBOOTHEX=$(BINDIR)/iceboot.hex
export ICEBOOTBINGZ=$(BINDIR)/iceboot.bin.gz
export STFSERVBINGZ=$(BINDIR)/stfserv.bin.gz
export MENUBINGZ=$(BINDIR)/menu.bin.gz
export YMODEMBINGZ=$(BINDIR)/ymodem.bin.gz

export KERNELX=$(WD)/public/loader/kernel.x
export RAWX=$(WD)/public/loader/raw.x
export RAWS=$(WD)/public/loader/raw.S

export EPXAHD = $(WD)/public
export EPXAH = $(EPXAHD)/booter/epxa.h
export PTESD = ../public
export PTES = $(PTESD)/booter/pte.S

export CRT0=$(WD)/lib/crt0.o
export LIBK=$(WD)/lib/libkernel.a

export ARM_HOME := /usr
export AFLAGS := -mcpu=arm920t  -I../public
SYSINCLUDE :=-I$(ARM_HOME)/arm-elf/arm-elf/include \
	-I$(ARM_HOME)/arm-elf/lib/gcc-lib/arm-elf/3.2/include \
	-I$(ARM_HOME)/arm-elf/include \
	-I../../../tools/expat-1.2/expat

export CPPFLAGS = -I$(EPXAHD) $(GENDEFS)
export CFLAGS = -mlittle-endian -mcpu=arm920 -Wall -nostdinc \
	$(CPPFLAGS) $(SYSINCLUDE) -I..

export CPP = arm-elf-cpp
export CC = arm-elf-gcc
export AS = arm-elf-as
export LD = arm-elf-ld -N
export OBJCOPY = arm-elf-objcopy

export SYSLIBS = $(ARM_HOME)/arm-elf/arm-elf/lib/libc.a \
	$(ARM_HOME)/arm-elf/lib/gcc-lib/arm-elf/3.2/libgcc.a \
	$(ARM_HOME)/arm-elf/lib/libz.a -lm

.c.o:
	$(CC) -c $(CFLAGS) $<

.elf.bin:
	$(OBJCOPY) -O binary $*.elf $*.bin
	$(CPP) $(CPPFLAGS) -DBINFILE=\"$*.bin\" -o $*-raw.i $(RAWS)
	$(AS) $(AFLAGS) -o $*-raw.o $*-raw.i
	$(LD) --script=$(RAWX) -o $*-raw.elf $*-raw.o
	$(OBJCOPY) -O binary $*-raw.elf $*.bin

FPGA_VERSIONS = epxa10/public/dom-fpga/fpga-versions.h

all: versions iceboot stfserv menu newbuild

iceboot:
	cd epxa10/booter; make config_files
	cd epxa10/loader; make all
	cd epxa10/hal; make all
	cd epxa10/iceboot; make all
	cd epxa10/booter; make bin

stfserv:
	cd epxa10/booter; make config_files
	cd epxa10/loader; make all
	cd epxa10/hal; make all
	cd epxa10/stf; make all
	cd epxa10/stf-apps; make $(STFSERVBINGZ)

menu:
	cd epxa10/booter; make config_files
	cd epxa10/loader; make all
	cd epxa10/hal; make all
	cd epxa10/stf; make all
	cd epxa10/stf-apps; make $(MENUBINGZ)

ymodem:
	cd epxa10/booter; make config_files
	cd epxa10/loader; make all
	cd epxa10/hal; make all
	cd epxa10/iceboot; make $(YMODEMBINGZ)

clean:
	cd $(PLATFORM)/booter; make clean
	cd $(PLATFORM)/loader; make clean
	cd $(PLATFORM)/iceboot; make clean
	cd $(PLATFORM)/hal; make clean
	cd $(PLATFORM)/stf-apps; make clean
	cd $(PLATFORM)/stf; make clean
	rm -f $(PLATFORM)/bin/* $(PLATFORM)/lib/*

versions:
	cd ../dom-fpga/scripts; ./mkhdr.sh > ../public/dom-fpga/fpga-versions.h
