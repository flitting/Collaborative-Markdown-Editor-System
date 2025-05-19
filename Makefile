# TODO: make sure the rules for server client and markdown filled!
CC := gcc
#CFLAGS := -Wall -Wextra
CFLAGS := -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable
TSAN_FLAGS := -fsanitize=thread
NAME ?= ryan
DOCS := source/document.c source/markdown.c libs/extension.h

all: server client

server: 
	$(CC) $(CFLAGS) $(TSAN_FLAGS) source/server.c ./source/utils.c -o server
	./tsan_server 100 & echo $$! > server.pid

client:
	$(CC) $(CFLAGS) $(TSAN_FLAGS) source/client.c ./source/utils.c -o client
	$(eval PID := $(shell cat server.pid))
	./tsan_client $(PID) $(NAME)

tests:
	$(CC) $(CFLAGS) -fsanitize=address test/markdown_base_test.c $(DOCS) -o markdown_test
	./markdown_test

stop:
	@pkill -x server || echo "No server instances running."
	@rm -f server.pid
clean:
	@pkill -x server || echo "No server instances running."
	@rm -f *.o *.out server client server.pid FIFO/* markdown_test