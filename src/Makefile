CC=gcc
CFLAGS=-O3

all: simkernel shmdisplay

clean:
	rm -f simkernel shmdisplay

simkernel: simkernel.c
	$(CC) $(CFLAGS) simkernel.c -o simkernel -lrt

shmdisplay: shmdisplay.c
	$(CC) $(CFLAGS) shmdisplay.c -o shmdisplay -lSDL2 -lSDL2_image -lrt
