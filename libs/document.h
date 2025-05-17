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

#define MAX_CHUNK_SIZE 2048


// a command (especial deletion) should be done in one chunk
typedef struct{
    uint64_t chunk_length;
    chunk * next;
    chunk * last;
    char * content;
} chunk;

enum CommandType{
    INSERT,
    DEL,
    NEWLINE,
    HEADING,
    BOLD,
    ITALIC,
    BLOCKQUOTE,
    ORDERED_LIST,
    UNORDERED_LIST,
    CODE,
    HORIZONTAL_RULE,
    LINK
};

typedef struct{
    uint64_t version;
    enum CommandType ctype;
    char * content;//content or link
    size_t pos;//pos or pos_start
    size_t pos_end;
    int range;// no_char for del and level for heading
    time_t time;
    int result;

    command_log * next;
}command_log;



typedef struct {
    uint64_t version;
    command_log * logs;
    command_log * last_logs;
    uint64_t chunk_num;
    // content between start and end chunk , version is at N-1 if COMP9017
    chunk * start_empty_chunk;
    chunk * end_empty_chunk;
} document;

// Functions from here onwards.

chunk * chunk_init(const char * str,chunk* last_chunk, chunk* next_chunk);
void chunk_free(chunk * obj);
int chunk_split(chunk *original_obj, chunk *former_chunk, chunk *next_chunk);
int chunk_split_at(chunk *original_obj, uint64_t pos, chunk *former_chunk, chunk *next_chunk);
chunk *chunk_merge(chunk *start_chunk, chunk *end_chunk);

chunk * find_pos_chunk(document* doc, size_t pos,size_t *left_pos);

int markdown_instant_insert(document *doc, size_t pos, const char *content);
int markdown_instant_delete(document *doc, size_t pos, size_t len);


#endif
