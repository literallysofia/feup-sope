CC=gcc
CFLAGS=-Wall

all: dir rmdup lstdir

dir:
	@mkdir -p bin

rmdup: rmdup.c
	@$(CC) rmdup.c $(CFLAGS) -o bin/rmdup

lstdir: lstdir.c
	@$(CC) lstdir.c $(CFLAGS) -o bin/lstdir
