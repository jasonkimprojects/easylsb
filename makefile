# Copyright 2019 Jason Kim. All rights reserved.
# g++ Makefile to compile EasyLSB. 
# bitmapparser.h MUST be in the same directory as EasyLSB.cpp!
all:
	g++ -std=c++17 -Wall -Werror -pedantic -o3 EasyLSB.cpp -o EasyLSB
# Compile with -g3 flag for easier debugging
debug:
	g++ -std=c++17 -Wall -Werror -pedantic -g3 EasyLSB.cpp -o EasyLSB_debug
clean:
	rm -f EasyLSB
	rm -f EasyLSB_debug