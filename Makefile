CC = g++
CFLAGS = -Wall -std=c++11

SERVER=Server.o recv_send.o
CLIENT=TCP_client.o recv_send.o

all: server subscriber recv_send.o

server: Server.o recv_send.o
	$(CC) $(SERVER) -o $@

subscriber: TCP_client.o recv_send.o
	$(CC) $(CLIENT) -o $@

.o: .cpp
	$(CC) $(CFLAGS) -c $@ $<

clean:
	rm -f server subscriber *.o
