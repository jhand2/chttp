chttp : server.o main.o
	gcc -Wall -o chttp server.o main.o

main.o: src/main.c
	gcc -Wall -g -c src/main.c

server.o : src/server.c src/server.h
	gcc -Wall -g -c src/server.c src/server.h

clean :
	rm -f .*.swp showip chttp
	rm -f *.gch *.o
