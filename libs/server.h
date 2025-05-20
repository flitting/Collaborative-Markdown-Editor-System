#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "utils.h"
#include "markdown.h"

typedef struct client_info {
    int s2c_fd;
    int c2s_fd;// used for cleaning
    FILE * c2s_fp;
    char * s2c_path;
    char * c2s_path;
    pthread_mutex_t s2c_mutex;
    struct client_info *last;
    struct client_info *next;
} client_info;

void handle_client_sig(int sig, siginfo_t *info, void * context);
void * client_thread(void * arg);
void *stdin_thread(void *arg);
void *broadcast_thread(void *arg);
void client_disconnect(client_info * client);