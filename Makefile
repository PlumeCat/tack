# Makefile

CXX := g++
CXX_FLAGS := \
	-I./include \
	-std=c++17 \
	-w \
	-g

source := $(wildcard src/*.cpp)
target := dist/main.exe

clean:
	rm -r dist

build: $(source)
	$(CXX) $(CXX_FLAGS) $(source) -o $(target)

run: build
	$(target) source.str
