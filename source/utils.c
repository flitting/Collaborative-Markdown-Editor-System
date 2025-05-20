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


char *dump_commandlogs(Commandlogs *head, size_t *total_size) {
    if(head==NULL){
        *total_size = 0;
        char *dump = (char *)malloc(1);
        dump[0] = '\0';
        return dump;
    }
    Commandlogs *curr = head;
    uint64_t current_version = (uint64_t)-1;
    size_t buffer_capacity = 4096;
    size_t offset = 0;

    char *dump = malloc(buffer_capacity);
    if (!dump) return NULL;

    while (curr) {
        if (curr->version != current_version) {
            if (current_version != (uint64_t)-1) {
                // Add END line
                const char *end_line = "END\n";
                size_t len = strlen(end_line);
                if (offset + len >= buffer_capacity) {
                    buffer_capacity *= 2;
                    dump = realloc(dump, buffer_capacity);
                }
                memcpy(dump + offset, end_line, len);
                offset += len;
            }

            // Add VERSION line
            char version_line[64];
            int written = snprintf(version_line, sizeof(version_line), "VERSION %lu\n", curr->version);
            if (offset + written >= buffer_capacity) {
                buffer_capacity *= 2;
                dump = realloc(dump, buffer_capacity);
            }
            memcpy(dump + offset, version_line, written);
            offset += written;

            current_version = curr->version;
        }

        // Add command log line
        char command_line[1024];
        snprintf(command_line, sizeof(command_line),
                 "EDIT %d %s%s", curr->client_id, curr->cmd, curr->response);
        size_t len = strlen(command_line);
        if (offset + len >= buffer_capacity) {
            buffer_capacity = buffer_capacity * 2 + len;
            dump = realloc(dump, buffer_capacity);
        }
        memcpy(dump + offset, command_line, len);
        offset += len;

        curr = curr->next;
    }

    // Add final END line if needed
    if (current_version != (uint64_t)-1) {
        const char *end_line = "END\n";
        size_t len = strlen(end_line);
        if (offset + len >= buffer_capacity) {
            buffer_capacity += len;
            dump = realloc(dump, buffer_capacity);
        }
        memcpy(dump + offset, end_line, len);
        offset += len;
    }

    // Null-terminate the result
    if (offset + 1 >= buffer_capacity) {
        dump = realloc(dump, buffer_capacity + 1);
    }
    dump[offset] = '\0';
    *total_size = offset;
    return dump;
}






void free_logs(Commandlogs *logs) {
    Commandlogs *current = logs;
    Commandlogs *next;

    while (current != NULL) {
        next = current->next; 
        free(current->cmd);    
        free(current);        
        current = next;        // Move to the next node
    }
}