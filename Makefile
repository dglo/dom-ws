PLATFORM=epxa10
#PLATFORM=Linux-i386

export PROJECT_TAG=devel
export ICESOFT_BUILD=$(shell /bin/sh getbld.sh)
export LIBHAL=../lib/libhal.a

export LIBEXPAT=../../../tools/$(PLATFORM)/lib/libexpat.a

export GENDEFS=-DICESOFT_BUILD=$(ICESOFT_BUILD) -DPROJECT_TAG=$(PROJECT_TAG)

include $(PLATFORM).mk

links:
	gawk -f mkws.awk dom.ws

remove:
	rm -rf $(PLATFORM)

tags:
	cd $(PLATFORM); find . -name '*.[ch]' | ctags - 

update:
	cd ../dom-loader; cvs update -d .
	cd ../hal; cvs update -d .
	cd ../iceboot; cvs update -d .
	cd ../stf; cvs update -d .
	cvs update -d .

diff:
	cd ../dom-loader; cvs diff .
	cd ../hal; cvs diff .
	cd ../iceboot; cvs diff .
	cd ../stf; cvs diff .
	cvs diff .

hwdiff:
	cd ../dom-fpga; cvs diff .
	cd ../dom-cpld; cvs diff .

hwupdate:
	cd ../dom-fpga; cvs update -d .
	cd ../dom-cpld; cvs update -d .

doc:
	cd ../hal; doxygen doxygen.conf

doc.install: doc
	cd ../hal/html; tar cf - . | (cd ~/public_html/dom-mb; tar xf -)

newbuild:
	/bin/sh newbld.sh

domserv: domserv.c
	gcc -o domserv -Wall domserv.c -lutil

sendfile: sendfile.c
	gcc -o sendfile -Wall sendfile.c

iceboot.sbi.gz: ../dom-fpga/resources/epxa10/simpletest_com_13.sbi
	gzip -c ../dom-fpga/resources/epxa10/simpletest_com_13.sbi > \
		iceboot.sbi.gz

stf.sbi.gz: ../dom-fpga/resources/epxa10/simpletest_com_13.sbi
	gzip -c ../dom-fpga/resources/epxa10/simpletest_com_13.sbi > \
		stf.sbi.gz

release.hex: mkrelease.sh domserv all iceboot.sbi.gz stf.sbi.gz
	/bin/sh mkrelease.sh ./epxa10/bin/iceboot.bin.gz \
		./epxa10/bin/stfserv.bin.gz \
		../iceboot/resources/startup.fs \
		iceboot.sbi.gz \
		stf.sbi.gz \
		domapp.sbi.gz \
		domapp.bin.gz

DEVEL_RELEASE=0
DEVEL_BUILD=3

tag-build:
	cd ../hal; cvs tag devel-$(DEVEL_RELEASE)-$(DEVEL_BUILD) .
	cd ../dom-loader; cvs tag devel-$(DEVEL_RELEASE)-$(DEVEL_BUILD) .
	cd ../configboot; cvs tag devel-$(DEVEL_RELEASE)-$(DEVEL_BUILD) .
	cd ../dom-cpld; cvs tag devel-$(DEVEL_RELEASE)-$(DEVEL_BUILD) .
	cd ../dom-fpga; cvs tag devel-$(DEVEL_RELEASE)-$(DEVEL_BUILD) .
	cd ../iceboot; cvs tag devel-$(DEVEL_RELEASE)-$(DEVEL_BUILD) .
	cd ../stf; cvs tag devel-$(DEVEL_RELEASE)-$(DEVEL_BUILD) .
	cvs tag devel-$(DEVEL_RELEASE)-$(DEVEL_BUILD) .

viewtags:
	@echo hal
	@cd ../hal; cvs status -v project.mk | \
		grep 'V[0-9][0-9]-[0-9][0-9]-[0-9][0-9]'
	@echo dom-loader
	@cd ../dom-loader; cvs status -v project.mk | \
		grep 'V[0-9][0-9]-[0-9][0-9]-[0-9][0-9]'
#	@echo configboot
#	@cd ../configboot; \
#		cvs status -v \
#			./private/epxa10/configboot/configboot.c | \
#		grep 'V[0-9][0-9]-[0-9][0-9]-[0-9][0-9]'
	@echo dom-cpld
	@cd ../dom-cpld; cvs status -v Dom_Cpld_rev2.vhd | \
		grep 'V[0-9][0-9]-[0-9][0-9]-[0-9][0-9]'
	@echo dom-fpga
	@cd ../dom-fpga; cvs status -v scripts/mkmif.sh | \
		grep 'V[0-9][0-9]-[0-9][0-9]-[0-9][0-9]'
	@echo iceboot
	@cd ../iceboot; cvs status -v project.mk | \
		grep 'V[0-9][0-9]-[0-9][0-9]-[0-9][0-9]'
	@echo stf
	@cd ../stf; cvs status -v project.mk | \
		grep 'V[0-9][0-9]-[0-9][0-9]-[0-9][0-9]'
	@echo dom-ws
	@cvs status -v Makefile | grep 'V[0-9][0-9]-[0-9][0-9]-[0-9][0-9]'




