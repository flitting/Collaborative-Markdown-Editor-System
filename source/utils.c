#include "./../libs/utils.h"


char *read_full_message(int fd) {
    size_t capacity = BUFFER_SIZE_UNIT;
    size_t length = 0;
    char *message = malloc(capacity);
    if (!message) return NULL;
    ssize_t bytes;

    while (1) {
        if (length + BUFFER_SIZE_UNIT +1 > capacity) {
            capacity *= 2;
            char *temp = realloc(message, capacity);
            if (!temp) {
                free(message);
                return NULL;
            }
            message = temp;
        }

        bytes = read(fd, message + length, BUFFER_SIZE_UNIT);
        if (bytes < 0) {
            if (errno == EINTR || errno == EAGAIN) continue;
            free(message);
            return NULL;
        }
        if (bytes == 0) {
            // EOF before delimiter—return what we have (or NULL if empty)
            if (length == 0) {
                free(message);
                return NULL;
            }
            break;
        }

        length += bytes;

        // scan for the delimiter "\n\0"
        for (size_t i = 1; i < length; ++i) {
            if (message[i-1] == '\n' && message[i] == '\0') {
                // terminate the string at the '\n'
                message[i-1] = '\0';
                return message;
            }
        }
    }

    message[length < capacity ? length : capacity-1] = '\0';
    return message;
}

int search_roles(char * client_name){
    /*
    return 0 meaning no find
    return 1 meaning read
    return 2 meaning write
    return -1 meaining error
    
    */
    //debug
    printf("\n%s\n",client_name);
    char * role_path = "./roles.txt";
    FILE * file = fopen(role_path,"r");
    if (file == NULL){
        perror("error opening file");
        return -1;
    }
    char buffer[MAX_LINE_LENGTH+1];
    char username_buffer[MAX_LINE_LENGTH];
    char permission_buffer[10];

    int find_flag = 0;
    while(fgets(buffer,sizeof(buffer),file)){
        buffer[strcspn(buffer, "\n")] = 0;
        if(sscanf(buffer,"%s %s",username_buffer,permission_buffer) == 2){
            if (strcmp(client_name,username_buffer)== 0){
                if (strcmp(permission_buffer,"write")==0){
                    return 2;
                }else if (strcmp(permission_buffer,"read")==0){
                    return 1;
                }else{
                    perror("error file format");
                    return -1;
                }
            }
        }else{
            perror("wrong rales.txt");
            return -1;
        }
    }
    fclose(file);
    return 0;
}


int send_full_document(){
    // todo send document;
    return 0;
}


int receive_full_document(){
    return 0;
}

