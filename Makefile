# Makefile
CC := gcc
CFLAGS := -std=c11 -m32 -march=i386 \
		-fsanitize=address -fno-omit-frame-pointer -ggdb
TARGET := tack

INCLUDE := ./include
HEADERS = $(wildcard $(INCLUDE)/*.h)
#SRC = $(wildcard src/*.c)
#OBJ = $(patsubst %.c,%.o,$(SRC))

#%.o: %.c $(HEADERS)
#	@$(CC) $(CFLAGS) -c -o $@ $< -I$(INCLUDE)

clean:
	@rm -rf bin/*
	@find . -name "*.o" -delete

#build: clean $(OBJ)
#	@$(CC) $(CFLAGS) -o bin/$(TARGET) $(OBJ)

build: src/main.c
	g++ -std=c11 src/main.c -o bin/$(TARGET)

run: build
	@bin/$(TARGET)
