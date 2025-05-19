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
	echo "Running tests..."
	echo "markdown_base_test"
	$(CC) $(CFLAGS) -fsanitize=address test/markdown_base_test.c $(DOCS) -o markdown_test
	./markdown_test
	echo "markdown_version_test"
	$(CC) $(CFLAGS) -fsanitize=address test/markdown_version_test.c $(DOCS) -o markdown_version_test
	./markdown_version_test

	$(MAKE) bold_test

	echo "cleaning up test files..."
	rm -f markdown_test markdown_version_test

bold_test:
	$(CC) $(CFLAGS) -fsanitize=address test/markdown_bold_test.c $(DOCS) -o bold_test
	./bold_test
	rm -f bold_test

stop:
	@pkill -x server || echo "No server instances running."
	@rm -f server.pid
clean:
	@pkill -x server || echo "No server instances running."
	@rm -f *.o *.out server client server.pid FIFO/* bold_test