PKGS=poppler-glib
CFL:=$(shell pkg-config --cflags $(PKGS))
LFL:=$(shell pkg-config --libs $(PKGS))

CFLAGS=-g -Wall -W -std=gnu99 $(CFL)
CPPFLAGS=$(CFLAGS)
LDFLAGS=$(LFL)

qw: poppler.o main.o
	gcc -o qw poppler.o main.o $(LDFLAGS)

poppler.o: poppler.cc poppler.h

main.o: main.c poppler.h

clean:
	rm -f *.o qw
