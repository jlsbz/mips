cc = g++
prog = code
($prog) : main.cpp
	g++ -o code main.cpp -std=c++14 -O2
clean:
	rm -rf code
