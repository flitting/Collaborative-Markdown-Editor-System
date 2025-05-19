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

// === Edit Commands ===
int markdown_insert(document *doc, uint64_t version, size_t pos, const char *content){
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
    printf("debug newline insert pos %d\n",left_pos);
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
    return result_num;

    
}
int markdown_bold(document *doc, uint64_t version, size_t start, size_t end);
int markdown_italic(document *doc, uint64_t version, size_t start, size_t end);
int markdown_blockquote(document *doc, uint64_t version, size_t pos);
int markdown_ordered_list(document *doc, uint64_t version, size_t pos);
int markdown_unordered_list(document *doc, uint64_t version, size_t pos);
int markdown_code(document *doc, uint64_t version, size_t start, size_t end);
int markdown_horizontal_rule(document *doc, uint64_t version, size_t pos);
int markdown_link(document *doc, uint64_t version, size_t start, size_t end, const char *url);

// === Utilities ===
void markdown_print(const document *doc, FILE *stream){
    const char * result = markdown_flatten(doc);
    fputs(result,stream);
    
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



