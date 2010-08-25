PKGS=poppler-glib
CFL:=$(shell pkg-config --cflags $(PKGS))
LFL:=$(shell pkg-config --libs $(PKGS))

CFLAGS=-g -Wall -W -std=gnu99 $(CFL)
CPPFLAGS=-g -Wall -W  $(CFL)
LDFLAGS=$(LFL)

qw: inputs.o poppler.o main.o
	gcc -o qw inputs.o poppler.o main.o $(LDFLAGS)

inputs.o: inputs.c inputs.h

poppler.o: poppler.cc poppler.h

main.o: main.c inputs.h poppler.h

clean:
	rm -f *.o qw
