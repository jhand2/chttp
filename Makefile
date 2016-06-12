chttp : server.o
	gcc -Wall -o chttp server.o

server.o : server.c
	gcc -Wall -g -c server.c

clean :
	rm -f .*.swp showip chttp
	rm -f *.gch *.o
