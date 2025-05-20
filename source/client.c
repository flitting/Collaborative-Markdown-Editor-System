#include "../libs/client.h"
#include <sys/select.h>


document * doc = NULL;



//TODO: client code that can send instructions to server.
int main(int argc, char *argv[]){
    // error handling
    if(argc!=3){
        printf("Parameter error !\n");
        return 1;
    }
    // ./client <server_pid> <username>
    pid_t server_pid;
    pid_t client_pid = getpid();
    char * username;
    username = argv[2];

    char * endptr;

    server_pid = strtol(argv[1],&endptr,10);
    if(*endptr != '\0'||server_pid<=0){
        printf("Invalid pid %s !\n",argv[1]);
        return 1;
    }
    
    
    sigset_t set;
    struct timespec timeout;
    siginfo_t info;

    sigemptyset(&set);
    sigaddset(&set, SIGRTMIN+1);
    sigprocmask(SIG_BLOCK, & set, NULL);
    
    timeout.tv_sec = 1;
    timeout.tv_nsec =0;
    //send sigrtmin after wait is ready

    if(kill(server_pid, SIGRTMIN) == -1){
        perror("Failed send sigrtmin!\n");
        return 1;
    }
    int sig = sigtimedwait(&set, &info, &timeout);




    if(sig == SIGRTMIN +1 ){
        int space_need = snprintf(NULL,0,"./FIFO/FIFO_C2S_%d",client_pid);
        char * c2s_path = (char *)malloc(space_need+1);
        char * s2c_path = (char *)malloc(space_need+1);
        snprintf(c2s_path,space_need+1, "./FIFO/FIFO_C2S_%d",client_pid);
        snprintf(s2c_path,space_need+1, "./FIFO/FIFO_S2C_%d",client_pid);
        sleep(1);// wait untill the server start fifo;
        int c2s = open(c2s_path, O_WRONLY);
        if (c2s == -1){
            perror("open c2s");
            return 1;
        }
        int s2c = open(s2c_path, O_RDONLY);
        if (s2c == -1){
            perror("open s2c");
            return 1;
        }
        FILE * s2c_fp = fopen(s2c_path,"r");
        FILE * c2s_fp = fopen(c2s_path,"w");


        free(c2s_path);
        free(s2c_path);   

        char * user_message = malloc(strlen(username) + 2); // +1 for '\n', +1 for '\0'
        if (user_message == NULL) {
            perror("malloc");
            disconnect(c2s,s2c,c2s_fp,s2c_fp,0);
            return 1;
        }
        sprintf(user_message, "%s\n", username);
        if(write(c2s,user_message,strlen(user_message)+1)==-1){
            perror("write username null");
            free(user_message);
            disconnect(c2s,s2c,c2s_fp,s2c_fp,0);
            return 1;
        }
        free(user_message);
        char * role_message = read_full_message(s2c);
        if (!role_message) {
            perror("null role_message");
            disconnect(c2s,s2c,c2s_fp,s2c_fp,0);
            return 1;
        }
        int role;
        if (strcmp(role_message,"write")==0){
            role = 2;
            free(role_message);
        }else if (strcmp(role_message,"read")==0){
            role = 1;
            free(role_message);
        }else{
            printf("%s",role_message);
            free(role_message);
            disconnect(c2s,s2c,c2s_fp,s2c_fp,0);
            return 0;
        }

        // receive docuement from server
        char * cmd_buffer = (char*)malloc(MAX_COMMAND_LENGTH);
        fgets(cmd_buffer,MAX_COMMAND_LENGTH,s2c);
        char * endptr;
        size_t version = strtoul(cmd_buffer,endptr,10);
        fgets(cmd_buffer,MAX_COMMAND_LENGTH,s2c);
        size_t doc_length =  strtoul(cmd_buffer,endptr,10);
        char * doc_content_buffer = (char *)malloc(doc_length);
        read(s2c,doc_content_buffer,doc_length);
        doc = markdown_init();
        int result = markdown_insert(doc,0,0,doc_content_buffer);
        free(doc_content_buffer);
        if(result < 0){
            perror("error insert document");
            disconnect(c2s,s2c,c2s_fp,s2c_fp,0);
            return -1;
        }
        doc->version = version-1;
        markdown_increment_version(doc);
        Commandlogs * logs_start = NULL;
        Commandlogs * curr_log = NULL;

        fd_set readfds;
        int maxfd = (s2c > STDIN_FILENO ? s2c : STDIN_FILENO) + 1;

        while (1) {
            FD_ZERO(&readfds);
            FD_SET(STDIN_FILENO, &readfds); //  user
            FD_SET(s2c, &readfds);          // server

            int activity = select(maxfd, &readfds, NULL, NULL, NULL);
            if (activity < 0) {
                perror("select error");
                break;
            }

            // user
            if (FD_ISSET(STDIN_FILENO, &readfds)) {
                char buffer[MAX_COMMAND_LENGTH];
                if (fgets(buffer, MAX_COMMAND_LENGTH, stdin) == NULL) {
                    printf("Error reading input\n");
                    continue;
                }
                if(strcmp(buffer,"DOC?\n")==0){
                    markdown_print(doc,stdout);
                }else if(strcmp(buffer,"LOG?\n")==0){
                    size_t log_size;
                    char * log_content = dump_commandlogs(logs_start,&log_size);
                    printf("%s\n",log_content);
                    free(log_content);
                }else if(strcmp(buffer,"DISCONNECT\n")==0){
                    fputs(buffer,c2s_fp);
                    break;
                }else{
                    fputs(buffer,c2s_fp);
                }

            }

            // server update
            if (FD_ISSET(s2c, &readfds)) {
                char update_buf[MAX_COMMAND_LENGTH];
                if (fgets(update_buf, MAX_COMMAND_LENGTH, s2c_fp) == NULL) {
                    printf("Disconnected by server\n");
                    break;
                }
                uint64_t input_version = version;
                // read version info
                if(strncmp(update_buf,"VERSION",7)){
                    sscanf(update_buf,"VERSION %ld\n",&input_version);
                    while(fgets(update_buf, MAX_COMMAND_LENGTH, s2c_fp) == NULL){
                        if(strcmp(update_buf,"END\n")==0) break;
                        Commandlogs * log = (Commandlogs *)malloc(sizeof(Commandlogs));
                        pid_t cmd_pid;
                        char * cmd_name_buffer = (char*)malloc(10);
                        char * result_buffer = (char*)malloc(30);
                        read_buffer_cmd(update_buf,&cmd_pid,cmd_name_buffer,result_buffer);//
                        log->client_id = cmd_pid;
                        log->cmd = cmd_name_buffer;
                        log->response = result_buffer;
                        log->version = input_version;
                        if(logs_start == NULL){
                            logs_start = log;
                            curr_log = log;
                        }else{
                            curr_log->next = log;
                            curr_log = log;
                        }
                        int result = update_by_logs(doc,curr_log);
                        if(result != 0) break;





                    }

                }
                
            }
        }
        free_logs(logs_start);
        disconnect(c2s,s2c,c2s_fp,s2c_fp,1);
    }else if (sig == -1) {
        perror("sigtimedwait timeout or error");
        return 1;
    }else{
        perror("TIMEOUT");
        return 1;
    }



    return 0;
}


void disconnect(int c2s,int s2c, FILE* c2s_fp, FILE * s2c_fp,int is_server_ready){
    if(is_server_ready){
        fputs("DISCONNECT\n",c2s_fp);
    }
    close(s2c);
    close(c2s);
    fclose(c2s_fp);
    fclose(s2c_fp);

}
void read_buffer_cmd(char * buffer, pid_t *pid, char *cmd, char *response) {
    if (strncmp(buffer, "EDIT ", 5) != 0) {
        fprintf(stderr, "Invalid command format.\n");
        return;
    }

    char *p = buffer + 5;


    while (isspace(*p)) p++; // 
    char *user_start = p;
    while (*p && isdigit(*p)) p++;
    char user_buf[20] = {0};
    strncpy(user_buf, user_start, p - user_start);
    *pid = (pid_t)atoi(user_buf);


    while (isspace(*p)) p++; //
    char *command_start = p;

    char *success_pos = strstr(p, " SUCCESS");
    char *reject_pos = strstr(p, " Reject");

    char *response_pos = NULL;
    int is_success = 0;

    if (success_pos && (!reject_pos || success_pos < reject_pos)) {
        response_pos = success_pos;
        is_success = 1;
    } else if (reject_pos) {
        response_pos = reject_pos;
        is_success = 0;
    } else {
        fprintf(stderr, "Invalid command: missing SUCCESS or Reject.\n");
        return;
    }


    size_t cmd_len = response_pos - command_start;
    strncpy(cmd, command_start, cmd_len);
    cmd[cmd_len] = '\0';


    strcpy(response, response_pos + 1);
}

int update_by_logs(document *doc,Commandlogs *logs){
    Commandlogs * log = logs;
    while(log!=NULL){
        char* buffer = log->cmd;
        char *res = log->response;
        if(strcmp(res,"SUCCESS")!=0) continue;
        uint64_t version = log->version;

        int pos, level, pos_start, pos_end, no_char,result;
        char link[MAX_COMMAND_LENGTH];      // for LINK command
        char content[MAX_COMMAND_LENGTH];  // for INSERT command

        if(!COMP9017){
            // 2017
            if (strncmp(buffer, "NEWLINE", 7) == 0) {
                sscanf(buffer + 8, "%d", &pos);
                result = markdown_newline(doc, version, pos);
                // handle NEWLINE at pos
            } else if (strncmp(buffer, "HEADING", 7) == 0) {
                sscanf(buffer + 8, "%d %d", &level, &pos);
                result = markdown_heading(doc,version,level,pos);
                // handle HEADING of level at pos
            } else if (strncmp(buffer, "BOLD", 4) == 0) {
                sscanf(buffer + 5, "%d %d", &pos_start, &pos_end);
                result = markdown_bold(doc,version,pos_start,pos_end);
                
                // handle BOLD from pos_start to pos_end
            } else if (strncmp(buffer, "ITALIC", 6) == 0) {
                sscanf(buffer + 7, "%d %d", &pos_start, &pos_end);
                result = markdown_italic(doc,version,pos_start,pos_end);
                // handle ITALIC from pos_start to pos_end
            } else if (strncmp(buffer, "BLOCKQUOTE", 10) == 0) {
                sscanf(buffer + 11, "%d", &pos);
                result = markdown_blockquote(doc,version,pos);
                // handle BLOCKQUOTE at pos
            } else if (strncmp(buffer, "ORDERED_LIST", 12) == 0) {
                sscanf(buffer + 13, "%d", &pos);
                result = markdown_ordered_list(doc,version,pos);
                // handle ORDERED_LIST at pos
            } else if (strncmp(buffer, "UNORDERED_LIST", 14) == 0) {
                sscanf(buffer + 15, "%d", &pos);
                result = markdown_unordered_list(doc,version,pos);
                // handle UNORDERED_LIST at pos
            } else if (strncmp(buffer, "CODE", 4) == 0) {
                sscanf(buffer + 5, "%d %d", &pos_start, &pos_end);
                result = markdown_code(doc,version,pos_start,pos_end);
                // handle CODE from pos_start to pos_end
            } else if (strncmp(buffer, "HORIZONTAL_RULE", 15) == 0) {
                sscanf(buffer + 16, "%d", &pos);
                result = markdown_horizontal_rule(doc,version,pos);
                // handle HORIZONTAL_RULE at pos
            } else if (strncmp(buffer, "LINK", 4) == 0) {
                sscanf(buffer + 5, "%d %d %s", &pos_start, &pos_end, link);
                result = markdown_link(doc,version,pos_start,pos_end,link);
                // handle LINK from pos_start to pos_end with link
            } else if (strncmp(buffer, "INSERT", 6) == 0) {
                sscanf(buffer + 7, "%d %[^\n]", &pos, content);
                result = markdown_insert(doc,version,pos,content);
                // handle INSERT at pos with content
            } else if (strncmp(buffer, "DEL", 3) == 0) {
                sscanf(buffer + 4, "%d %d", &pos, &no_char);
                result = markdown_delete(doc,version,pos,no_char);
                // handle DEL from pos deleting no_char characters
            } else {
                continue;
            }
        }else{//COMP9017

            if (strncmp(buffer, "NEWLINE", 7) == 0) {
                sscanf(buffer + 8, "%lu %d", &version, &pos);
                result = markdown_newline(doc, version, pos);
                // handle NEWLINE at pos with version
            } else if (strncmp(buffer, "HEADING", 7) == 0) {
                sscanf(buffer + 8, "%lu %d %d", &version, &level, &pos);
                result = markdown_heading(doc,version,level,pos);
                // handle HEADING of level at pos with version
            } else if (strncmp(buffer, "BOLD", 4) == 0) {
                sscanf(buffer + 5, "%lu %d %d", &version, &pos_start, &pos_end);
                result = markdown_bold(doc,version,pos_start,pos_end);
                // handle BOLD from pos_start to pos_end with version
            } else if (strncmp(buffer, "ITALIC", 6) == 0) {
                sscanf(buffer + 7, "%lu %d %d", &version, &pos_start, &pos_end);
                result = markdown_italic(doc,version,pos_start,pos_end);
                // handle ITALIC from pos_start to pos_end with version
            } else if (strncmp(buffer, "BLOCKQUOTE", 10) == 0) {
                sscanf(buffer + 11, "%lu %d", &version, &pos);
                result = markdown_blockquote(doc,version,pos);
                // handle BLOCKQUOTE at pos with version
            } else if (strncmp(buffer, "ORDERED_LIST", 12) == 0) {
                sscanf(buffer + 13, "%lu %d", &version, &pos);
                result = markdown_ordered_list(doc,version,pos);
                // handle ORDERED_LIST at pos with version
            } else if (strncmp(buffer, "UNORDERED_LIST", 14) == 0) {
                sscanf(buffer + 15, "%lu %d", &version, &pos);
                result = markdown_unordered_list(doc,version,pos);
                // handle UNORDERED_LIST at pos with version
            } else if (strncmp(buffer, "CODE", 4) == 0) {
                sscanf(buffer + 5, "%lu %d %d", &version, &pos_start, &pos_end);
                result = markdown_code(doc,version,pos_start,pos_end);
                // handle CODE from pos_start to pos_end with version
            } else if (strncmp(buffer, "HORIZONTAL_RULE", 15) == 0) {
                sscanf(buffer + 16, "%lu %d", &version, &pos);
                result = markdown_horizontal_rule(doc,version,pos);
                // handle HORIZONTAL_RULE at pos with version
            } else if (strncmp(buffer, "LINK", 4) == 0) {
                sscanf(buffer + 5, "%lu %d %d %s", &version, &pos_start, &pos_end, link);
                result = markdown_link(doc,version,pos_start,pos_end,link);
                // handle LINK from pos_start to pos_end with link and version
            } else if (strncmp(buffer, "INSERT", 6) == 0) {
                sscanf(buffer + 7, "%lu %d %[^\n]", &version, &pos, content);
                result = markdown_insert(doc,version,pos,content);
                // handle INSERT at pos with content and version
            } else if (strncmp(buffer, "DEL", 3) == 0) {
                sscanf(buffer + 4, "%lu %d %d", &version, &pos, &no_char);
                result = markdown_delete(doc,version,pos,no_char);
                // handle DEL from pos deleting no_char characters with version
            } else{
                continue;
            }

        }
        free(link);
        free(content);
        if(result != 0){
            // error command exec;
            return -1;
        }



    }
    return 0;
}