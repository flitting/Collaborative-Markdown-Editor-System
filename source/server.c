#include "../libs/server.h"


int pipe_handle2loop[2];


//define global info for doc
u_int64_t doc_version;
u_int64_t doc_length;




client_info *client_list = NULL;

int client_num=0;
document * doc = NULL;
int is_doc_changed = 0;

Commandlogs * history_log_start = NULL;// all logs
Commandlogs * broadcast_log = NULL;// current broadcast log
Commandlogs * buffer_log_start = NULL;// un update log
Commandlogs * buffer_curr = NULL;// end of buffer_logs
Commandlogs * histroy_curr = NULL;
pthread_mutex_t client_list_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t doc_mutex = PTHREAD_MUTEX_INITIALIZER;



void handle_client_sig(int sig, siginfo_t *info, void * context){
    if (sig == SIGRTMIN){
        pid_t client_pid = info->si_pid;
        write(pipe_handle2loop[1],&client_pid,sizeof(client_pid));
    }
}

void * client_thread(void * arg){
    pid_t client_pid = *(pid_t *) arg;
    free(arg);
    


    int space_need = snprintf(NULL,0,"./FIFO_C2S_%d",client_pid);
    char * c2s_path = (char *)malloc(space_need+1);
    char * s2c_path = (char *)malloc(space_need+1);
    snprintf(c2s_path,space_need+1, "./FIFO_C2S_%d",client_pid);
    snprintf(s2c_path,space_need+1, "./FIFO_S2C_%d",client_pid);

    remove(c2s_path);
    remove(s2c_path);
    if (mkfifo(c2s_path,0666)==-1){
        perror("mkfifo c2s error");
        return NULL;
    }
    if (mkfifo(s2c_path,0666)==-1){
        perror("mkfifo s2c error");
        return NULL;
    }

    
    // send the sigrtmin +1 after fifo created.
    kill(client_pid, SIGRTMIN + 1);

    FILE * c2s_fp = fopen(c2s_path,"r");
    FILE * s2c_fp = fopen(s2c_path,"w");



    // init client_info
    client_info *new_client = (client_info *)malloc(sizeof(client_info));
    new_client->client_id = client_pid;
    new_client->c2s_fp = c2s_fp;
    new_client->s2c_fp = s2c_fp;
    new_client->s2c_path = s2c_path;
    new_client->c2s_path = c2s_path;
    client_num ++;

    pthread_mutex_init(&new_client->s2c_mutex, NULL);
    pthread_mutex_lock(&client_list_mutex);
    new_client->next = client_list;
    new_client->last = NULL;
    if (client_list != NULL) {
        client_list->last = new_client;
    }
    client_list = new_client;

    pthread_mutex_unlock(&client_list_mutex);

    char username_message[MAX_COMMAND_LENGTH];
    fgets(username_message, sizeof(username_message), c2s_fp);
    username_message[strcspn(username_message, "\n")] = '\0';
    printf("Client %s registered\n",username_message);
    int role = search_roles(username_message);
    if(role==-1){
        //disconnect
        // delete files
        
        client_disconnect(new_client);
        return NULL;
    }else if (role == 0){
        // role no found
        const char * reject_role_message = "Reject UNAUTHORISED\n";
        pthread_mutex_lock(&new_client->s2c_mutex);
        fputs(reject_role_message,s2c_fp);
        fflush(new_client->s2c_fp);
        pthread_mutex_unlock(&new_client->s2c_mutex);
        sleep(1);

        //disconnect
        client_disconnect(new_client);
        return NULL;   
    }else{
        if (role==1){
            const char * role_read_message = "read\n";
            pthread_mutex_lock(&new_client->s2c_mutex);
            fputs(role_read_message,s2c_fp);
            fflush(s2c_fp);
            pthread_mutex_unlock(&new_client->s2c_mutex);
        }else{
            const char * role_write_message = "write\n";
            pthread_mutex_lock(&new_client->s2c_mutex);
            fputs(role_write_message,s2c_fp);
            fflush(s2c_fp);
            pthread_mutex_unlock(&new_client->s2c_mutex);
        }
        // send version
        char buffer[128];
        snprintf(buffer,128,"%ld\n",doc->version);
        pthread_mutex_lock(&new_client->s2c_mutex);
        fwrite(buffer,1,strlen(buffer),s2c_fp);
        fflush(s2c_fp);
        pthread_mutex_unlock(&new_client->s2c_mutex);

        snprintf(buffer,128,"%ld\n",strlen(doc->flatten_cache));
        pthread_mutex_lock(&new_client->s2c_mutex);
        fwrite(buffer,1,strlen(buffer),s2c_fp);
        fflush(s2c_fp);
        pthread_mutex_unlock(&new_client->s2c_mutex);
        //send doc length \n
        //send full document contents
        pthread_mutex_lock(&new_client->s2c_mutex);
        fwrite(doc->flatten_cache,1,strlen(doc->flatten_cache),s2c_fp);
        fflush(s2c_fp);
        pthread_mutex_unlock(&new_client->s2c_mutex);
        //edit-command loop
        //interact with command 

        // check command type
        // check command permission
        // run command
        // send sync to all client in interval    

        printf("%s connected\n",username_message);

        char cmd_buffer[MAX_COMMAND_LENGTH];
        int pos, level, pos_start, pos_end, no_char;
        char link[MAX_COMMAND_LENGTH];      // for LINK command
        char content[MAX_COMMAND_LENGTH];  // for INSERT command
        
        const char * invalid_position = "Reject INVALID_POSITION\n";
        const char * deleted_position = "Reject DELETED_POSITION\n";
        const char * outdated_version = "Reject OUTDATED_VERSION\n";
        const char * write_pms = "Reject UNAUTHORISED %s write read\n";
        const char * other_error = "INTERNAL_ERR\n";

        
        while (fgets(cmd_buffer, sizeof(cmd_buffer), c2s_fp) != NULL) {

            if(strcmp(cmd_buffer,"DISCONNECT\n")==0){
                break;
            }else if (strcmp(cmd_buffer,"PERM?\n")==0){
                if (role==1){
                    const char * role_read_message = "read\n";
                    pthread_mutex_lock(&new_client->s2c_mutex);
                    fputs(role_read_message,s2c_fp);
                    fflush(s2c_fp);
                    pthread_mutex_unlock(&new_client->s2c_mutex);
                }else{
                    const char * role_write_message = "write\n";
                    pthread_mutex_lock(&new_client->s2c_mutex);
                    fputs(role_write_message,s2c_fp);
                    fflush(s2c_fp);
                    pthread_mutex_unlock(&new_client->s2c_mutex);
                }
            
            }else{

                Commandlogs * log = (Commandlogs *)malloc(sizeof(Commandlogs));
                if(log == NULL){

                    client_disconnect(new_client);
                    return NULL;
                }
                

                log->client_id = client_pid;
                char * cmd = (char *)malloc(MAX_COMMAND_LENGTH);
                strcpy(cmd,cmd_buffer);
                cmd[strcspn(cmd, "\n")] = '\0'; // remove \n for cmd;
                log->cmd = cmd;
                char * response = (char *)malloc(MAX_COMMAND_LENGTH);
                if (cmd==NULL || response == NULL){
                    if(cmd!=NULL) free(cmd);
                    if(response!=NULL) free(response);
                    free(log);
                    //disconnect
                    client_disconnect(new_client);
                    return NULL;
                }

                int result;
                char * cmd_name = "";
                if (role == 1){
                    result = -4;
                }else if(!COMP9017){
                    log->version = doc->version;
                    uint64_t version = doc->version;
                    

                    if (strncmp(cmd_buffer, "NEWLINE", 7) == 0) {
                        sscanf(cmd_buffer + 8, "%d", &pos);

                        pthread_mutex_lock(&doc_mutex);
                        result = markdown_newline(doc, version, pos);
                        pthread_mutex_unlock(&doc_mutex);
                        cmd_name = "NEWLINE";

                        // handle NEWLINE at pos
                    } else if (strncmp(cmd_buffer, "HEADING", 7) == 0) {
                        sscanf(cmd_buffer + 8, "%d %d", &level, &pos);
                        result = markdown_heading(doc,version,level,pos);
                        // handle HEADING of level at pos
                    } else if (strncmp(cmd_buffer, "BOLD", 4) == 0) {
                        sscanf(cmd_buffer + 5, "%d %d", &pos_start, &pos_end);
                        result = markdown_bold(doc,version,pos_start,pos_end);
                        
                        // handle BOLD from pos_start to pos_end
                    } else if (strncmp(cmd_buffer, "ITALIC", 6) == 0) {
                        sscanf(cmd_buffer + 7, "%d %d", &pos_start, &pos_end);
                        result = markdown_italic(doc,version,pos_start,pos_end);
                        // handle ITALIC from pos_start to pos_end
                    } else if (strncmp(cmd_buffer, "BLOCKQUOTE", 10) == 0) {
                        sscanf(cmd_buffer + 11, "%d", &pos);
                        result = markdown_blockquote(doc,version,pos);
                        // handle BLOCKQUOTE at pos
                    } else if (strncmp(cmd_buffer, "ORDERED_LIST", 12) == 0) {
                        sscanf(cmd_buffer + 13, "%d", &pos);
                        result = markdown_ordered_list(doc,version,pos);
                        // handle ORDERED_LIST at pos
                    } else if (strncmp(cmd_buffer, "UNORDERED_LIST", 14) == 0) {
                        sscanf(cmd_buffer + 15, "%d", &pos);
                        result = markdown_unordered_list(doc,version,pos);
                        // handle UNORDERED_LIST at pos
                    } else if (strncmp(cmd_buffer, "CODE", 4) == 0) {
                        sscanf(cmd_buffer + 5, "%d %d", &pos_start, &pos_end);
                        result = markdown_code(doc,version,pos_start,pos_end);
                        // handle CODE from pos_start to pos_end
                    } else if (strncmp(cmd_buffer, "HORIZONTAL_RULE", 15) == 0) {
                        sscanf(cmd_buffer + 16, "%d", &pos);
                        result = markdown_horizontal_rule(doc,version,pos);
                        // handle HORIZONTAL_RULE at pos
                    } else if (strncmp(cmd_buffer, "LINK", 4) == 0) {
                        sscanf(cmd_buffer + 5, "%d %d %s", &pos_start, &pos_end, link);
                        result = markdown_link(doc,version,pos_start,pos_end,link);
                        // handle LINK from pos_start to pos_end with link
                    } else if (strncmp(cmd_buffer, "INSERT", 6) == 0) {
                        sscanf(cmd_buffer + 7, "%d %[^\n]", &pos, content);
                        result = markdown_insert(doc,version,pos,content);
                        // handle INSERT at pos with content
                    } else if (strncmp(cmd_buffer, "DEL", 3) == 0) {
                        sscanf(cmd_buffer + 4, "%d %d", &pos, &no_char);
                        result = markdown_delete(doc,version,pos,no_char);
                        // handle DEL from pos deleting no_char characters
                    }  else {
                        // unknown command

                        result = -5;
                        continue;

                    }
                }else{
                    uint64_t version;
                    if (strncmp(cmd_buffer, "NEWLINE", 7) == 0) {
                        sscanf(cmd_buffer + 8, "%lu %d", &version, &pos);
                        result = markdown_newline(doc, version, pos);
                        // handle NEWLINE at pos with version
                    } else if (strncmp(cmd_buffer, "HEADING", 7) == 0) {
                        sscanf(cmd_buffer + 8, "%lu %d %d", &version, &level, &pos);
                        result = markdown_heading(doc,version,level,pos);
                        // handle HEADING of level at pos with version
                    } else if (strncmp(cmd_buffer, "BOLD", 4) == 0) {
                        sscanf(cmd_buffer + 5, "%lu %d %d", &version, &pos_start, &pos_end);
                        result = markdown_bold(doc,version,pos_start,pos_end);
                        // handle BOLD from pos_start to pos_end with version
                    } else if (strncmp(cmd_buffer, "ITALIC", 6) == 0) {
                        sscanf(cmd_buffer + 7, "%lu %d %d", &version, &pos_start, &pos_end);
                        result = markdown_italic(doc,version,pos_start,pos_end);
                        // handle ITALIC from pos_start to pos_end with version
                    } else if (strncmp(cmd_buffer, "BLOCKQUOTE", 10) == 0) {
                        sscanf(cmd_buffer + 11, "%lu %d", &version, &pos);
                        result = markdown_blockquote(doc,version,pos);
                        // handle BLOCKQUOTE at pos with version
                    } else if (strncmp(cmd_buffer, "ORDERED_LIST", 12) == 0) {
                        sscanf(cmd_buffer + 13, "%lu %d", &version, &pos);
                        result = markdown_ordered_list(doc,version,pos);
                        // handle ORDERED_LIST at pos with version
                    } else if (strncmp(cmd_buffer, "UNORDERED_LIST", 14) == 0) {
                        sscanf(cmd_buffer + 15, "%lu %d", &version, &pos);
                        result = markdown_unordered_list(doc,version,pos);
                        // handle UNORDERED_LIST at pos with version
                    } else if (strncmp(cmd_buffer, "CODE", 4) == 0) {
                        sscanf(cmd_buffer + 5, "%lu %d %d", &version, &pos_start, &pos_end);
                        result = markdown_code(doc,version,pos_start,pos_end);
                        // handle CODE from pos_start to pos_end with version
                    } else if (strncmp(cmd_buffer, "HORIZONTAL_RULE", 15) == 0) {
                        sscanf(cmd_buffer + 16, "%lu %d", &version, &pos);
                        result = markdown_horizontal_rule(doc,version,pos);
                        // handle HORIZONTAL_RULE at pos with version
                    } else if (strncmp(cmd_buffer, "LINK", 4) == 0) {
                        sscanf(cmd_buffer + 5, "%lu %d %d %s", &version, &pos_start, &pos_end, link);
                        result = markdown_link(doc,version,pos_start,pos_end,link);
                        // handle LINK from pos_start to pos_end with link and version
                    } else if (strncmp(cmd_buffer, "INSERT", 6) == 0) {
                        sscanf(cmd_buffer + 7, "%lu %d %[^\n]", &version, &pos, content);
                        result = markdown_insert(doc,version,pos,content);

                        // handle INSERT at pos with content and version
                    } else if (strncmp(cmd_buffer, "DEL", 3) == 0) {
                        sscanf(cmd_buffer + 4, "%lu %d %d", &version, &pos, &no_char);
                        result = markdown_delete(doc,version,pos,no_char);
                        // handle DEL from pos deleting no_char characters with version
                    }else{
                        // unknown command
                        result = -5;
                        continue;
                    }
                    log->version = version;
                }
                // -1 wrong version
                // -2 deleted pos
                // -3 wrong range
                // -4 wrong permission  as no user data in doc, it can't be used
                // -5 other reasons
                // -6 unrecovered error
                if(result == -4)snprintf(response,MAX_COMMAND_LENGTH,write_pms,cmd_name);
                else if(result == -1) strcpy(response,outdated_version);
                else if(result == -2) strcpy(response,deleted_position);
                else if(result == -3) strcpy(response,invalid_position);
                else if(result == 0) strcpy(response,"SUCCESS\n");
                else strcpy(response,other_error); // should be fixed.
                log->response = response;
                pthread_mutex_lock(&log_mutex);
                log->next = NULL;
                buffer_curr->next = log;
                buffer_curr = log;
                pthread_mutex_unlock(&log_mutex);
                // send response in broadcasting
                if(result!= -5||result != -6){
                    is_doc_changed = 1;
                }
                
                

            }
        }

    }

    client_disconnect(new_client);
    return NULL;
}


// TODO: server code that manages the document and handles client instructions
int main(int argc, char *argv[]){
    // read interval
    // error handling.
    if (argc!= 2){
        printf("Parameter error !\n");
        pthread_mutex_destroy(&doc_mutex);
        return 1;
    }
    char * endptr;
    long interval = strtol(argv[1],&endptr,10);
    if(*endptr != '\0' || interval<=0 ){
        printf("Invalid number %s !\n",argv[1]);
        pthread_mutex_destroy(&doc_mutex);
        return 1;
    }
    // print pid
    pid_t pid = getpid();
    printf("Server PID: %d\n", pid);

    FILE *fp = fopen("server.pid", "w");
    if (fp == NULL) {
        perror("Failed to write PID to file");
        return 1;
    }
    fprintf(fp, "%d\n", pid);
    fclose(fp);


    if (pipe(pipe_handle2loop) == -1) {
        perror("pipe");
        pthread_mutex_destroy(&doc_mutex);
        return 1;
    }


    struct sigaction sa;
    sa.sa_sigaction = handle_client_sig;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGRTMIN, &sa, NULL) == -1){
        perror("sigaction");
        pthread_mutex_destroy(&doc_mutex);
        return 1;
    }
    // init
    doc = markdown_init();
    int quit_flag = 0;

    pthread_t stdin_tid;
    pthread_create(&stdin_tid, NULL, stdin_thread, &quit_flag);
    pthread_t broadcast_tid;
    pthread_create(&broadcast_tid, NULL, broadcast_thread, &interval);

    history_log_start = (Commandlogs *)malloc(sizeof(Commandlogs));
    history_log_start->next = NULL;
    histroy_curr = history_log_start;
    buffer_log_start = (Commandlogs *)malloc(sizeof(Commandlogs));
    buffer_log_start->next = NULL;
    buffer_curr = buffer_log_start;


    while(!quit_flag){
        fd_set readfds;
        struct timeval tv;
        FD_ZERO(&readfds);
        FD_SET(pipe_handle2loop[0], &readfds);

        // set a timeout of 1 second
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int ret = select(pipe_handle2loop[0] + 1, &readfds, NULL, NULL, &tv);
        if (ret > 0 && FD_ISSET(pipe_handle2loop[0], &readfds)) {
            pid_t client_pid;
            ssize_t byte_num = read(pipe_handle2loop[0], &client_pid, sizeof(client_pid));
            if (byte_num == sizeof(client_pid)) {
                pthread_t thread_id;
                pid_t *arg = malloc(sizeof(pid_t));
                *arg = client_pid;
                if (pthread_create(&thread_id, NULL, client_thread, arg) == 0) {
                    pthread_detach(thread_id);
                } else {
                    perror("pthread create error");
                    free(arg);
                }
            }
        }
    }



    printf("Shutting down server\n");
    markdown_free(doc);
    pthread_mutex_destroy(&doc_mutex);
    free_logs(history_log_start);
    return 0;
}


void *stdin_thread(void *arg) {
    char buffer[MAX_COMMAND_LENGTH];

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        if (strcmp(buffer, "DOC?\n") == 0) {
            if(is_doc_changed){
                markdown_print(doc,stdout);
            }else{
                printf("%s\n",doc->flatten_cache);
            }
            
        } else if (strcmp(buffer, "QUIT\n") == 0) {
            if(client_num == 0 ){
                *((int*)arg) = 1;
                break;
            }else{
                printf("QUIT rejected, %d clients still connected.",client_num);
            }
        } else if (strcmp(buffer, "LOG?\n") == 0){
            size_t log_size;
            char * log_content = dump_commandlogs(history_log_start->next,&log_size);
            if(log_content!=NULL)printf("%s",log_content);
            free(log_content);
        } else {
            ;
        }
    }

    return NULL;
}

void sleep_ms(long milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

void *broadcast_thread(void *arg) {
    long interval = *((long *)arg);

    while (1) {
        sleep_ms(interval);
        // freeze the logs for the whole code.
        pthread_mutex_lock(&log_mutex);

        //increment
        if(is_doc_changed){
            markdown_increment_version(doc);
            is_doc_changed = 0;
        }

        Commandlogs * version_tag_log = (Commandlogs*) malloc(sizeof(Commandlogs));
        version_tag_log->version = doc->version;
        version_tag_log->response = NULL;
        version_tag_log->cmd = NULL;
        version_tag_log->next = buffer_log_start->next;
        buffer_log_start->next = NULL;

        //summary the buffer_log and broadcast
        size_t dump_size;
        char * dump_buffer = dump_commandlogs(version_tag_log,&dump_size);
        //broadcast.
        pthread_mutex_lock(&client_list_mutex);
        client_info * curr_client = client_list;
        while(curr_client!=NULL){
            pthread_mutex_lock(&curr_client->s2c_mutex);
            if(dump_buffer)
            {
                fwrite(dump_buffer,1,dump_size,curr_client->s2c_fp);
            }
            fflush(curr_client->s2c_fp);
            pthread_mutex_unlock(&curr_client->s2c_mutex);

            curr_client = curr_client->next;
        }
        pthread_mutex_unlock(&client_list_mutex);
        //update logs
        
        histroy_curr->next = version_tag_log;
        histroy_curr = buffer_curr;
        buffer_curr = buffer_log_start;
        pthread_mutex_unlock(&log_mutex);
        free(dump_buffer);
    }

    return NULL;
}


void client_disconnect(client_info * client){

    if(client->last == NULL || client->next == NULL){
        // last client
        client_list = NULL;    
    }else{
        if(client->last!=NULL){
            client->last->next = client->next;
        }
        if(client->next!=NULL){
            client->next->last = client->last;
        }
    }
    printf("Client %d disconnect\n",client->client_id);
    pthread_mutex_destroy(&client->s2c_mutex);
        remove(client->c2s_path);
        remove(client->s2c_path);
        free(client->c2s_path);
        free(client->s2c_path);

        fclose(client->c2s_fp);
        fclose(client->s2c_fp);
        free(client);
        client_num--;
}