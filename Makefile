CC = clang

BUILD := debug
# later we might link dynamiclly for somethig we could've done with 2 lines of code..
PKGS.debug = 
PKGS.release = 
PKGS = raylib 

BIN_DIR := bin

CFLAGS.debug = -ggdb -O0 -fsanitize=address
CFLAGS.release = -Ofast 
CFLAGS=-Wall -Wextra -Wno-gnu -pthread -std=gnu2x ${CFLAGS.${BUILD}}

SRC=$(wildcard src/*.c) $(wildcard src/**/*.c) $(wildcard src/include/**/*.c)

INCLUDE_DIR = src/include

MURSUL_BUILD = $(CC) $(CFLAGS) `pkg-config --cflags $(PKGS)` -o bin/mursul $(SRC) `pkg-config --libs $(PKGS)` -I$(INCLUDE_DIR) 
# MURSUL_BUILD = $(CC) $(CFLAGS) -o bin/mursul $(SRC) -I$(INCLUDE_DIR) 

.PHONY = release clean
build: clean
	$(MURSUL_BUILD)

release: clean
	$(eval BUILD=release)
	$(MURSUL_BUILD)

clean:
	mkdir -p bin


