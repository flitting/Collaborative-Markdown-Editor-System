# TODO: make sure the rules for server client and markdown filled!
CC := gcc
#CFLAGS := -Wall -Wextra
CFLAGS := -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable
NAME ?= ryan

all: server client

server: 
	$(CC) $(CFLAGS) source/server.c ./source/utils.c -o server
	./server 100 & echo $$! > server.pid
client:
	$(CC) $(CFLAGS) source/client.c ./source/utils.c -o client
	$(eval PID := $(shell cat server.pid))
	./client $(PID) $(NAME)

markdown.o:

stop:
	@pkill -x server || echo "No server instances running."
	@rm -f server.pid
clean:
	@pkill -x server || echo "No server instances running."
	@rm -f *.o *.out server client server.pid FIFO/*