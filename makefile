run : awesomescript.exe
	./awesomescript.exe

awesomescript.exe : awesomescript.cpp
	mingw32-g++ --std=c++11 -m32 -o awesomescript.exe awesomescript.cpp