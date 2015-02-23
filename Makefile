api: api.o
	gcc -g -o api api.c -lpthread

clean:
	rm *.o