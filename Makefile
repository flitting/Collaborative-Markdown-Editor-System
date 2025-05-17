# TODO: make sure the rules for server client and markdown filled!
CC := gcc
#CFLAGS := -Wall -Wextra
CFLAGS := -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable
TSAN_FLAGS := -fsanitize=thread
NAME ?= ryan

all: server client

server: 
	$(CC) $(CFLAGS) $(TSAN_FLAGS) source/server.c ./source/utils.c -o server
	./tsan_server 100 & echo $$! > server.pid

client:
	$(CC) $(CFLAGS) $(TSAN_FLAGS) source/client.c ./source/utils.c -o client
	$(eval PID := $(shell cat server.pid))
	./tsan_client $(PID) $(NAME)

markdown.o:

stop:
	@pkill -x server || echo "No server instances running."
	@rm -f server.pid
clean:
	@pkill -x server || echo "No server instances running."
	@rm -f *.o *.out server client server.pid FIFO/*