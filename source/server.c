#include "../libs/server.h"


int pipe_handle2loop[2];


//define global info for doc
u_int64_t doc_version;
u_int64_t doc_length;

int client_num=0;


void handle_client_sig(int sig, siginfo_t *info, void * context){
    if (sig == SIGRTMIN){
        pid_t client_pid = info->si_pid;
        write(pipe_handle2loop[1],&client_pid,sizeof(client_pid));
    }
}

void * client_thread(void * arg){
    pid_t client_pid = *(pid_t *) arg;
    free(arg);
    


    int space_need = snprintf(NULL,0,"./FIFO/FIFO_C2S_%d",client_pid);
    char * c2s_path = (char *)malloc(space_need+1);
    char * s2c_path = (char *)malloc(space_need+1);
    snprintf(c2s_path,space_need+1, "./FIFO/FIFO_C2S_%d",client_pid);
    snprintf(s2c_path,space_need+1, "./FIFO/FIFO_S2C_%d",client_pid);

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
    client_num ++;

    int c2s = open(c2s_path, O_RDONLY);
    int s2c = open(s2c_path, O_WRONLY);
    if (c2s < 0) {
    perror("open c2s");
    //disconnect
    client_num --;
    return NULL;
}
    char* username_message = read_full_message(c2s);

    int role = search_roles(username_message);
    free(username_message);
    if(role==-1){
        //disconnect
        client_num --;
        return NULL;
    }else if (role == 0){
        // role no found
        const char * reject_role_message = "Reject UNAUTHORISED\n";
        write(s2c,reject_role_message,strlen(reject_role_message)+1);
        sleep(1);
        remove(c2s_path);
        remove(s2c_path);
        free(c2s_path);
        free(s2c_path);
        //disconnect
        client_num --;
        return NULL;   
    }else{
        if (role==1){
            const char * role_read_message = "read\n";
            write(s2c,role_read_message,strlen(role_read_message)+1);
        }else{
            const char * role_write_message = "write\n";
            write(s2c,role_write_message,strlen(role_write_message)+1);
        }
        //send doc length
        //send full document contents
        //edit-command loop
    }


    
    // clear path memory
    free(c2s_path);
    free(s2c_path);
    //disconnect
    client_num --;
    return NULL;
}


// TODO: server code that manages the document and handles client instructions
int main(int argc, char *argv[]){
    // read interval
    // error handling.
    if (argc!= 2){
        printf("Parameter error !\n");
        return 1;
    }
    char * endptr;
    long interval = strtol(argv[1],&endptr,10);
    if(*endptr != '\0' || interval<=0 ){
        printf("Invalid number %s !\n",argv[1]);
        return 1;
    }
    // print pid
    pid_t pid = getpid();
    printf("Server PID: %d\n", pid);

    if (pipe(pipe_handle2loop) == -1) {
        perror("pipe");
        return 1;
    }


    struct sigaction sa;
    sa.sa_sigaction = handle_client_sig;
    sa.sa_flags = SA_SIGINFO;EXIT_FAILURE;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGRTMIN, &sa, NULL) == -1){
        perror("sigaction");
        return 1;
    }


    while(1){
        pid_t client_pid;
        ssize_t byte_num = read(pipe_handle2loop[0],&client_pid,sizeof(client_pid));
        if( byte_num == sizeof(client_pid)){
            pthread_t thread_id;
            pid_t * arg = malloc(sizeof(pid_t));
            *arg = client_pid;
            // create pthread and error handle.
            if(pthread_create(&thread_id,NULL,client_thread, arg) == 0){
                pthread_detach(thread_id);
            }else{
                perror("pthread create error");
                free(arg);
            }

        }
    }



    



    return 0;
}