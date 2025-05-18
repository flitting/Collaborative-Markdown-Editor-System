#ifndef DOCUMENT_H

#define DOCUMENT_H
/**
 * This file is the header file for all the document functions. You will be tested on the functions inside markdown.h
 * You are allowed to and encouraged multiple helper functions and data structures, and make your code as modular as possible. 
 * Ensure you DO NOT change the name of document struct.
 */

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include <extension.h>

#define MAX_CHUNK_SIZE 2048


enum STATUS{
    STILL = 1,
    DELETE_OLD,
    INSERT_OLD,
    MODIFIED_OLD,// for ordered list change
    DELETE_NEW,
    INSERT_NEW,
    MODIFIED_NEW,
    DEL_INS,
    DEL_MOD,
    MOD_INS,
};



// a command (especial deletion) should be done in one chunk
typedef struct{
    uint64_t chunk_length;// total length,memory length
    uint64_t chunk_old_length; // effect char length for version N-1,used for quick index
    uint64_t chunk_new_length; // effect char length for version N, used for quick index
    chunk * next;
    chunk * last;
    char * content;// the latest content with all accpected changes, contain deleted chars
    char * status;// consists of STATUS and \0
} chunk;




typedef struct {
    uint64_t version;//current version for last increment, one after the N-1;
    uint64_t chunk_num;
    // content between start and end chunk , version is at N-1 if COMP9017
    chunk * start_empty_chunk;
    chunk * end_empty_chunk;
    const char * flatten_cache;// cache for flatten, content for version N;
} document;

// Functions from here onwards.

chunk * chunk_init(const char * str,chunk* last_chunk, chunk* next_chunk);
void chunk_free(chunk * obj);
int chunk_split(chunk *original_obj, chunk *former_chunk, chunk *next_chunk);
int chunk_split_at(chunk *original_obj, uint64_t pos, chunk *former_chunk, chunk *next_chunk);
chunk *chunk_merge(chunk *start_chunk, chunk *end_chunk);

chunk * find_pos_chunk(document* doc, size_t pos, size_t *left_pos_ptr, int version);
size_t find_logical_index_map(chunk *ck, int version, size_t *map, size_t max_len);
void chunk_insert(chunk* ck, size_t pos, char *content, int version);
void chunk_change_char(chunk* ck, size_t pos, char ch, int version);
int chunk_mark_delete(chunk* ck, size_t pos, size_t len, int version);


#endif
