#include "../libs/client.h"



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
        free(c2s_path);
        free(s2c_path);   
        char * user_message = malloc(strlen(username) + 2); // +1 for '\n', +1 for '\0'
        if (user_message == NULL) {
            perror("malloc");
            return 1;
        }
        sprintf(user_message, "%s\n", username);
        if(write(c2s,user_message,strlen(user_message)+1)==-1){
            perror("write username null");
            return 1;
        }
        free(user_message);
        char * role_message = read_full_message(s2c);
        if (!role_message) {
            perror("null role_message");
            close(s2c);
            close(c2s);
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
            close(s2c);
            close(c2s);
            return 0;
        }
        




        //debug
        printf("we reached here!%d\n",role);
        close(s2c);
        close(c2s);
    }else if (sig == -1) {
        perror("sigtimedwait timeout or error");
        return 1;
    }else{
        perror("TIMEOUT");
        return 1;
    }



    return 0;
}