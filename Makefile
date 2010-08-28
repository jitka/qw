PKGS=poppler-glib
CFL:=$(shell pkg-config --cflags $(PKGS))
LFL:=$(shell pkg-config --libs $(PKGS))

CFLAGS=-g -Wall -W -std=gnu99 $(CFL)
CPPFLAGS=-g -Wall -W  $(CFL)
LDFLAGS=$(LFL)


sources=$(wildcard *.c)

qw: poppler.o $(sources:.c=.o)
	@gcc -o qw poppler.o $(sources:.c=.o) $(LDFLAGS)

%.d: %.c
	@set -e; rm -f $@; \
		gcc -MM $< | \
		sed -e 's,\($*\)\.o[ :]*,\1.o $@ : ,g' > $@;
	@echo "\t@gcc $(CFLAGS) -c $<" >> $@

include $(sources:.c=.d)
poppler.o: poppler.cc poppler.h pixbuffer.h
	@g++ $(CPPFLAGS) -c $<

clean:
	@rm -f *.o *.d qw
