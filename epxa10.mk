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
export LOOKBACKBINGZ=$(BINDIR)/lookback.bin.gz
export LOOKBACKBINGZ=$(BINDIR)/domapp.bin.gz
export DOMCALBINGZ=$(BINDIR)/domcal.bin.gz

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
	-I$(ARM_HOME)/arm-elf/include

export CPPFLAGS = -I$(EPXAHD) $(GENDEFS)
export CFLAGS = -mlittle-endian -mcpu=arm920 -Wall -nostdinc \
	$(CPPFLAGS) $(SYSINCLUDE) -I..

export CPP = arm-elf-cpp
export CC = arm-elf-gcc
export AS = arm-elf-as
export LD = arm-elf-ld -N
export OBJCOPY = arm-elf-objcopy

export SYSLIBS = $(ARM_HOME)/arm-elf/arm-elf/lib/libc.a \
	$(ARM_HOME)/arm-elf/arm-elf/lib/libm.a \
	$(ARM_HOME)/arm-elf/lib/gcc-lib/arm-elf/3.2/libgcc.a \
	$(ARM_HOME)/arm-elf/lib/libz.a 

.c.o:
	$(CC) -c $(CFLAGS) $<

.elf.bin:
	$(OBJCOPY) -O binary $*.elf $*.bin
	$(CPP) $(CPPFLAGS) -DBINFILE=\"$*.bin\" -o $*-raw.i $(RAWS)
	$(AS) $(AFLAGS) -o $*-raw.o $*-raw.i
	$(LD) --script=$(RAWX) -o $*-raw.elf $*-raw.o
	$(OBJCOPY) -O binary $*-raw.elf $*.bin


all: pld-versions versions iceboot stfserv menu stfsfe newbuild domcal

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
	cd epxa10/stf-apps; make "CFLAGS=$(CFLAGS) -DVERBOSE" $(MENUBINGZ)

ymodem:
	cd epxa10/booter; make config_files
	cd epxa10/loader; make all
	cd epxa10/hal; make all
	cd epxa10/iceboot; make $(YMODEMBINGZ)

lookback:
	cd epxa10/booter; make config_files
	cd epxa10/loader; make all
	cd epxa10/hal; make all
	cd epxa10/iceboot; make $(LOOKBACKBINGZ)

clean:
	cd $(PLATFORM)/booter; make clean
	cd $(PLATFORM)/loader; make clean
	cd $(PLATFORM)/iceboot; make clean
	cd $(PLATFORM)/hal; make clean
	cd $(PLATFORM)/stf-apps; make clean
	cd $(PLATFORM)/stf; make clean
	cd $(PLATFORM)/domapp; make -f ../../domapp.mk clean
	cd $(PLATFORM)/stf-docs; make clean
	cd $(PLATFORM)/iceboot-docs; make clean
	cd $(PLATFORM)/dom-cal; make clean
	rm -f $(PLATFORM)/bin/* $(PLATFORM)/lib/* sendfile

stfsfe:
	cd epxa10/stf-sfe; make

domapp:
	cd $(PLATFORM)/booter; make config_files
	cd $(PLATFORM)/loader; make all
	cd $(PLATFORM)/hal; make all
	cd $(PLATFORM)/domapp; make -f ../../domapp.mk $(DOMAPPBINGZ)

domcalbase:
	cd $(PLATFORM)/booter; make config_files
	cd $(PLATFORM)/loader; make all
	cd $(PLATFORM)/hal; make all
	cd $(PLATFORM)/iceboot; make all

domcal2: domcalbase
	cd $(PLATFORM)/dom-cal; make clean
	cd $(PLATFORM)/dom-cal; make -I "../iceboot" "CFLAGS=$(CFLAGS) -DDOMCAL_REV2" $(DOMCALBINGZ)
	cd $(PLATFORM)/dom-cal; mv $(DOMCALBINGZ) $(BINDIR)/domcal2.bin.gz

