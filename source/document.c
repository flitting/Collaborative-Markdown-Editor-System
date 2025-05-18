#include "./../libs/document.h"


chunk * chunk_init(const char * str,chunk* last_chunk, chunk* next_chunk){
    chunk * obj = (chunk*)malloc(sizeof(chunk));
    if (obj == NULL) {
        return NULL;  // error handel for malloc
    }
    //dump str into content;
    size_t len = strlen(str);
    char * content = (char *)malloc(len + 1);
    char * status = (char *)malloc(len + 1);
    if (!content || !status) {
        free(content);
        free(status);
        free(obj);
        return NULL;
    }




    strcpy(content, str);
    memset(status, STILL, len); 
    status[len] = '\0';
    obj->content = content;
    obj->status = status;
    obj->chunk_length = len+1;
    obj->chunk_old_length = 0;
    obj->chunk_new_length = 0;
    obj->last = last_chunk;
    obj->next = next_chunk;

    return obj;
}
void chunk_free(chunk * obj){
    if(obj->last)obj->last->next = obj->next;
    if(obj->next)obj->next->last = obj->last;
    free(obj->content);
    free(obj->status);
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
        char *substatus = (char *)malloc(len + 1);
        if (!substr || !substatus) {
            free(substr);
            free(substatus);
            chunk *curr = head;
            while (curr) {
                chunk *next = curr->next;
                chunk_free(curr);
                curr = next;
            }
            return -1;
        }

        strncpy(substr, original_obj->content + start, len);
        strncpy(substatus, original_obj->status + start, len);
        substr[len] = '\0';
        substatus[len] = '\0';

        chunk *new_chunk = chunk_init(substr, NULL, NULL);
        if (!new_chunk) {
            free(substr);
            free(substatus);
            chunk *curr = head;
            while (curr) {
                chunk *next = curr->next;
                chunk_free(curr);
                curr = next;
            }
            return -1;
        }
        memcpy(new_chunk->status, substatus, len + 1);
        free(substr);
        free(substatus);

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
    if (!original_obj || !original_obj->content || !original_obj->status) return -1;

    if (original_obj->last != former_chunk || original_obj->next != next_chunk) return -1;

    size_t length = strlen(original_obj->content);
    if (pos > length) return -1;

    // Split the string

    char *left_part = (char *)malloc(pos + 1);
    char *right_part = (char *)malloc(length - pos + 1);
    char *left_status = (char *)malloc(pos + 1);
    char *right_status = (char *)malloc(length - pos + 1);



    if ((pos > 0 && (!left_part || !left_status)) || (length - pos > 0 && (!right_part || !right_status))) {
        free(left_part);
        free(right_part);
        return -1;
    }

    if (pos > 0) {
        strncpy(left_part, original_obj->content, pos);
        strncpy(left_status, original_obj->status, pos);
        left_part[pos] = '\0';
        left_status[pos] = '\0';
    }

    if (length - pos > 0) {
        strcpy(right_part, original_obj->content + pos);
        strcpy(right_status, original_obj->status + pos);
    }

    chunk *left_chunk = NULL;
    chunk *right_chunk = NULL;

    if (pos > 0) {
        left_chunk = chunk_init(left_part, NULL, NULL);
        memcpy(left_chunk->status, left_status, pos + 1);
    }

    if (length - pos > 0) {
        right_chunk = chunk_init(right_part, NULL, NULL);
        memcpy(right_chunk->status, right_status, length - pos + 1);
    }

    free(left_part);
    free(left_status);
    free(right_part);
    free(right_status);

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

    size_t total_len = 0;
    chunk *curr = start_chunk;
    while (curr) {
        if (!curr->content || !curr->status) return NULL;
        total_len += strlen(curr->content);
        if (curr == end_chunk) break;
        curr = curr->next;
    }
    if (curr != end_chunk) return NULL;

    char *merged_content = (char *)malloc(total_len + 1);
    char *merged_status = (char *)malloc(total_len + 1);
    if (!merged_content || !merged_status) {
        free(merged_content); free(merged_status);
        return NULL;
    }

    merged_content[0] = '\0';
    merged_status[0] = '\0';
    curr = start_chunk;
    while (curr) {
        strcat(merged_content, curr->content);
        strcat(merged_status, curr->status);
        if (curr == end_chunk) break;
        curr = curr->next;
    }

    chunk *merged_chunk = chunk_init(merged_content, NULL, NULL);
    memcpy(merged_chunk->status, merged_status, total_len + 1);
    free(merged_content);
    free(merged_status);

    return merged_chunk;
}

// version = 0 N-1 version, version = 1, N version; version = 2, current version;
chunk * find_pos_chunk(document* doc, size_t pos, size_t *left_pos_ptr, int version) {
    *left_pos_ptr = pos;
    chunk* current_chunk = doc->start_empty_chunk->next;

    while (current_chunk && current_chunk != doc->end_empty_chunk) {
        size_t logical_len = 0;

        for (size_t i = 0; current_chunk->content[i] != '\0'; ++i) {
            char status = current_chunk->status[i];

            int include = 0;

            if (version == 0) {
                if (status == STILL || status == DELETE_OLD || status == MODIFIED_OLD ||
                    status == DELETE_NEW || status == MODIFIED_NEW || status == DEL_MOD) {
                    include = 1;
                }
            } else if (version == 1) {
                if (status == STILL || status == INSERT_OLD || status == MODIFIED_OLD ||
                    status == INSERT_NEW || status == MODIFIED_NEW || status == MOD_INS) {
                    include = 1;
                }
            } else if (version == 2) {
                if (status != DELETE_OLD && status != DELETE_NEW && status != DEL_INS && status != DEL_MOD) {
                    include = 1;
                }
            }

            if (include) {
                if (*left_pos_ptr == 0) {
                    return current_chunk;
                }
                (*left_pos_ptr)--;
                logical_len++;
            }
        }

        current_chunk = current_chunk->next;
    }

    return NULL; // Reached end without finding the position
    // if left pos = 0, reach the end of doc;
}

int find_pos_in_chunk(chunk *ck, size_t logical_pos, size_t *real_index, int version) {
    size_t count = 0;

    for (size_t i = 0; ck->content[i] != '\0'; ++i) {
        char status = ck->status[i];

        int include = 0;
        if (version == 0) {
            if (status == STILL || status == DELETE_OLD || status == MODIFIED_OLD ||
                status == DELETE_NEW || status == MODIFIED_NEW || status == DEL_MOD) {
                include = 1;
            }
        } else if (version == 1) {
            if (status == STILL || status == INSERT_OLD || status == MODIFIED_OLD ||
                status == INSERT_NEW || status == MODIFIED_NEW || status == MOD_INS) {
                include = 1;
            }
        } else if (version == 2) {
            if (status != DELETE_OLD && status != DELETE_NEW &&
                status != DEL_INS && status != DEL_MOD) {
                include = 1;
            }
        }

        if (include) {
            if (count == logical_pos) {
                *real_index = i;
                return 0;  // success
            }
            count++;
        }
    }

    return -1;  // chunk 内未找到该逻辑位置
}
// get index map ffrom logical to real index based on version 
size_t find_logical_index_map(chunk *ck, int version, size_t *map, size_t max_len) {
    size_t count = 0;
    for (size_t i = 0; ck->content[i] != '\0'; ++i) {
        char status = ck->status[i];

        int include = 0;
        if (version == 0) {
            if (status == STILL || status == DELETE_OLD || status == MODIFIED_OLD ||
                status == DELETE_NEW || status == MODIFIED_NEW || status == DEL_MOD) {
                include = 1;
            }
        } else if (version == 1) {
            if (status == STILL || status == INSERT_OLD || status == MODIFIED_OLD ||
                status == INSERT_NEW || status == MODIFIED_NEW || status == MOD_INS) {
                include = 1;
            }
        } else if (version == 2) {
            if (status != DELETE_OLD && status != DELETE_NEW &&
                status != DEL_INS && status != DEL_MOD) {
                include = 1;
            }
        }

        if (include) {
            if (count < max_len) map[count] = i;
            count++;
        }
    }
    return count;
}


void chunk_insert(chunk* ck, size_t pos, char *content, int version) {
    if (!ck || !content) return;

    size_t insert_len = strlen(content);
    if (insert_len == 0) return;

    size_t index_map[MAX_CHUNK_SIZE];
    size_t logical_count = find_logical_index_map(ck, version, index_map, MAX_CHUNK_SIZE);

    size_t real_insert_pos;

    if (pos >= logical_count) {
        real_insert_pos = strlen(ck->content);
    } else {
        real_insert_pos = index_map[pos];
    }

    size_t old_len = strlen(ck->content);
    size_t new_len = old_len + insert_len;

    // expand space
    ck->content = realloc(ck->content, new_len + 1);
    ck->status = realloc(ck->status, new_len + 1);
    if (!ck->content || !ck->status) return;

    memmove(ck->content + real_insert_pos + insert_len, ck->content + real_insert_pos, old_len - real_insert_pos + 1);
    memmove(ck->status + real_insert_pos + insert_len, ck->status + real_insert_pos, old_len - real_insert_pos + 1);

    memcpy(ck->content + real_insert_pos, content, insert_len);

    char status_char = (version == 0) ? INSERT_OLD :
                       (version == 1) ? INSERT_NEW :
                       MOD_INS;

    memset(ck->status + real_insert_pos, status_char, insert_len);
    ck->status[new_len] = '\0';
    ck->chunk_length = new_len + 1;

    if (ck->chunk_length >= MAX_CHUNK_SIZE) {
        chunk_split(ck, ck->last, ck->next);
    }
}

void chunk_change_char(chunk* ck, size_t pos, char ch, int version) {
    if (!ck || !ck->content || !ck->status) return;

    size_t real_pos = 0;
    if (find_pos_in_chunk(ck, pos, &real_pos, version) != 0) return;

    char s = ck->status[real_pos];
    if (s == DELETE_OLD || s == DELETE_NEW || s == DEL_INS || s == DEL_MOD) return;

    ck->content[real_pos] = ch;

    if (s == STILL) {
        ck->status[real_pos] = (version == 0) ? MODIFIED_OLD :
                               (version == 1) ? MODIFIED_NEW : MOD_INS;
    } else if (s == INSERT_OLD || s == INSERT_NEW) {
        // 不变
    } else {
        ck->status[real_pos] = (version == 0) ? MODIFIED_OLD :
                               (version == 1) ? MODIFIED_NEW : MOD_INS;
    }
}

int chunk_mark_delete(chunk* ck, size_t pos, size_t len, int version) {
    if (!ck || !ck->content || !ck->status) return -1;

    size_t deleted = 0;
    chunk *curr = ck;
    size_t logical_offset = pos;

    while (curr && deleted < len) {
        size_t logical_to_real[MAX_CHUNK_SIZE];
        size_t logical_count = find_logical_index_map(curr, version, logical_to_real, MAX_CHUNK_SIZE);

        while (logical_offset < logical_count && deleted < len) {
            size_t real_index = logical_to_real[logical_offset];
            char s = curr->status[real_index];

            if (s == DELETE_OLD || s == DELETE_NEW || s == DEL_INS || s == DEL_MOD) {
                return -1;  // for delete again
            }

            if (s == INSERT_OLD || s == INSERT_NEW) {
                curr->status[real_index] = DEL_INS;
            } else if (s == MODIFIED_OLD || s == MODIFIED_NEW) {
                curr->status[real_index] = DEL_MOD;
            } else {
                curr->status[real_index] = (version == 0) ? DELETE_OLD : DELETE_NEW;
            }

            deleted++;
            logical_offset++;
        }

        if (deleted < len) {
            curr = curr->next;
            logical_offset = 0;  // start from 0 for next chunk
        }
    }

    return deleted;
}




