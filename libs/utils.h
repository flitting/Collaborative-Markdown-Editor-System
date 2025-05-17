#ifndef UTILS_H
#define UTILS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define BUFFER_SIZE_UNIT 64 
#define MAX_LINE_LENGTH 1000
#define MAX_COMMAND_LENGTH 256

char *read_full_message(int fd);
int search_roles(char * client_name);

int send_full_document();//todo


int receive_full_document();
#endif // UTILS_H