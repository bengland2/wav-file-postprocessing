# this makefile requires that Fedora 35 libao package be installed, or equivalent in other distros
#

BINARIES = pulseaudio-example test_readwav
#
# change from -O3 to -g for debugging
OPT_FLAGS=-g
CFLAGS=-Wall $(OPT_FLAGS) -D_REENTRANT

all: $(BINARIES)

# works on Pop!OS (Debian)
pulseaudio-example: pulseaudio-example.c readwav.h readwav.o
	$(CC) $(CFLAGS) -o $@ -D_REENTRANT readwav.o $< -lpulse -lm -lpthread

test_readwav: test_readwav.c readwav.h readwav.o
	$(CC) $(CFLAGS) -o $@ readwav.o $< 

# worked on Fedora 35
#pulseaudio-example: pulseaudio-example.c readwav.h readwav.o
#	$(CC) $(CFLAGS) -o $@ -D_REENTRANT readwav.o -lpulse -pthread -lm $<

clean:
	rm -rf $(BINARIES) *.o 

