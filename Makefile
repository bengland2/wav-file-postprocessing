# this makefile requires that Fedora 35 libao package be installed, or equivalent in other distros
#

BINARIES = pulseaudio-example copy_wav_file pacat-simple
#
# change from -O3 to -g for debugging
OPT_FLAGS=-O3
CFLAGS=-Wall $(OPT_FLAGS)

all: $(BINARIES)

# works on Pop!OS (Debian)
pulseaudio-example: pulseaudio-example.c wav_file_access.h wav_file_access.o
	$(CC) $(CFLAGS) -o $@ -D_REENTRANT wav_file_access.o $< -lpulse -lm -lpthread

copy_wav_file: copy_wav_file.c wav_file_access.h wav_file_access.o
	$(CC) $(CFLAGS) -o $@ wav_file_access.o $< 

# worked on Fedora 35
#pulseaudio-example: pulseaudio-example.c wav_file_access.h wav_file_access.o
#	$(CC) $(CFLAGS) -o $@ -D_REENTRANT wav_file_access.o -lpulse -pthread -lm $<

# example of simple pulseaudio API 
pacat-simple: pacat-simple.c wav_file_access.h wav_file_access.o
	$(CC) $(CFLAGS) -o $@ -D_REENTRANT wav_file_access.o $< -lpulse-simple -lpulse -lm -lpthread

clean:
	rm -rf $(BINARIES) *.o 

