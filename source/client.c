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
        int space_need = snprintf(NULL,0,"./FIFO_C2S_%d",client_pid);
        char * c2s_path = (char *)malloc(space_need+1);
        char * s2c_path = (char *)malloc(space_need+1);
        snprintf(c2s_path,space_need+1, "./FIFO_C2S_%d",client_pid);
        snprintf(s2c_path,space_need+1, "./FIFO_S2C_%d",client_pid);
        sleep(1);// wait untill the server start fifo;
        FILE * c2s_fp = fopen(c2s_path,"w");
        FILE * s2c_fp = fopen(s2c_path,"r");
        


        free(c2s_path);
        free(s2c_path);   

        char * user_message = malloc(strlen(username) + 2); // +1 for '\n', +1 for '\0'
        if (user_message == NULL) {
            perror("malloc");
            disconnect(c2s_fp,s2c_fp,0);
            return 1;
        }
        sprintf(user_message, "%s\n", username);
        if(fputs(user_message, c2s_fp)==-1){
            perror("write username null");
            free(user_message);
            disconnect(c2s_fp,s2c_fp,0);
            return 1;
        }
        fflush(c2s_fp);
        free(user_message);
        char role_message[MAX_COMMAND_LENGTH];
        if (fgets(role_message, sizeof(role_message), s2c_fp) == NULL) {
            perror("fgets failed or connection closed");
            disconnect(c2s_fp, s2c_fp, 0);
            return 1;
        }
        //int role;
        if (strcmp(role_message,"write\n")==0){
            ;
        //    role = 2;
        }else if (strcmp(role_message,"read\n")==0){

            ;
        //    role = 1;
        }else{
            printf("Current Role: %s\n",role_message);
            disconnect(c2s_fp,s2c_fp,0);
            return 0;
        }
        printf("Current Role: %s\n",role_message);

        // receive docuement from server
        char * cmd_buffer = (char*)malloc(MAX_COMMAND_LENGTH);
        fgets(cmd_buffer,MAX_COMMAND_LENGTH,s2c_fp);
        char * endptr;
        size_t version = strtoul(cmd_buffer,&endptr,10);
        fgets(cmd_buffer,MAX_COMMAND_LENGTH,s2c_fp);
        size_t doc_length =  strtoul(cmd_buffer,&endptr,10);
        char * doc_content_buffer = (char *)malloc(doc_length+1);
        fread(doc_content_buffer, 1, doc_length, s2c_fp);
        doc_content_buffer[doc_length] = '\0'; 
        doc = markdown_init();
        if(strcmp(doc_content_buffer,"")!=0 && strcmp(doc_content_buffer,"\n")!=0 ){
        int result = markdown_insert(doc,0,0,doc_content_buffer);
            if(result < 0){
                printf("error insert document:\n%s",doc_content_buffer);
                free(doc_content_buffer);
                disconnect(c2s_fp,s2c_fp,0);
                return -1;
            }
        }
        free(doc_content_buffer);
        doc->version = version-1;
        markdown_increment_version(doc);
        Commandlogs * logs_start = NULL;
        Commandlogs * curr_log = NULL;



        while (1) {
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(STDIN_FILENO, &rfds);
            FD_SET(fileno(s2c_fp), &rfds);
            int maxfd = (STDIN_FILENO > fileno(s2c_fp) ? STDIN_FILENO : fileno(s2c_fp)) + 1;

            struct timeval tv = { .tv_sec = 0, .tv_usec = 500000 };

            int ready = select(maxfd, &rfds, NULL, NULL, &tv);
            if (ready == -1) {
                perror("select");
                break;
            }
            uint64_t last_version=0;


            // Handle user input
            if (FD_ISSET(STDIN_FILENO, &rfds)) {
                if (fgets(cmd_buffer, MAX_COMMAND_LENGTH, stdin) != NULL) {

                    if(strcmp(cmd_buffer,"DOC?\n")==0){
                        markdown_print(doc,stdout);
                    }else if(strcmp(cmd_buffer,"LOG?\n")==0){
                        size_t log_size;
                        char * log_content = dump_commandlogs(logs_start,&log_size);
                        if(log_content == NULL);
                        else printf("%s\n",log_content);
                        free(log_content);
                    }else if(strcmp(cmd_buffer,"DISCONNECT\n")==0){
                        fputs(cmd_buffer,c2s_fp);
                        fflush(c2s_fp);
                        break; 
                    }else if(strcmp(cmd_buffer,"PERM?\n")==0){
                        fputs(cmd_buffer,c2s_fp);
                        fflush(c2s_fp);
                        if (fgets(cmd_buffer, MAX_COMMAND_LENGTH, s2c_fp) != NULL){
                            printf("%s",cmd_buffer);
                        }else{
                            printf("Server disconnected.\n");
                            break;
                        }
                    }else{
                        fputs(cmd_buffer,c2s_fp);
                        fflush(c2s_fp);
                    }
                }else {
                    // Server closed connection or error
                    printf("Server disconnected.\n");
                    break;
                }
            }

            // Handle server messages
            if (FD_ISSET(fileno(s2c_fp), &rfds)) {
                if (fgets(cmd_buffer, MAX_COMMAND_LENGTH, s2c_fp) != NULL) {
                    if (strncmp(cmd_buffer, "VERSION", 7) == 0) {
                        uint64_t input_version;
                        sscanf(cmd_buffer, "VERSION %lu\n", &input_version);

                        Commandlogs * tag_log = (Commandlogs*)malloc(sizeof(Commandlogs));
                        tag_log->cmd=NULL;
                        tag_log->response = NULL;
                        tag_log->next = NULL;
                        tag_log->version = input_version;  // Add this line
                        tag_log->client_id = 0;  

                        if (logs_start == NULL) {
                            logs_start = tag_log;
                            curr_log = tag_log;
                        } else {
                            curr_log->next = tag_log;
                            curr_log = tag_log;
                        }


                        while (fgets(cmd_buffer, MAX_COMMAND_LENGTH, s2c_fp) != NULL) {
                            if(strcmp(cmd_buffer, "END\n") == 0) {
                                if(input_version == last_version+1){
                                    markdown_increment_version(doc);
                                }
                                last_version = input_version;
                                break;
                            }

                            Commandlogs *log = (Commandlogs *)malloc(sizeof(Commandlogs));
                            pid_t cmd_pid;
                            char *cmd_name_buffer = (char*)malloc(MAX_COMMAND_LENGTH);
                            char *result_buffer = (char*)malloc(30);
                            int is_success = 0;

                            read_buffer_cmd(cmd_buffer, &cmd_pid, cmd_name_buffer, result_buffer, &is_success);
                            if (is_success == -1) {
                                printf("Invalid response\n");
                                break;
                            }

                            log->client_id = cmd_pid;
                            log->cmd = cmd_name_buffer;
                            log->response = result_buffer;
                            log->version = input_version;
                            log->next = NULL;

                            if (logs_start == NULL) {
                                logs_start = log;
                                curr_log = log;
                            } else {
                                curr_log->next = log;
                                curr_log = log;
                            }
                            int result = update_by_logs(doc, curr_log);
                            if (result != 0) break;
                        }
                    } else {
                        // Optional: handle unexpected lines from server
                        printf("Unknown server message: %s\n", cmd_buffer);
                    }
                } else {
                    // Server closed connection or error
                    printf("Server disconnected.\n");
                    break;
                }
            }
        }
        free_logs(logs_start);
        disconnect(c2s_fp,s2c_fp,1);
    }else if (sig == -1) {
        perror("sigtimedwait timeout or error");
        return 1;
    }else{
        perror("TIMEOUT");
        return 1;
    }



    return 0;
}


void disconnect(FILE* c2s_fp, FILE * s2c_fp,int is_server_ready){
    if(is_server_ready){
        fputs("DISCONNECT\n",c2s_fp);
    }
    fclose(c2s_fp);
    fclose(s2c_fp);
    sleep(1);

}
void read_buffer_cmd(char * buffer, pid_t *pid, char *cmd, char *response,int * is_success) {
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

    if (success_pos && (!reject_pos || success_pos < reject_pos)) {
        response_pos = success_pos;
        *is_success = 1;
    } else if (reject_pos) {
        response_pos = reject_pos;
        *is_success = 0;
    } else {
        *is_success = -1;
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
        if(strcmp(res,"SUCCESS\n")!=0){
            log = log->next;
            continue;
        }
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
        if(result != 0){
            break;
        }


        log = log->next;
    }
    return 0;
}