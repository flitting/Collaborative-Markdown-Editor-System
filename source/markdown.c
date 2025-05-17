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
    doc->logs = NULL;
    doc->last_logs = NULL;




    return doc;
}
void markdown_free(document *doc){
    command_log * current = doc->logs;
    while(current != NULL){
        command_log * next = current->next;
        free(current);
        current = next;
    }

    chunk * current = doc->start_empty_chunk;
    while(current->next != doc->end_empty_chunk){
        chunk_free(current->next);
    }
    chunk_free(doc->start_empty_chunk);
    chunk_free(doc->end_empty_chunk);
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
void markdown_print(const document *doc, FILE *stream);
char *markdown_flatten(const document *doc);

// === Versioning ===
void markdown_increment_version(document *doc);
