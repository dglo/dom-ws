PLATFORM=epxa10
#PLATFORM=Linux-i386

export PROJECT_TAG=devel
export ICESOFT_BUILD=$(shell cat build_num)
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

diff:
	cd ../dom-loader; cvs diff -d .
	cd ../hal; cvs diff -d .
	cd ../iceboot; cvs diff -d .
	cd ../stf; cvs diff -d .

doc:
	cd ../hal; doxygen doxygen.conf

doc.install: doc
	cd ../hal/html; tar cf - . | (cd ~/public_html/dom-mb; tar xf -)














