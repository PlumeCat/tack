headers=ast.h symbol.h token.h bnf.h

ifeq ($(OS),Windows_NT)
    binary = ./build/awesomescript.exe
else
    binary = ./build/awesomescript
endif

run : awesomescript
	$(binary)

awesomescript : awesomescript.cpp $(headers)
	g++ -std=c++11 \
	-o $(binary) \
	awesomescript.cpp