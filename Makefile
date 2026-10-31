all:
	gcc -Wall -shared -fPIC toralize.c -o toralize.so -ldl -D_GNU_SOURCE
