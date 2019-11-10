# Makefile

# CXX := g++
CXX := clang

CXX_FLAGS := \
	-I./include \
	-std=c++17 \
	-w \
	-g

SRC=$(wildcard src/*.cpp)
TARGET=dist/main.exe

clean:
	rm -r dist

build: $(SRC)
	$(CXX) $(CXX_FLAGS) $(SRC) -o $(TARGET)

run: build
	$(TARGET) source.str
