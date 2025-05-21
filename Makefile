# TODO: make sure the rules for server client and markdown filled!
CC := gcc
#CFLAGS := -Wall -Wextra
CFLAGS := -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable
NAME ?= daniel
DOCS := source/markdown.c libs/extension.h

all: server client

server: 
	$(CC) $(CFLAGS) source/server.c ./source/utils.c $(DOCS)  -o server
#./server 1000 & echo $$! > server.pid
client:
	$(CC) $(CFLAGS) source/client.c ./source/utils.c $(DOCS)  -o client


markdown.o:
	$(CC) $(CFLAGS)  -c source/markdown.c -o markdown.o
	

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

defer_test:
	$(CC) $(CFLAGS) -fsanitize=address test/markdown_def_test.c $(DOCS) -o defer_test
	./defer_test
	rm -f defer_test

stop:
	@pkill -x server || echo "No server instances running."
	@rm -f server.pid
clean:
	@pkill -x server || echo "No server instances running."
	@rm -f *.o *.out server client server.pid bold_test defer_test FIFO_C2S_* FIFO_S2C_* doc.md