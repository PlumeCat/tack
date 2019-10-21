# Makefile
CC := gcc
CFLAGS := -std=c11 -m64 -D_DEBUG
TARGET := langs

SRC := $(wildcard src/*.c)

clean:
	@rm -rf bin/*

build: $(SRC)
	@gcc $(CFLAGS) $(SRC) -o bin/$(TARGET)

run: build
	@bin/$(TARGET)
