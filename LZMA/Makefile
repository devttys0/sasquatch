CC=gcc
LDFLAGS=
CFLAGS=-DDBG=1 -I. -Wall -Werror
TARGET=lzma.o
DEPS=$(wildcard *.h)
SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)

.PHONY: brcm-lzma sqlzma clean

all: brcm-lzma
	$(CC) -c lzma.c $(CFLAGS) $(LDFLAGS)
	${AR} cr liblzma.a brcm-lzma/*.o lzma.o

sqlzma:
	cd sqlzma/C/Compress/Lzma/ && make -f sqlzma.mk

brcm-lzma:
	make -C brcm-lzma

clean:
	rm -f *.o *.a
	make -C brcm-lzma clean
	cd sqlzma/C/Compress/Lzma/ && make -f sqlzma.mk clean
