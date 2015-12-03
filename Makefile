CC = gcc
all: client server
client: client.c
		$(CC) client.c -o client
server: server.c
		$(CC) server.c -lrt -o server
clean:
		rm client server