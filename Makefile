#
# main makefile for the dom-ws project
#
PLATFORM=epxa10
#PLATFORM=Linux-i386

export PROJECT_TAG=devel
export ICESOFT_BUILD:=$(shell /bin/sh getbld.sh)
export LIBHAL=../lib/libhal.a

export GENDEFS=-DICESOFT_BUILD=$(ICESOFT_BUILD) -DPROJECT_TAG=$(PROJECT_TAG)

include $(PLATFORM).mk

links:
	gawk -f mkws.awk dom.ws

remove:
	rm -rf $(PLATFORM)

tags:
	cd $(PLATFORM); find . -name '*.[ch]' | etags - 

doc:
	cd ../hal; doxygen doxygen.conf
	cd epxa10/stf-docs; make stf-tests.pdf
	cd epxa10/iceboot-docs; make iceboot-ug.pdf

doc.install: doc
	cd ../hal/html; tar cf - . | (cd ~/public_html/dom-mb; tar xf -)

domserv: domserv.c
	gcc -o domserv -Wall domserv.c -lutil

dhserv: dhserv.c
	gcc -o dhserv -Wall dhserv.c

dt: dt.c
	gcc -o dt -Wall -O -g dt.c

xmln: xmln.c
	gcc -o xmln -Wall -O -g xmln.c -lexpat

dhclient: dhclient.c
	gcc -o dhclient -Wall dhclient.c

sendfile: sendfile.c
	gcc -o sendfile -Wall sendfile.c

iceboot.sbi.gz: ../dom-fpga/resources/epxa10/simpletest_com_13.sbi
	gzip -c ../dom-fpga/resources/epxa10/simpletest_com_13.sbi > \
		iceboot.sbi.gz

stf.sbi.gz: ../dom-fpga/resources/epxa10/simpletest_com_13.sbi
	gzip -c ../dom-fpga/resources/epxa10/simpletest_com_13.sbi > \
		stf.sbi.gz

domapp.sbi.gz: ../dom-fpga/resources/epxa10/simpletest_com_13.sbi
	gzip -c ../dom-fpga/resources/epxa10/simpletest_com_13.sbi > \
		domapp.sbi.gz

release.hex: mkrelease.sh domserv all iceboot.sbi.gz stf.sbi.gz domapp.sbi.gz
	/bin/sh mkrelease.sh ./epxa10/bin/iceboot.bin.gz \
		./epxa10/bin/stfserv.bin.gz \
		../iceboot/resources/startup.fs \
		iceboot.sbi.gz \
		stf.sbi.gz \
		domapp.sbi.gz \
		domapp.bin.gz

HWPROJECTS=dom-cpld dom-fpga
SWPROJECTS=hal dom-loader configboot iceboot stf dom-ws
PROJECTS=$(HWPROJECTS) $(SWPROJECTS)
DEVEL_RELEASE=1
DEVEL_BUILD=1

commit:
	cd ..; cvs commit $(SWPROJECTS)

diff:
	cd ..; cvs diff $(SWPROJECTS)

update:
	cd ..; cvs update $(SWPROJECTS)

hwdiff:
	cd ..; cvs diff $(HWPROJECTS)

hwupdate:
	cd ..; cvs update $(HWPROJECTS)

tag-build:
	cd ..; cvs tag devel-$(DEVEL_RELEASE)-$(DEVEL_BUILD) $(PROJECTS)

tag-diff:
	cd ..; cvs diff -r devel-$(DEVEL_RELEASE)-$(DEVEL_BUILD) $(PROJECTS)

branch-build:
	cd ..; cvs rtag -b -r devel-$(DEVEL_RELEASE)-$(DEVEL_BUILD) \
		devel-$(DEVEL_RELEASE) $(PROJECTS)

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

stfmode.class: stfmode.java
	javac stfmode.java

alloff.class: alloff.java
	javac alloff.java

addresults.class: addresults.java
	javac addresults.java

addhw.class: addhw.java
	javac addhw.java

html-install: dt dhclient
	if [[ ! -d /usr/lib/cgi-bin/stf/xml/bin ]]; then mkdir -p /usr/lib/cgi-bin/stf/xml/bin; fi
	cd epxa10/stf-sfe; make html-install
	cd epxa10/std-tests; make install
	cp dhclient dt /usr/lib/cgi-bin/stf/xml/bin


VERDIR = $(PLATFORM)/public/dom-fpga
PVERDIR = $(PLATFORM)/public/dom-cpld
FPGA_VERSIONS = ../../dom-ws/$(VERDIR)/fpga-versions.h

versions:
	if [[ ! -d $(VERDIR) ]]; then mkdir -p $(VERDIR); fi
	cd ../dom-fpga/scripts; ./mkhdr.sh > $(FPGA_VERSIONS)

pld-versions:
	if [[ ! -d $(PVERDIR) ]]; then mkdir -p $(PVERDIR); fi
	cd ../dom-cpld; make
	cp ../dom-cpld/pld-version.h $(PVERDIR)

