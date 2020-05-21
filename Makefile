#
# rhn's hl2setup makefile
#

OS := $(shell uname)

ifeq ($(OS), Darwin)
	CC  = clang
	LL  = -lm
else
ifeq ($(OS), Linux)
	CC  = cc
	LL = -lm
	STD = -std=c99
else
	$(error OS not detected)
endif
endif

hl2setup:	hl2.c hl2setup.c
		$(info Building for $(OS))
		$(CC) hl2.c hl2setup.c $(LL) -o hl2setup $(STD) 

clean:
	rm hl2setup

