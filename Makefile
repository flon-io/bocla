
NAME=bocla

default: $(NAME).o

.DEFAULT spec clean:
	$(MAKE) -C tmp/ $@ NAME=$(NAME)

# copy updated version of dep libs into src/
#
upgrade:
	cp -v ../flutil/src/flutil.[ch] src/
	cp -v ../flutil/src/flu64.[ch] src/

.PHONY: spec clean upgrade

