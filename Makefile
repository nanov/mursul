CC = clang

BUILD := debug
# later we might link dynamiclly for somethig we could've done with 2 lines of code..
PKGS.debug =
PKGS.release =
PKGS = raylib

BIN_DIR := bin

CFLAGS.debug = -ggdb -O0 -fsanitize=address
CFLAGS.release = -Ofast
CFLAGS=-Wall -Wextra -Wno-gnu -std=gnu2x ${CFLAGS.${BUILD}}

SRC=$(wildcard src/main.c)
GENERATOR_SRC=$(wildcard src/generator.c)

INCLUDE_DIR = src/include

MURSUL_BUILD = $(CC) $(CFLAGS) `pkg-config --cflags $(PKGS)` -o bin/mursul $(SRC) -lcurl `pkg-config --libs $(PKGS)` -I$(INCLUDE_DIR)
GENERTOR_BUILD = $(CC) $(CFLAGS) -o bin/generate $(GENERATOR_SRC)

.PHONY = release clean
build: clean
	$(MURSUL_BUILD)

release: clean
	$(eval BUILD=release)
	$(MURSUL_BUILD)

generator: clean
	$(GENERTOR_BUILD)

clean:
	mkdir -p bin
