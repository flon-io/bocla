
CFLAGS=-I../src -g -Wall -O3
LDLIBS=/usr/lib/libev.so
CC=c99
VPATH=../src

NAMES=$(patsubst $(VPATH)/%.c, ./%.o, $(wildcard $(VPATH)/*.c))
  # since wildcard doesn't care about VPATH...


server: $(NAMES)

clean:
	rm -f *.o
	rm -f *.so


.PHONY: clean

