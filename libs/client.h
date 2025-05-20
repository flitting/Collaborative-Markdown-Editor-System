#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include "utils.h"
#include "markdown.h"


void disconnect(int c2s,int s2c, FILE* c2s_fp, FILE * s2c_fp,int is_server_ready);
void read_buffer_cmd(char * buffer, pid_t *pid, char *cmd, char *response);
int update_by_logs(document *doc,Commandlogs *logs);