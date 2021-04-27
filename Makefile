SERVER_SOURCE_CODE=server.c
CLIENT_SOURCE_CODE=tcp_client.c
SERVER_EXE=server
CLIENT_EXE=subscriber
CFLAGS=-Wall -Wextra -g -o
CC=gcc

server:
	$(CC) $(SERVER_SOURCE_CODE) $(CFLAGS) $(SERVER_EXE)

subscriber:
	$(CC) $(CLIENT_SOURCE_CODE) $(CFLAGS) $(CLIENT_EXE)

clean:
	rm -rf $(SERVER_EXE) $(CLIENT_EXE)
