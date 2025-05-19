#include <stdio.h>
#include <string.h>
#include "../libs/markdown.h"

void print_chunk_memory(chunk* obj_chunk){
    printf("content: %s\n",obj_chunk->content);
    char result[strlen(obj_chunk->status)+1];
    for(int i = 0;i<strlen(obj_chunk->status);i++){
        result[i] = obj_chunk->status[i]+'0';
    }
    result[strlen(obj_chunk->status)] = '\0';
    printf("status: %s\n",result);

}

int main(void) {
    // Step 1: Initialize Document
    document *doc = markdown_init();
    markdown_increment_version(doc);
    char *initial = markdown_flatten(doc);
    printf("Initial flatten: '%s'\n", initial);
    free(initial);
    // Step 2: Test first insertion
    const char *content1 = "Hello ";
    int ins_result1 = markdown_insert(doc, doc->version, 0, content1);
    if (ins_result1 != 0) {
        printf("Insertion 1 failed with error code: %d\n", ins_result1);
    }
    markdown_increment_version(doc);

    char *after_first_insert = markdown_flatten(doc);
    printf("After first insert: '%s'\n", after_first_insert);
    free(after_first_insert);

    // Step 3: Test second insertion
    const char *content2 = "World!";
    // Insert at position 6 after "Hello "
    int ins_result2 = markdown_insert(doc, doc->version, 6, content2);
    if (ins_result2 != 0) {
        printf("Insertion 2 failed with error code: %d\n", ins_result2);
    }
    markdown_increment_version(doc);

    char *after_second_insert = markdown_flatten(doc);
    printf("After second insert: '%s'\n", after_second_insert);
    free(after_second_insert);

    // Step 4: Test newline insertion (formatting command)
    // Insert a newline (i.e. "\n") at position 5
    int newline_result = markdown_newline(doc, doc->version, 5);
    if (newline_result != 0) {
        printf("Newline insertion failed with error code: %d\n", newline_result);
    }
    markdown_increment_version(doc);

    char *after_newline = markdown_flatten(doc);
    printf("After newline insertion: '%s'\n", after_newline);
    free(after_newline);

    // Step 5: Test deletion command, delete 3 characters from position 0
    int delete_result = markdown_delete(doc, doc->version, 0, 3);
    if (delete_result < 0) {
        printf("Deletion failed with error code: %d\n", delete_result);
    }else{
        markdown_increment_version(doc);

        char *after_deletion = markdown_flatten(doc);
        printf("After deletion: '%s'\n", after_deletion);
        chunk * obj_chunk = doc->start_empty_chunk->next;
        print_chunk_memory(obj_chunk);

        free(after_deletion);
    }
        // Step 6: Try using previous version for insertion
    const char *content3 = "TestOldVersion ";
    int ins_old_version = markdown_insert(doc, doc->version-1, 0, content3);
    if (ins_old_version != 0) {
        printf("Old version insertion failed with error code: %d\n", ins_old_version);
    } else {
        markdown_increment_version(doc);
        char *after_old_insert = markdown_flatten(doc);
        printf("After insertion with version-1: '%s'\n", after_old_insert);
        chunk * obj_chunk = doc->start_empty_chunk->next;
        print_chunk_memory(obj_chunk);

        free(after_old_insert);
    }

    // Step 7: Try newline using version-1
    int newline_old_version = markdown_newline(doc, doc->version -1, 2);//wrong
    if (newline_old_version != 0) {
        printf("Old version newline failed with error code: %d\n", newline_old_version);
    } else {
        markdown_increment_version(doc);
        char *after_old_newline = markdown_flatten(doc);
        printf("After newline with version-1: '%s'\n", after_old_newline);

        chunk * obj_chunk = doc->start_empty_chunk->next;
        print_chunk_memory(obj_chunk);

        free(after_old_newline);
    }

    // Step 8: Try delete using version-1
    int del_old_version = markdown_delete(doc, doc->version - 1, 0, 5);
    if (del_old_version < 0) {
        printf("Old version deletion failed with error code: %d\n", del_old_version);
    } else {
        markdown_increment_version(doc);
        char *after_old_delete = markdown_flatten(doc);
        printf("After delete with version-1: '%s'\n", after_old_delete);// wrong?
        free(after_old_delete);
    }

    // Step 9: Try invalid insert (position too large)
    const char *invalid_content = "Invalid";
    int invalid_insert = markdown_insert(doc, doc->version, 1000, invalid_content);
    if (invalid_insert != 0) {
        printf("Invalid insertion correctly failed with error code: %d\n", invalid_insert);
    }

    // Step 10: Try invalid delete (length too long)
    int delete_overflow_test = markdown_delete(doc, doc->version, 0, 1000);
    if (delete_overflow_test < 0) {
        printf("deletion failed with error code: %d\n", delete_overflow_test);
    }else{
        markdown_increment_version(doc);
        char *overflow_delete = markdown_flatten(doc);
        printf("delete_overflow: '%s'\n", overflow_delete);//
        free(overflow_delete);
    }

    // Step 11: Try newline at invalid position 
    // it should return -3
    int invalid_newline = markdown_newline(doc, doc->version, 999);
    if (invalid_newline != 0) {
        printf("Invalid newline insertion correctly failed with error code: %d\n", invalid_newline);
    }


    // Cleanup
    markdown_free(doc);
    return 0;
}