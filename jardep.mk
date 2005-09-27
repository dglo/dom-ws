#
# create dependency files for jars in current directory...
#
.SUFFIXES: .jar .dep

vpath %.jar ../lib:../tools/lib
vpath %.dep jardep.deps

.jar.dep:
	./jardep.sh $< > jardep.deps/$@

JARS=$(shell cd ../lib; lessecho *.jar; cd ../tools/lib; lessecho *.jar)
DEPS=$(JARS:%.jar=%.dep)

all: jardep.mk jardep.sh jardep.deps jardep.deps/all.dep jardep.deps/all.jardep

clean:
	rm -rf jardep.deps 
	rm -f filldeps filldeps.o

jardep.deps:
	if [[ ! -d jardep.deps ]]; then mkdir jardep.deps; fi

filldeps: filldeps.c
	gcc -O -g -Wall -o filldeps filldeps.c

jardep.deps/all.jardep: filldeps jardep.deps/all.dep
	cat jardep.deps/all.dep | ./filldeps -jardep > jardep.deps/all.jardep

jardep.deps/all.dep: $(DEPS) filldeps
	rm -f jardep.deps/all.dep
	cat jardep.deps/*.dep | ./filldeps > jardep.deps/all.dep

