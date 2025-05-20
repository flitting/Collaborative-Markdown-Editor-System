#ifndef UTILS_H
#define UTILS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>

#define BUFFER_SIZE_UNIT 64 
#define MAX_LINE_LENGTH 1000
#define MAX_COMMAND_LENGTH 256

char *read_full_message(int fd);
int search_roles(char * client_name);


typedef struct commandlogs{
    uint64_t version;
    char * cmd;
    char * response;
    pid_t client_id;
    struct commandlogs * next;
}Commandlogs;

char* dump_commandlogs(Commandlogs *head, size_t *out_size);


void free_logs(Commandlogs* logs);



#endif // UTILS_H