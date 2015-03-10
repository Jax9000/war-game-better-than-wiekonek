client: client.c
	gcc client.c -lncurses -o client

server: server.c
	gcc server.c -lm -o server
