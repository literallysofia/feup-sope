CC=gcc
CFLAGS=-Wall

all: dir gerador parque

dir:
	@mkdir -p bin

gerador: gerador.c
	@$(CC) gerador.c $(CFLAGS) -o bin/gerador -pthread

parque: parque.c
	@$(CC) parque.c $(CFLAGS) -o bin/parque -pthread