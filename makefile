all:
	g++ -std=c++17 -Wall -Werror -pedantic -o3 EasyLSB.cpp -o EasyLSB

debug:
	g++ -std=c++17 -Wall -Werror -pedantic -g3 EasyLSB.cpp -o EasyLSB_debug

clean:
	rm -f EasyLSB
	rm -f EasyLSB_debug