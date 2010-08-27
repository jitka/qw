PKGS=poppler-glib
CFL:=$(shell pkg-config --cflags $(PKGS))
LFL:=$(shell pkg-config --libs $(PKGS))

CFLAGS=-g -Wall -W -std=gnu99 $(CFL)
CPPFLAGS=-g -Wall -W  $(CFL)
LDFLAGS=$(LFL)

qw: pixbuffer.o inputs.o poppler.o main.o
	gcc -o qw pixbuffer.o inputs.o poppler.o main.o $(LDFLAGS)

inputs.o: inputs.c inputs.h
poppler.o: poppler.cc poppler.h pixbuffer.h
pixbuffer.o: pixbuffer.c pixbuffer.h

main.o: main.c pixbuffer.h inputs.h poppler.h

clean:
	rm -f *.o qw
