#include "../libs/markdown.h"
//Each command must include the version number of the document it intends to modify. (only for COMP9017 students).


// create a empty document
document * markdown_init(void){
    document * doc = (document*)malloc(sizeof(document));
    doc->version = 0;
    doc->chunk_num = 0;
    doc->start_empty_chunk = chunk_init("",NULL,NULL);
    doc->end_empty_chunk = chunk_init("",doc->end_empty_chunk,NULL);
    doc->start_empty_chunk->next = doc->end_empty_chunk;




    return doc;
}
void markdown_free(document *doc){
    chunk * current = doc->start_empty_chunk;
    while(current->next != doc->end_empty_chunk){
        chunk_free(current->next);
    }
    chunk_free(doc->start_empty_chunk);
    chunk_free(doc->end_empty_chunk);
    free(doc->flatten_cache);
    free(doc);
}

// === Edit Commands ===
int markdown_insert(document *doc, uint64_t version, size_t pos, const char *content);
int markdown_delete(document *doc, uint64_t version, size_t pos, size_t len);

// === Formatting Commands ===
int markdown_newline(document *doc, size_t version, size_t pos);
int markdown_heading(document *doc, uint64_t version, size_t level, size_t pos);
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

    if(doc->flatten_cache){
        // suppose the outer function will free the content;
        char * result = (char *)malloc(str(doc->flatten_cache)+1);
        strcpy(result, doc->flatten_cache);
        return result;
    }
    if(!COMP9017){

        size_t total_length = 0;
        chunk * current = doc->start_empty_chunk->next;
        while(current && current != doc->end_empty_chunk){
            if(current->content){
                total_length += strlen(current->content);
            }
            current = current ->next;

        }

        char * result = (char * )malloc(total_length +1);
        if(!result) return NULL;

        result[0] = '\0';
        current = doc ->start_empty_chunk ->next;
        while(current && current != doc->end_empty_chunk){
            if(current -> content){
                strcat(result,current->content);
            }
            current = current -> next;
        }
        return result;
    }else{
        // todo 
    }

}

// check COMP9017 here
int markdown_version_check(document *doc ,size_t version){
    size_t current_version = doc->version;
    if(current_version == version) return 2;
    if(current_version == version + 1 && COMP9017) return 1;
    return 0;
}

// === Versioning ===
void markdown_increment_version(document *doc){
    // in comp9017,increment by one version


}

