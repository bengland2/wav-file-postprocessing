# this makefile requires that Fedora 35 libao package be installed, or equivalent in other distros
#

BINARIES = pulseaudio-example
#
# change from -O3 to -g for debugging
OPT_FLAGS=-g
CFLAGS=-Wall $(OPT_FLAGS) -D_REENTRANT 

all: $(BINARIES)

pulseaudio-example: pulseaudio-example.c readwav.h readwav.o
	$(CC) $(CFLAGS) -o $@ -D_REENTRANT readwav.o -lpulse -pthread -lm $<

clean:
	rm -rf $(BINARIES) *.o 

