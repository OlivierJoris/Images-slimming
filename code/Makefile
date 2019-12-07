CC=gcc
LD=gcc
CFLAGS=--std=c99 --pedantic -Wall -O3 -W -Wmissing-prototypes
LDFLAGS=

all: slimming

slimming: PNM.o mainSlimming.o slimming.o
	$(LD) -o slimming mainSlimming.o PNM.o slimming.o $(LDFLAGS)

mainSlimming.o: mainSlimming.c slimming.h PNM.h
	$(CC) -c mainSlimming.c -o mainSlimming.o $(CFLAGS)

PNM.o: PNM.c PNM.h
	$(CC) -c PNM.c -o PNM.o $(CFLAGS)

slimming.o: slimming.c slimming.h PNM.h
	$(CC) -c slimming.c -o slimming.o $(CFLAGS)

clean:
	rm -f *.o
	rm -f slimming
	clear
