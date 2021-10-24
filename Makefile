CXX = g++
CXX_FLAGS = -c -g -Iinclude/ -std=c++17
LD_FLAGS = 

sources = $(wildcard src/*.cpp)
objects = $(sources:.cpp=.o)
output = "./bin"

%.o: %.cpp
	$(CXX) $(CXX_FLAGS) $< -o $@

main: $(objects)
	$(CXX) $(LD_FLAGS) $(objects) -o main

clean:
	rm main
	rm src/*.o

run: main
	./main source.scala
