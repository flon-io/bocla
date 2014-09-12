
#CFLAGS=-I../src -g -Wall -O3
#LDLIBS=
#CC=c99

build:
	cp -uv ../../../shervin/src/*.h src/
	cp -uv ../../../shervin/src/*.c src/
	$(MAKE) -C tmp/

clean:
	$(MAKE) -C tmp/ clean

.PHONY: build clean

