default: all

all: coloredsquares double mystify

coloredsquares: coloredsquares.c
	gcc -std=gnu99 -o coloredsquares coloredsquares.c -L/usr/lib -lX11

double: double.c
	gcc -std=gnu99 -o double double.c -L/usr/lib -lX11

mystify: mystify.c
	gcc -std=gnu99 -g -o mystify mystify.c -L/usr/lib -lX11

clean:
	rm coloredsquares double mystify
