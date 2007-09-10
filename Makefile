#
# main makefile for the dom-ws project
#
PLATFORM=epxa10
#PLATFORM=Linux-i386

export PROJECT_TAG=devel
export ICESOFT_BUILD:=$(shell /bin/sh getbld.sh)
export LIBHAL=../lib/libhal.a

export GENDEFS=-DICESOFT_BUILD=$(ICESOFT_BUILD) -DPROJECT_TAG=$(PROJECT_TAG)
export XMLDESCPATH=/home/jacobsen/icecube/work/dommb-releases/stf-prod-110/stf-schema

include $(PLATFORM).mk

links:
	gawk -f mkws.awk dom.ws

doc:
	cd ../hal; doxygen doxygen.conf
#	cd epxa10/stf-docs; make stf-tests.pdf
#	cd epxa10/iceboot-docs; make iceboot-ug.pdf

#doc.install: doc
#	cd ../hal/html; tar cf - . | ssh glacier.lbl.gov "(cd ~/public_html/dom-mb; tar xf -)"
#	cd epxa10/stf-docs; make install
#	cd epxa10/iceboot-docs; make install

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

#
# release stuff...
#
REL=$(shell cat prod.num)
RTB=dom-mb-$(REL).tar.gz
IMPORTS=dom-cal dom-cpld dom-fpga dom-loader dom-ws fb-cpld hal \
	iceboot stf testdomapp domapp

release: $(RTB)
	cp ChangeLog /net/usr/pdaq/packaged-releases/DOM-MB/stable_hex/RELEASE_NOTES
	cp $(RTB) /net/usr/pdaq/packaged-releases/DOM-MB/stable_hex
	@cvs tag rel-$(REL)
#	@cp ../.git/refs/tags/rel-$(REL) tags
#	@cg add tags/rel-$(REL)
#	@cg commit -m "release `cat prod.num`" tags/rel-$(REL)
#	@for i in $(IMPORTS); do \
#		( cd ../$$i && \
#		  cvs -z9 import -m "dom-mb `cat ../dom-ws/prod.num`" \
#		     $$i rel-4xx rel-$(REL) ) | tee import.log \
#	 done
	@echo "`cat prod.num` 1 + p" | dc > prod.num.2
	@mv prod.num.2 prod.num
	@cvs commit -m "updated tag" prod.num

$(RTB):
	@./mkprod.sh
	@echo created: $(RTB)

#
# for convenience...
#
rtb: $(RTB)

ChangeLog: prod.num
	@./mklog.sh > ChangeLog.$(REL)
	@cat ChangeLog.$(REL) ChangeLog > t
	@rm -f ChangeLog.$(REL)
	@mv t ChangeLog
	@echo "updated ChangeLog, do not forget to edit..."
