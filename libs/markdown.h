#ifndef MARKDOWN_H
#define MARKDOWN_H
#include <stdio.h>
#include <stdint.h>
#include "document.h"  
/**
 * The given file contains all the functions you will be required to complete. You are free to and encouraged to create
 * more helper functions to help assist you when creating the document. For the automated marking you can expect unit tests
 * for the following tests, verifying if the document functionalities are correctly implemented. All the commands are explained 
 * in detail in the assignment spec.
 */

// Initialize and free a document
document * markdown_init(void);
void markdown_free(document *doc);

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

int markdown_version_check(document *doc ,size_t version);



// from docuement.h 
chunk * chunk_init(const char * str,chunk* last_chunk, chunk* next_chunk);
void chunk_free(chunk * obj);
int chunk_split(chunk *original_obj, chunk *former_chunk, chunk *next_chunk);
int chunk_split_at(chunk *original_obj, uint64_t pos, chunk *former_chunk, chunk *next_chunk);
chunk *chunk_merge(chunk *start_chunk, chunk *end_chunk);

chunk * find_pos_chunk(document* doc, size_t pos, size_t *left_pos_ptr, int version);
size_t find_logical_index_map(chunk *ck, int version, size_t *map, size_t max_len);

void find_insert_point(chunk *ck, size_t base_index, int direction, chunk **out_chunk, size_t *out_index,int version);
void find_insert_point(chunk *ck, size_t base_index, int direction, chunk **out_chunk, size_t *out_index,int version);


int chunk_insert(chunk* ck, size_t pos, char *content, int version,int direction);
int chunk_change_char(chunk* ck, size_t pos, char ch, int version);
int chunk_mark_delete(chunk* ck, size_t pos, size_t len, int version);

void update_chunk_lengths(chunk* ck);

char * document_serialize(document *doc);



#endif // MARKDOWN_H


