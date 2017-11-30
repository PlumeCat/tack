
run : main
	./build/main

main : main.cpp
	g++ main.cpp -o ./build/main -std=c++14