# Makefile

CXX := g++
CXX_FLAGS := -w -g -std=c++17 \
	-I./include

source := $(wildcard src/*.cpp)
target := dist/main.exe

clean:
	rm -r dist

build: $(source)
	$(CXX) $(CXX_FLAGS) $(source) -o $(target)

run: build
	$(target) source.str
