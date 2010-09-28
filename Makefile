PKGS=poppler-glib libspectre
CFL:=$(shell pkg-config --cflags $(PKGS))
LFL:=$(shell pkg-config --libs $(PKGS))

CFLAGS=-g -Wall -W -std=gnu99 $(CFL)
CPPFLAGS=-g -Wall -W  $(CFL)
LDFLAGS=$(LFL)


sources=inputs main pixbuffer render backend spectre

qw: poppler.o $(addsuffix .o,$(sources))
	@echo LD $@
	@gcc -o $@ $^ $(LDFLAGS)

%.d: %.c
	@set -e; rm -f $@; \
		gcc -MM $< | \
		sed -e 's,\($*\)\.o[ :]*,\1.o $@ : ,g' > $@;

%.o: %.c
	@echo CC $<
	@gcc $(CFLAGS) -c $< -o $@

poppler.o: poppler.cc poppler.h pixbuffer.h
	@echo CPP $<
	@g++ $(CPPFLAGS) -c $< -o $@

clean:
	@echo CLEAN
	@rm -f *.o *.d qw

messages:
	xgettext *.c --from-code=UTF-8 -k_

-include $(addsuffix .d,$(sources))
