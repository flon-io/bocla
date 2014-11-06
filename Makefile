
NAME=bocla

default: $(NAME).o

.DEFAULT spec clean:
	$(MAKE) -C tmp/ $@ NAME=$(NAME)

# copy updated version of dep libs into src/
#
upgrade:
	cp -v ../flutil/src/flutil.[ch] src/

supgrade:
	$(MAKE) -C spec/server/ upgrade

cs: clean spec

.PHONY: spec clean upgrade cs

