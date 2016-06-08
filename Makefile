server : server.o
	gcc -Wall -o server server.o

server.o : server.c
	gcc -Wall -g -c server.c

clean :
	rm -f .*.swp showip server
	rm -f *.gch *.o
