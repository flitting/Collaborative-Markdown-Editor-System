#include "./../libs/document.h"


chunk * chunk_init(const char * str,chunk* last_chunk, chunk* next_chunk){
    chunk * obj = (chunk*)malloc(sizeof(chunk));
    if (obj == NULL) {
        return NULL;  // error handel for malloc
    }
    //dump str into content;
    char * content = (char *)malloc(strlen(str)+1);
    if (content == NULL) {
        free(obj);  // error handel for malloc
        return NULL;
    }




    strcpy(content, str);
    obj->content = content;
    obj->chunk_length = strlen(content)+1;
    obj->last = last_chunk;
    obj->next = next_chunk;

    return obj;
}
void chunk_free(chunk * obj){
    if(obj->last)obj->last->next = obj->next;
    if(obj->next)obj->next->last = obj->last;
    free(obj->content);
    free(obj);
}
int chunk_split(chunk *original_obj, chunk *former_chunk, chunk *next_chunk) {
    if (!original_obj || !original_obj->content) {
        return -1;
    }

    // Ensure original_obj is properly linked to its neighbors
    if (original_obj->last != former_chunk || original_obj->next != next_chunk) {
        return -1;
    }

    size_t total_length = strlen(original_obj->content);

    if(total_length<= MAX_CHUNK_SIZE - 1) return 0;//do nothing for this case;

    size_t num_chunks = (total_length + MAX_CHUNK_SIZE - 1) / MAX_CHUNK_SIZE;

    chunk *head = NULL;
    chunk *tail = NULL;

    for (size_t i = 0; i < num_chunks; ++i) {
        size_t start = i * MAX_CHUNK_SIZE;
        size_t len = (start + MAX_CHUNK_SIZE <= total_length)
                         ? MAX_CHUNK_SIZE
                         : total_length - start;

        char *substr = (char *)malloc(len + 1);
        if (!substr) {
            // Free already allocated chunks
            chunk *curr = head;
            while (curr) {
                chunk *next = curr->next;
                chunk_free(curr);
                curr = next;
            }
            return -1;
        }

        strncpy(substr, original_obj->content + start, len);
        substr[len] = '\0';

        chunk *new_chunk = chunk_init(substr, NULL, NULL);
        free(substr);
        if (!new_chunk) {
            chunk *curr = head;
            while (curr) {
                chunk *next = curr->next;
                chunk_free(curr);
                curr = next;
            }
            return -1;
        }

        if (tail) {
            tail->next = new_chunk;
            new_chunk->last = tail;
        } else {
            head = new_chunk;
        }

        tail = new_chunk;
    }

    // Connect to former_chunk and next_chunk
    if (former_chunk) {
        former_chunk->next = head;
        head->last = former_chunk;
    }
    if (next_chunk) {
        next_chunk->last = tail;
        tail->next = next_chunk;
    }

    chunk_free(original_obj);
    return 0;
}

int chunk_split_at(chunk *original_obj, size_t pos, chunk *former_chunk, chunk *next_chunk) {

    // size of original_obj can be larger than MAX_CHUNK_LENGTH
    // make sure pos < obj->length
    if (!original_obj || !original_obj->content) return -1;

    if (original_obj->last != former_chunk || original_obj->next != next_chunk) return -1;

    size_t length = strlen(original_obj->content);
    if (pos > length) return -1;

    // Split the string
    char *left_part = (char *)malloc(pos + 1);
    char *right_part = (char *)malloc(length - pos + 1);
    if ((pos > 0 && !left_part) || (length - pos > 0 && !right_part)) {
        free(left_part);
        free(right_part);
        return -1;
    }

    if (pos > 0) {
        strncpy(left_part, original_obj->content, pos);
        left_part[pos] = '\0';
    }

    if (length - pos > 0) {
        strcpy(right_part, original_obj->content + pos);
    }

    chunk *left_chunk = NULL;
    chunk *right_chunk = NULL;

    if (pos > 0) {
        left_chunk = chunk_init(left_part, NULL, NULL);
        if (!left_chunk) {
            free(left_part);
            free(right_part);
            return -1;
        }
    }

    if (length - pos > 0) {
        right_chunk = chunk_init(right_part, NULL, NULL);
        if (!right_chunk) {
            free(right_part);
            if (left_chunk) chunk_free(left_chunk);
            return -1;
        }
    }

    free(left_part);
    free(right_part);

    // Connect them
    if (former_chunk) {
        former_chunk->next = left_chunk ? left_chunk : right_chunk;
    }
    if (left_chunk) {
        left_chunk->last = former_chunk;
        left_chunk->next = right_chunk;
    }
    if (right_chunk) {
        right_chunk->last = left_chunk ? left_chunk : former_chunk;
        right_chunk->next = next_chunk;
    }
    if (next_chunk) {
        next_chunk->last = right_chunk ? right_chunk : left_chunk;
    }

    chunk_free(original_obj);
    return 0;
}

chunk *chunk_merge(chunk *start_chunk, chunk *end_chunk) {
    if (!start_chunk || !end_chunk) return NULL;

    if (start_chunk == end_chunk) return start_chunk;
    // Calculate total length
    size_t total_length = 0;
    chunk *curr = start_chunk;
    while (curr) {
        if (!curr->content) return NULL;
        total_length += strlen(curr->content);
        if (curr == end_chunk) break;
        curr = curr->next;
    }

    // If end_chunk was not reached from start_chunk (broken link)
    if (curr != end_chunk) return NULL;

    // Allocate buffer for merged content
    char *merged_content = (char *)malloc(total_length + 1);
    if (!merged_content) return NULL;

    // Copy all contents into the buffer
    merged_content[0] = '\0';  // Ensure starting with empty string
    curr = start_chunk;
    while (curr) {
        strcat(merged_content, curr->content);
        if (curr == end_chunk) break;
        curr = curr->next;
    }

    // Create the new chunk (no links by default)
    chunk *merged_chunk = chunk_init(merged_content, NULL, NULL);
    free(merged_content);  // chunk_init makes its own copy

    return merged_chunk;
}

chunk * find_pos_chunk(document* doc, size_t pos,size_t *left_pos_ptr){
    *left_pos_ptr = pos;
    chunk* currnet_chunk = doc->start_empty_chunk->next;
    while(*left_pos_ptr >= currnet_chunk->chunk_length-1){
        if(currnet_chunk== doc->end_empty_chunk) return NULL;//create chunk if pos ==0 or just error;
        *left_pos_ptr = *left_pos_ptr - (currnet_chunk->chunk_length-1);
        currnet_chunk = currnet_chunk ->next;
    } 
    return currnet_chunk;
}



int markdown_instant_insert(document *doc, size_t pos, const char *content){
    // just insert and don't care about anything
    // content should not be NULL
    if (pos<0) return -1;

    size_t length = strlen(content)+1;
    chunk * start_chunk = NULL;
    size_t left_pos = pos;
    start_chunk = find_pos_chunk(doc,pos,&left_pos);
    if (!start_chunk){
        if (pos == 0){
            start_chunk = chunk_init(content,doc->start_empty_chunk,doc->end_empty_chunk);
            chunk_split(start_chunk,start_chunk->last,start_chunk->next);
        }else{
            return -1;
        }
    }else{
        size_t len1 = strlen(start_chunk->content);
        size_t len2 = strlen(content);
        memmove(start_chunk->content + left_pos + len2, start_chunk->content + left_pos, len1-left_pos+1);
        memcpy(start_chunk->content + left_pos,content,len2);
        chunk_split(start_chunk,start_chunk->last,start_chunk->next);
        return 0;
    }

}
// I should test these functions
int markdown_instant_delete(document *doc, size_t pos, size_t len){
    if(pos<0){
        len = len + pos;
        pos = 0;
    }
    if(len==0){
        return 0;
    }else if (len<0){
        return -1;
    }
    // just delete and don't care about anything
    chunk * current = NULL;
    size_t left_pos = pos;
    current = find_pos_chunk(doc,pos,&left_pos);
    if(current->chunk_length-1-left_pos+1 < 0){
        // for pos overflow chunk
        while(current->next != doc->end_empty_chunk){
            current = chunk_merge(current->last,current->next->next);
        }
        if(current->next == doc->end_empty_chunk && current->chunk_length-1-left_pos+1 < 0){
            len = current->chunk_length-1 - left_pos;
        }

    }
    memmove(current->content + left_pos,current->content + left_pos + len, current->chunk_length-1 - left_pos-len + 1);
    return 0;
}


