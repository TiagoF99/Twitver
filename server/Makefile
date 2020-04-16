PORT=55248
FLAGS = -DPORT=$(PORT) -Wall -g -std=gnu99 

server : server.o socket.o
	gcc $(FLAGS) -o $@ $^

%.o : %.c socket.h
	gcc $(FLAGS) -c $<

tidy :
	rm *.o

clean :
	rm *.o server