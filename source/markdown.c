#include "../libs/markdown.h"
//Each command must include the version number of the document it intends to modify. (only for COMP9017 students).


// create a empty document
document * markdown_init(void){
    document * doc = (document*)malloc(sizeof(document));
    doc->version = 0;
    doc->start_empty_chunk = chunk_init("",NULL,NULL);
    // Create an editable initial chunk with empty content.
    chunk * initial_chunk = chunk_init("", doc->start_empty_chunk, NULL);

    doc->end_empty_chunk = chunk_init("",initial_chunk,NULL);
    doc->start_empty_chunk->next = initial_chunk;
    initial_chunk->next = doc->end_empty_chunk;
    char * cache = (char*)malloc(1);
    cache[0] = '\0';
    doc->flatten_cache = cache;

    return doc;
}
void markdown_free(document *doc){
    chunk * current = doc->start_empty_chunk;
    while (current) {
        chunk *next = current->next;
        chunk_free(current);
        current = next;
    }

    free(doc->flatten_cache);

    free(doc);
}

// -1 wrong version
// -2 deleted pos
// -3 wrong range
// -4 wrong permission  as no user data in doc, it can't be used
// -5 other reasons
// -6 unrecovered error

// === Edit Commands ===
int markdown_insert(document *doc, uint64_t version, size_t pos, const char *content){
    // the pos should always the logical pos
    int version_num = markdown_version_check(doc,version);
    if (version_num < 0) return -1;//wrong version
    size_t left_pos = pos;
    chunk * obj = find_pos_chunk(doc,pos,&left_pos,version_num);
    if (obj == NULL) {
        return -3;
    }
    char * insert_content = (char*)malloc(strlen(content)+1);
    strcpy(insert_content,content);
    int result_num = chunk_insert(obj,left_pos,insert_content,version_num,0);
    free(insert_content);
    return result_num;
}
int markdown_delete(document *doc, uint64_t version, size_t pos, size_t len){
    int version_num = markdown_version_check(doc,version);
    if (version_num < 0) return -1;//wrong version
    size_t left_pos = pos;
    chunk * obj = find_pos_chunk(doc,pos,&left_pos,version_num);
    int result_num = chunk_mark_delete(obj,left_pos,len,version_num);
    return result_num;
}

// === Formatting Commands ===
int markdown_newline(document *doc, size_t version, size_t pos){
    // all effect newline should add from here.
    int version_num = markdown_version_check(doc,version);
    if (version_num < 0) return -1;//wrong version
    
    const char * newline = "\n";

    
    size_t left_pos = pos;
    chunk * obj = find_pos_chunk(doc,pos,&left_pos,version_num);
    if (obj == NULL) {
        return -3;          // out-of-range cursor
    }

    char * insert_newline = (char*)malloc(strlen(newline)+1);
    strcpy(insert_newline,newline);
    int result_num = chunk_insert(obj,left_pos,insert_newline,version_num,0);
    free(insert_newline);

    return result_num;

}
int markdown_heading(document *doc, uint64_t version, size_t level, size_t pos){
    int version_num = markdown_version_check(doc,version);
    if (version_num < 0) return -1;//wrong version
    if(level==0 ||level >3) return -3;//out of range


    size_t left_pos = pos;
    chunk * obj = find_pos_chunk(doc,pos,&left_pos,version_num);
    if (obj == NULL) {
        return -3;          // out-of-range cursor
    }

    char * insert_heading = (char*)malloc(level +1 +1);
    for(size_t i = 0;i<level;i++){
        insert_heading[i]='#';
    }
    insert_heading[level]=' ';
    insert_heading[level+1] = '\0';

    int result_num = chunk_insert(obj,left_pos,insert_heading,version_num,0);
    free(insert_heading);
    // block level element
    if (pos !=0 && !result_num){
        // insert a newline
        int newline_result = markdown_newline(doc,version,pos);
        if(newline_result) return -6;
    }


    return result_num;

    
}
int markdown_bold(document *doc, uint64_t version, size_t start, size_t end){
    int version_num = markdown_version_check(doc,version);
    if (version_num < 0) return -1;//wrong version
    if(start>=end){
        return -3;
    }
    
    // add end first to prevent memory change in range error
    size_t left_end_pos = end;
    chunk * end_obj = find_pos_chunk(doc,end,&left_end_pos,version_num);
    if (end_obj == NULL) {
        return -3;          // out-of-range cursor
    }
    size_t left_start_pos = start;
    chunk* start_obj = find_pos_chunk(doc,start,&left_start_pos,version_num);
    if (start_obj == NULL) {
        return -3;          // out-of-range cursor
    }
    // I need to consider the delete error for ranges
    // if statuses between poses are deleted,just send wrong.
    char * two_stars = (char*)malloc(3);
    two_stars[0]='*';
    two_stars[1]='*';
    two_stars[2]='\0';
    // if meet delete char, search to left
    int end_result = chunk_insert(end_obj,left_end_pos,two_stars,version_num,0);
    if(end_result){
        free(two_stars);
        return end_result; // error handle
    }
    // search to right
    int start_result = chunk_insert(start_obj,left_start_pos,two_stars,version_num,1);
    free(two_stars);
    if (start_result){
        return -6; // ojb_end memory done but start un done, should exit
    }

    

    return 0;
}
int markdown_italic(document *doc, uint64_t version, size_t start, size_t end){
    int version_num = markdown_version_check(doc,version);
    if (version_num < 0) return -1;//wrong version
    if(start>=end){
        return -3;
    }
    
    // add end first to prevent memory change in range error
    size_t left_end_pos = end;
    chunk * end_obj = find_pos_chunk(doc,end,&left_end_pos,version_num);
    if (end_obj == NULL) {
        return -3;          // out-of-range cursor
    }
    size_t left_start_pos = start;
    chunk* start_obj = find_pos_chunk(doc,start,&left_start_pos,version_num);
    if (start_obj == NULL) {
        return -3;          // out-of-range cursor
    }

    char * star = (char*)malloc(2);
    star[0]='*';
    star[1]='\0';
    // if meet delete char, search to left
    int end_result = chunk_insert(end_obj,left_end_pos,star,version_num,0);
    if(end_result){
        free(star);
        return end_result; // error handle
    }
    // search to right
    int start_result = chunk_insert(start_obj,left_start_pos,star,version_num,1);
    free(star);
    if (start_result){
        return -6; // ojb_end memory done but start un done, should exit
    }

   

    return 0;
}
int markdown_blockquote(document *doc, uint64_t version, size_t pos){
    int version_num = markdown_version_check(doc,version);
    if (version_num < 0) return -1;//wrong version
    size_t left_pos = pos;
    chunk * obj = find_pos_chunk(doc,pos,&left_pos,version_num);
    if (obj == NULL) {
        return -3;          // out-of-range cursor
    }

    char * blockquote = (char * )malloc(3);
    blockquote[0] = '>';
    blockquote[1] = ' ';
    blockquote[2] = '\0';

    int result_num = chunk_insert(obj,left_pos,blockquote,version_num,0);
    free(blockquote);
    // block level element
    if (pos !=0 && !result_num){
        // insert a newline
        int newline_result = markdown_newline(doc,version,pos);
        if(newline_result) return -6;
    }


    return result_num;
}
int markdown_ordered_list(document *doc, uint64_t version, size_t pos){
    int version_num = markdown_version_check(doc,version);
    if (version_num < 0) return -1;//wrong version
    //todo
    return -4;
}
int markdown_unordered_list(document *doc, uint64_t version, size_t pos){
    int version_num = markdown_version_check(doc,version);
    if (version_num < 0) return -1;//wrong version
    size_t left_pos = pos;
    chunk * obj = find_pos_chunk(doc,pos,&left_pos,version_num);
    if (obj == NULL) {
        return -3;          // out-of-range cursor
    }

    char * prefix = (char * )malloc(3);
    prefix[0] = '-';
    prefix[1] = ' ';
    prefix[2] = '\0';

    int result_num = chunk_insert(obj,left_pos,prefix,version_num,0);
    free(prefix);
    // block level element
    if (pos !=0 && !result_num){
        // insert a newline
        int newline_result = markdown_newline(doc,version,pos);
        if(newline_result) return -6;
    }
    return result_num;
}
int markdown_code(document *doc, uint64_t version, size_t start, size_t end){
    int version_num = markdown_version_check(doc,version);
    if (version_num < 0) return -1;//wrong version
    if(start>=end){
        return -3;
    }
    
    // add end first to prevent memory change in range error
    size_t left_end_pos = end;
    chunk * end_obj = find_pos_chunk(doc,end,&left_end_pos,version_num);
    if (end_obj == NULL) {
        return -3;          // out-of-range cursor
    }
    size_t left_start_pos = start;
    chunk* start_obj = find_pos_chunk(doc,start,&left_start_pos,version_num);
    if (start_obj == NULL) {
        return -3;          // out-of-range cursor
    }

    char * backtick = (char*)malloc(2);
    backtick[0]='`';
    backtick[1]='\0';
    // if meet delete char, search to left
    int end_result = chunk_insert(end_obj,left_end_pos,backtick,version_num,0);
    if(end_result){
        free(backtick);
        return end_result; // error handle
    }
    // search to right
    int start_result = chunk_insert(start_obj,left_start_pos,backtick,version_num,1);
    free(backtick);
    if (start_result){
        return -6; // ojb_end memory done but start un done, should exit
    }

    

    return 0;
}
int markdown_horizontal_rule(document *doc, uint64_t version, size_t pos){
    int result_num = markdown_insert(doc,version,pos,"---");
    if (result_num) return result_num;
    if (pos !=0){
        // insert a newline
        int newline_result = markdown_newline(doc,version,pos);
        if(newline_result) return -6;
    }
    return result_num;
}
int markdown_link(document *doc, uint64_t version, size_t start, size_t end, const char *url){
    int version_num = markdown_version_check(doc,version);
    if (version_num < 0) return -1;//wrong version
    if(start>=end){
        return -3;
    }
    
    // add end first to prevent memory change in range error
    size_t left_end_pos = end;
    chunk * end_obj = find_pos_chunk(doc,end,&left_end_pos,version_num);
    if (end_obj == NULL) {
        return -3;          // out-of-range cursor
    }
    size_t left_start_pos = start;
    chunk* start_obj = find_pos_chunk(doc,start,&left_start_pos,version_num);
    if (start_obj == NULL) {
        return -3;          // out-of-range cursor
    }

    char * left = (char*)malloc(2);
    left[0]='[';
    left[1]='\0';
    char * right = (char*)malloc(strlen(url)+1+2+1);
    right[0] = ']';
    right[1] = '(';
    right[2] = '\0';
    strcat(right,url);
    strcat(right,")");


    // if meet delete char, search to left
    int end_result = chunk_insert(end_obj,left_end_pos,right,version_num,0);
    free(right);
    if(end_result){
        return end_result; // error handle
    }
    // search to right
    int start_result = chunk_insert(start_obj,left_start_pos,left,version_num,1);
    free(left);
    if (start_result){
        return -6; // ojb_end memory done but start un done, should exit
    }
    
    return start_result;
    
}

// === Utilities ===
void markdown_print(const document *doc, FILE *stream){
    char * result = markdown_flatten(doc);
    fputs(result,stream);
    fputs("\n",stream);
    free(result);
    
}
char *markdown_flatten(const document *doc){
    if (!doc || !doc->start_empty_chunk || !doc->end_empty_chunk)return NULL;

    if(doc->flatten_cache){// ONLY update in version increment ,it should always have cache;
        // suppose the outer function will free the content;
        char * result = (char *)malloc(strlen(doc->flatten_cache)+1);
        strcpy(result, doc->flatten_cache);
        return result;
    }else{
        char * result = (char *)malloc(1);
        result[0]='\0';
        return result;
    }

}



// === Versioning ===
void markdown_increment_version(document *doc){
    // in comp9017,increment by one version
    char * preview = document_serialize(doc);
    free(doc->flatten_cache);
    doc->flatten_cache = preview;

    // update status and chunks
    chunk *current = doc->start_empty_chunk->next;

    while (current && current != doc->end_empty_chunk) {
        size_t write_pos = 0;

        for (size_t read_pos = 0; current->content[read_pos] != '\0'; ++read_pos) {
            char s = current->status[read_pos];
            char ch = current->content[read_pos];

            switch (s) {
                case STILL:
                case MODIFIED_OLD:
                case INSERT_OLD:
                    // Keep character, mark as STILL
                    current->content[write_pos] = ch;
                    current->status[write_pos++] = STILL;
                    break;

                case MODIFIED_NEW:
                case MOD_INS:
                    // Keep character, mark as MODIFIED_OLD
                    current->content[write_pos] = ch;
                    current->status[write_pos++] = MODIFIED_OLD;
                    break;

                case INSERT_NEW:
                    // Keep character, but mark as INSERT_OLD
                    current->content[write_pos] = ch;
                    current->status[write_pos++] = INSERT_OLD;
                    break;

                case DELETE_NEW:
                case DEL_INS:
                case DEL_MOD:
                    current->content[write_pos] = ch;
                    current->status[write_pos++] = DELETE_OLD;
                    break;

                case DELETE_OLD:
                    // Discard character
                    break;
                default:
                    // Fallback: treat unknown status as visible
                    current->content[write_pos] = ch;
                    current->status[write_pos++] = STILL;
                    break;
            }
        }

        current->content[write_pos] = '\0';
        current->status[write_pos] = '\0';
        current->chunk_length = write_pos + 1;

        update_chunk_lengths(current);
        current = current->next;
    }

    doc->version += 1;
}



// check COMP9017 here
int markdown_version_check(document *doc ,size_t version){
    size_t current_version = doc->version;
    if(current_version == version) return 1;
    if(current_version == version + 1 && COMP9017) return 0;
    return -1;// wrong version
}



// from document.c 



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
    update_chunk_lengths(obj);
    obj->last = last_chunk;
    obj->next = next_chunk;
    chunk_split(obj,last_chunk,next_chunk);

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
        update_chunk_lengths(new_chunk);
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
        update_chunk_lengths(left_chunk);
    }

    if (length - pos > 0) {
        right_chunk = chunk_init(right_part, NULL, NULL);
        memcpy(right_chunk->status, right_status, length - pos + 1);
        update_chunk_lengths(right_chunk);
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
    // manually free it
    free(original_obj->content);
    free(original_obj->status);
    free(original_obj);
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
    update_chunk_lengths(merged_chunk);
    free(merged_content);
    free(merged_status);

    return merged_chunk;
}

// version = 0 N-1 version, version = 1, N version; version = 2, preview version;
// case 1: reach the logical end but not doc end
// case 2: reach the doc end
// find pos just behind the former pos char
chunk * find_pos_chunk(document* doc, size_t pos, size_t *left_pos_ptr, int version) {
    // it accepts and returns the logical pos
    if(pos == 0){
        *left_pos_ptr = 0;
        return doc->start_empty_chunk->next;
    }
    *left_pos_ptr = pos;
    chunk* current_chunk = doc->start_empty_chunk->next;

    while (current_chunk && current_chunk != doc->end_empty_chunk) {
        size_t logical_len = (version == 0) ? current_chunk->chunk_old_length :
                            (version == 1) ? current_chunk->chunk_new_length :
                                             strlen(current_chunk->content); // fallback

        if (*left_pos_ptr <= logical_len) {
            return current_chunk;
        }

        *left_pos_ptr -= logical_len;
        current_chunk = current_chunk->next;
    }

    return NULL;
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
            if (status != DELETE_OLD && status!= INSERT_NEW) {
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

    return -1;  // chunk not found
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
            if (status != DELETE_OLD && status!= INSERT_NEW) {
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

// func for chunk_insert
int is_deleted_status(char s,int version) {
    if(version==1)return s == DELETE_NEW || s == DEL_INS || s == DEL_MOD;
    else return s == DELETE_OLD || s == DELETE_NEW || s == DEL_INS || s == DEL_MOD;
}

int is_status_can_be_seen(char s,int version){
    if(version == 0) return s == (s == STILL || s == DELETE_OLD || s == MODIFIED_OLD ||
                s == DELETE_NEW || s == MODIFIED_NEW || s == DEL_MOD);
    else if (version == 1) return s != DELETE_OLD && s!= INSERT_NEW;
    else return (s != DELETE_OLD && s != DELETE_NEW &&
                s != DEL_INS && s != DEL_MOD);
}


// func for search the first non deleting char before insert into delete range
void find_insert_point(chunk *ck, size_t base_index, int direction, chunk **out_chunk, size_t *out_index,int version) {
    if (!ck || !ck->content || !ck->status) {
        *out_chunk = ck;
        *out_index = 0;
        return;
    }

    if (direction == 0) {
        if(ck->chunk_length-1 == base_index){
            // at the end of chunk
            if(ck->next->next != NULL){
                // next is not end_empty_chunk
                if(!is_deleted_status(ck->next->content[0],version)){
                    *out_chunk = ck;
                    *out_index = base_index;
                    return;
                }
            }

        }else if (!is_deleted_status(ck->status[base_index],version)) {
            // if non delete char ,insert just before
            *out_chunk = ck;
            *out_index = base_index;
            return;
        }

        // find first non deleting acceptable char
        for (ssize_t i = (ssize_t)base_index - 1; i >= 0; --i) {
            if (!is_deleted_status(ck->status[i],version)) {
                if(is_status_can_be_seen(ck->status[i],version))
                {
                    *out_chunk = ck;
                    *out_index = i + 1;
                    return;
                }
            }
        }
        // find in the last chunk
        if (ck->last && ck->last->last != NULL) {
            size_t len = strlen(ck->last->content);
            if (len == 0) {
                find_insert_point(ck->last, 0, direction, out_chunk, out_index,version);
            } else {
                find_insert_point(ck->last, len - 1, direction, out_chunk, out_index,version);
            }
        } else {
            // insert in the begining of doc
            *out_chunk = ck;
            *out_index = 0;
        }
    } else {

        // search the following chunk
        size_t len = strlen(ck->content);
        for (size_t i = base_index; i < len; ++i) {
            if (!is_deleted_status(ck->status[i],version)) {
                if(is_status_can_be_seen(ck->status[i],version))
                {
                    *out_chunk = ck;
                    *out_index = i;
                    return;
                }
            }
        }
        
        if (ck->next && ck->next->next != NULL) {
            find_insert_point(ck->next, 0, direction, out_chunk, out_index,version);
        } else {
            // to the end of doc
            *out_chunk = ck;
            *out_index = len;
        }
    }
}

// direction for mutliclient handle. 0 to insert before first non delelte char,1 to insert after.
// direction 1 for
// """ If only one position is within the deleted region, the cursor position within the
// deleted region will be adjusted to the closer of the deleted edges to the valid cursor
// position. """
int chunk_insert(chunk* ck, size_t pos, char *content, int version, int direction) {
    // pos char can be \0
    // I need to considering deleted chars,if so ,skip it in the direction
    
    
    if (version == 2) return -1;
    // version = 0,1
    // version = 2 is for preview ,no effect
    if (!ck || !content) return -5; //other error

    size_t insert_len = strlen(content);
    if (insert_len == 0) return -5;

    size_t index_map[MAX_CHUNK_SIZE];
    size_t logical_count = find_logical_index_map(ck, version, index_map, MAX_CHUNK_SIZE);


    if (pos > logical_count) {
        return -3; // out of range
    }
    size_t base_index;
    if(logical_count == 0 || pos == 0){
        if(ck->last->last != NULL){
            logical_count = find_logical_index_map(ck->last, version, index_map, MAX_CHUNK_SIZE);
            // logical count for last chunk, it can be zero
            pos += logical_count;
            // find in last chunk
            chunk_insert(ck->last,pos,content,version,direction);

        }else{
            base_index = 0;
            direction =0;
        }


    }else{
        base_index = index_map[pos-1]+1;
    }



    chunk *insert_chunk = NULL;
    
    size_t real_insert_pos = 0;
    find_insert_point(ck, base_index, direction, &insert_chunk, &real_insert_pos,version);


    size_t old_len = strlen(insert_chunk->content);
    size_t new_len = old_len + insert_len;

    // expand space
    char* new_content = realloc(insert_chunk->content, new_len + 1);
    char* new_status = realloc(insert_chunk->status, new_len + 1);

    if (!new_content || !new_status) {
        free(new_content); // 
        free(new_status);  // 
        return -5;
    }

    insert_chunk->content = new_content;
    insert_chunk->status = new_status;
    if (!insert_chunk->content || !insert_chunk->status) return -5;

    memmove(insert_chunk->content + real_insert_pos + insert_len, insert_chunk->content + real_insert_pos, old_len - real_insert_pos + 1);
    memmove(insert_chunk->status + real_insert_pos + insert_len, insert_chunk->status + real_insert_pos, old_len - real_insert_pos + 1);

    memcpy(insert_chunk->content + real_insert_pos, content, insert_len);
    //
    char status_char = (version == 0) ? INSERT_OLD :
                       (version == 1) ? INSERT_NEW : INSERT_NEW;

    memset(insert_chunk->status + real_insert_pos, status_char, insert_len);
    insert_chunk->status[new_len] = '\0';
    insert_chunk->chunk_length = new_len + 1;


    char * newline = "\\n";
    if (strcmp(newline,content)==0){
        if(real_insert_pos!=0){
            // splite the chunk with newline;
            int result = chunk_split_at(insert_chunk,real_insert_pos,insert_chunk->last,insert_chunk->next);
            if(result){
                return -5;
            }
        }
    }else if (insert_chunk->chunk_length >= MAX_CHUNK_SIZE) {
        int result = chunk_split(insert_chunk, insert_chunk->last, insert_chunk->next);
        if (result){
            return -5;
        }
    }
    return 0;
}

int chunk_change_char(chunk* ck, size_t pos, char ch, int version) {
    // version = 0,1
    // version = 2 is for preview ,no effect
    if (version == 2) return -1;//wrong version
    if (!ck || !ck->content || !ck->status) return -5; //other error

    size_t real_pos = 0;
    if (find_pos_in_chunk(ck, pos, &real_pos, version) != 0) return -3;// wrong range

    char s = ck->status[real_pos];
    if (s == DELETE_OLD || s == DELETE_NEW || s == DEL_INS || s == DEL_MOD) return -2; //deleted range

    ck->content[real_pos] = ch;

    if (s == STILL) {
        ck->status[real_pos] = (version == 0) ? MODIFIED_OLD :
                               (version == 1) ? MODIFIED_NEW : MODIFIED_NEW;
    } else if (s == INSERT_OLD ) {
        ck->status[real_pos] = MOD_INS;
    } else {// error
        ck->status[real_pos] = (version == 0) ? MODIFIED_OLD :
                               (version == 1) ? MODIFIED_NEW : MODIFIED_NEW;
    }
    return 0;
}

int chunk_mark_delete(chunk* ck, size_t pos, size_t len, int version) {
    // version = 0,1
    // version = 2 is for preview ,no effect
    if (version == 2) return -1;
    if (!ck || !ck->content || !ck->status) return -5;

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
                return -2;  // for delete again
            }

            if (s == INSERT_OLD) {
                curr->status[real_index] = DEL_INS;
            } else if (s == MODIFIED_OLD || s == MODIFIED_NEW) {// for modified new? can I delete it? I think I can
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

    return 0;
}

void update_chunk_lengths(chunk* ck) {
    if (!ck || !ck->status) return;

    size_t old_len = 0;
    size_t new_len = 0;

    for (size_t i = 0; ck->status[i] != '\0'; ++i) {
        char s = ck->status[i];


        if (s == STILL || s == DELETE_OLD || s == MODIFIED_OLD ||
            s == DELETE_NEW || s == MODIFIED_NEW || s == DEL_MOD){
            old_len++;
        }

        if (s != DELETE_OLD && s!= INSERT_NEW) {
            new_len++;
        }
    }

    ck->chunk_old_length = old_len;
    ck->chunk_new_length = new_len;
}



char * document_serialize(document *doc) {
    // ONLY support for preview version
    if (!doc || !doc->start_empty_chunk || !doc->end_empty_chunk) return NULL;

    size_t buffer_size = 8192;
    size_t total_len = 0;

    char *result = (char *)malloc(buffer_size);
    if (!result) return NULL;

    chunk *curr = doc->start_empty_chunk->next;

    while (curr && curr != doc->end_empty_chunk) {
        for (size_t i = 0; curr->content[i] != '\0'; ++i) {
            char s = curr->status[i];


            if (s != DELETE_OLD && s != DELETE_NEW &&
                s != DEL_INS && s != DEL_MOD) {

                if (total_len + 1 >= buffer_size) {
                    size_t new_size = buffer_size * 2;
                    char *new_result = (char *)realloc(result, new_size);
                    if (!new_result) {
                        free(result);
                        return NULL;
                    }
                    result = new_result;
                    buffer_size = new_size;
                }

                result[total_len++] = curr->content[i];
            }
        }
        curr = curr->next;
    }

    result[total_len] = '\0';
    return result;
}


