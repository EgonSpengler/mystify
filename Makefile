default: mystify

mystify: mystify.c
	gcc -std=gnu99 -g -o mystify mystify.c -L/usr/lib -lX11

clean:
	rm mystify
