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
	cd ../hal/html; tar cf - . | ssh glacier.lbl.gov "(cd ~/public_html/dom-mb; tar xf -)"
	cd epxa10/stf-docs; make install
	cd epxa10/iceboot-docs; make install

w942: w942.c
	gcc -o w942 -Wall w942.c

domserv: domserv.c
	gcc -o domserv -Wall domserv.c -lutil

dhserv: dhserv.c
	gcc -o dhserv -Wall dhserv.c

xmln: xmln.c
	gcc -o xmln -Wall -O -g xmln.c -lexpat

dhclient: dhclient.c
	gcc -o dhclient -Wall dhclient.c

sendfile: sendfile.c
	gcc -o sendfile -Wall sendfile.c

tcalcycle: tcalcycle.c
	gcc -o tcalcycle -Wall tcalcycle.c

decoderaw: decoderaw.c
	gcc -o decoderaw -Wall decoderaw.c

HWPROJECTS=dom-cpld dom-fpga
SWPROJECTS=hal dom-loader configboot iceboot stf dom-ws dor-test
PROJECTS=$(HWPROJECTS) $(SWPROJECTS)

diff:
	cd ..; cvs diff $(SWPROJECTS)

update:
	cd ..; cvs update $(SWPROJECTS)

hwdiff:
	cd ..; cvs diff $(HWPROJECTS)

hwupdate:
	cd ..; cvs update $(HWPROJECTS)

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

rev-2004-06.pdf: rev-2004-06.tex
	latex rev-2004-06.tex
	latex rev-2004-06.tex
	dvips rev-2004-06.dvi
	ps2pdf rev-2004-06.ps

dhsave: dhsave.c
	gcc -Wall -g -o dhsave dhsave.c

se: se.c
	gcc -Wall -g -o se se.c -lutil

