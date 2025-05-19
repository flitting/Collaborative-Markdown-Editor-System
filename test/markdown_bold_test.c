#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../libs/markdown.h"

// Helper to print a chunk's memory details
void print_chunk_memory(chunk* obj_chunk) {
    if (!obj_chunk) return;
    printf("  content: \"%s\"\n", obj_chunk->content);
    size_t len = strlen(obj_chunk->status);
    char result[len + 1];
    for (size_t i = 0; i < len; i++) {
        result[i] = obj_chunk->status[i] + '0';
    }
    result[len] = '\0';
    printf("  status: \"%s\"\n", result);
}

// Print all chunks for detailed inspection
void print_all_chunks(document *doc) {
    printf("=== CHUNK DETAILS ===\n");
    chunk *cur = doc->start_empty_chunk;
    while (cur) {
        print_chunk_memory(cur);
        cur = cur->next;
    }
    printf("=====================\n");
}

int main(void) {
    int errors = 0;
    document *doc = markdown_init();
    
    // Increment to set an initial (empty) version.
    markdown_increment_version(doc);
    char *flat = markdown_flatten(doc);
    if (strcmp(flat, "") == 0)
        printf("Pass: Initial flatten empty [Expected empty]\n");
    else {
        printf("Fail: Initial flatten expected empty, got \"%s\"\n", flat);
        print_all_chunks(doc);
        errors++;
    }
    free(flat);
    
    // Test 1: Basic Insertion at beginning.
    const char *test1 = "Hello";
    int res = markdown_insert(doc, doc->version, 0, test1);
    if (res == 0) {
        markdown_increment_version(doc);
        flat = markdown_flatten(doc);
        if (strcmp(flat, "Hello") == 0)
            printf("Pass: Insert at beginning [Inserted \"Hello\"]\n");
        else {
            printf("Fail: Insert at beginning expected \"Hello\", got \"%s\"\n", flat);
            print_all_chunks(doc);
            errors++;
        }
        free(flat);
    } else {
        printf("Fail: Insertion at beginning returned error %d\n", res);
        print_all_chunks(doc);
        errors++;
    }
    


    // Test 2: bold
    res = markdown_bold(doc,doc->version,0,5);
    if (res == 0) {
        markdown_increment_version(doc);
        flat = markdown_flatten(doc);
        if (strcmp(flat, "**Hello**") == 0)
            printf("Pass: bold effect[%s]\n",flat);
        else {
            printf("Fail: bold error \"**Hello**\", got \"%s\"\n", flat);
            print_all_chunks(doc);
            errors++;
        }
        free(flat);
    } else if(res == -6){
        printf("uncoverable error -6!\n");
        print_all_chunks(doc);
        return -1;
    }else{
        printf("Fail: bold error returned error %d\n", res);
        print_all_chunks(doc);
        errors++;
    }

    // Final summary.
    if (errors == 0)
        printf("Pass: All tests passed.\n");
    else
        printf("Fail: Total %d tests failed.\n", errors);
    
    markdown_free(doc);
    return 0;
}