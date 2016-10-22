headers=ast.h symbol.h token.h bnf.h

run : awesomescript
	./awesomescript

awesomescript : awesomescript.cpp
	g++ -std=c++11 \
	-o awesomescript \
	awesomescript.cpp

awesomescript.cpp : $(headers)
	
