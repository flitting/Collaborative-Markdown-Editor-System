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
    
    const char * newline = "\\n";

    
    size_t left_pos = pos;
    chunk * obj = find_pos_chunk(doc,pos,&left_pos,version_num);
    if (obj == NULL) {
        return -3;          // out-of-range cursor
    }

    char * insert_newline = (char*)malloc(strlen(newline)+1);
    strcpy(insert_newline,newline);
    //debug
    printf("debug newline insert pos %ld\n",left_pos);
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
    const char * result = markdown_flatten(doc);
    fputs(result,stream);
    fputs("\n",stream);
    
}
char *markdown_flatten(const document *doc){
    if (!doc || !doc->start_empty_chunk || !doc->end_empty_chunk)return NULL;

    if(doc->flatten_cache){// ONLY update in version increment ,it should always have cache;
        // suppose the outer function will free the content;
        char * result = (char *)malloc(strlen(doc->flatten_cache)+1);
        strcpy(result, doc->flatten_cache);
        return result;
    }else{
        return "";
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



