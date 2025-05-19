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
    
    // Test 2: Append Insertion (at end)
    // Current flatten: "Hello" (length=5) -> Insert at pos 5.
    const char *test2 = " World";
    res = markdown_insert(doc, doc->version, 5, test2);
    if (res == 0) {
        markdown_increment_version(doc);
        flat = markdown_flatten(doc);
        if (strcmp(flat, "Hello World") == 0)
            printf("Pass: Append insertion [Appended \" World\"]\n");
        else {
            printf("Fail: Append insertion expected \"Hello World\", got \"%s\"\n", flat);
            print_all_chunks(doc);
            errors++;
        }
        free(flat);
    } else {
        printf("Fail: Append insertion returned error %d\n", res);
        print_all_chunks(doc);
        errors++;
    }
    
    // Test 3: Insertion in the middle.
    // Insert " dear" at pos 5 -> Expected: "Hello dear World"
    const char *test3 = " dear";
    res = markdown_insert(doc, doc->version, 5, test3);
    if (res == 0) {
        markdown_increment_version(doc);
        flat = markdown_flatten(doc);
        if (strcmp(flat, "Hello dear World") == 0)
            printf("Pass: Insertion in the middle [Inserted \" dear\" at pos 5]\n");
        else {
            printf("Fail: Middle insertion expected \"Hello dear World\", got \"%s\"\n", flat);
            print_all_chunks(doc);
            errors++;
        }
        free(flat);
    } else {
        printf("Fail: Insertion in the middle returned error %d\n", res);
        print_all_chunks(doc);
        errors++;
    }
    
    // Test 4: Deletion in the middle.
    // Delete " dear" (5 chars) starting at pos 5 -> Expected: "Hello World"
    res = markdown_delete(doc, doc->version, 5, 5);
    if (res == 0) {
        markdown_increment_version(doc);
        flat = markdown_flatten(doc);
        if (strcmp(flat, "Hello World") == 0)
            printf("Pass: Deletion in middle [Deleted \" dear\"]\n");
        else {
            printf("Fail: Deletion expected \"Hello World\", got \"%s\"\n", flat);
            print_all_chunks(doc);
            errors++;
        }
        free(flat);
    } else {
        printf("Fail: Deletion in middle returned error %d\n", res);
        print_all_chunks(doc);
        errors++;
    }
    
    // Test 5: Invalid insertion (position too large).
    const char *test_invalid = "Invalid";
    int invalid_insert = markdown_insert(doc, doc->version, 1000, test_invalid);
    if (invalid_insert != 0)
        printf("Pass: Invalid insertion correctly failed with error code: %d [Pos 1000 out-of-bound]\n", invalid_insert);
    else {
        printf("Fail: Invalid insertion expected failure but returned success\n");
        print_all_chunks(doc);
        errors++;
    }
    
    // Test 6: Invalid deletion (range too long).
    int invalid_delete = markdown_delete(doc, doc->version, 0, 1000);
    if (invalid_delete >= 0)
        printf("Pass: Invalid deletion correctly succeeded [Range too long handled gracefully]\n");
    else {
        printf("Fail: Invalid deletion expected success but returned error code: %d\n", invalid_delete);
        print_all_chunks(doc);
        errors++;
    }

    const char *undo = "Hello World";
    markdown_insert(doc, doc->version, 0, undo);
    
    // Test 7: Newline insertion at beginning.
    // Current flatten: "Hello World" -> Expected: "\\nHello World"
    res = markdown_newline(doc, doc->version, 0);
    if (res == 0) {
        markdown_increment_version(doc);
        flat = markdown_flatten(doc);
        if (strcmp(flat, "\\nHello World") == 0)
            printf("Pass: Newline insertion at beginning [Inserted newline at pos 0]\n");
        else {
            printf("Fail: Newline insertion (beginning) expected \"\\nHello World\", got \"%s\"\n", flat);
            print_all_chunks(doc);
            errors++;
        }
        free(flat);
    } else {
        printf("Fail: Newline insertion at beginning returned error %d\n", res);
        print_all_chunks(doc);
        errors++;
    }
    
    // Test 8: Newline insertion at end.
    // Current flatten: "\\nHello World" (length 12) -> Expected: "\\nHello World\\n"
    res = markdown_newline(doc, doc->version, 13);
    if (res == 0) {
        markdown_increment_version(doc);
        flat = markdown_flatten(doc);
        if (strcmp(flat, "\\nHello World\\n") == 0)
            printf("Pass: Newline insertion at end [Inserted newline at pos 12]\n");
        else {
            printf("Fail: Newline insertion (end) expected \"\\nHello World\\n\", got \"%s\"\n", flat);
            print_all_chunks(doc);
            errors++;
        }
        free(flat);
    } else {
        printf("Fail: Newline insertion at end returned error %d\n", res);
        print_all_chunks(doc);
        errors++;
    }
    
    // Test 9: Formatting across chunk boundaries.
    // Create a new document to stress test chunk_insert skipping deletion chars across chunks.
    markdown_free(doc);
    doc = markdown_init();
    markdown_increment_version(doc);
    // Insert "ABCDEFG"
    const char *str_val = "ABCDEFG";
    res = markdown_insert(doc, doc->version, 0, str_val);
    if (res == 0) {
        markdown_increment_version(doc);
        flat = markdown_flatten(doc);
        if (strcmp(flat, "ABCDEFG") == 0)
            printf("Pass: Insertion for multi-chunk test [Inserted \"ABCDEFG\"]\n");
        else {
            printf("Fail: Multi-chunk insertion expected \"ABCDEFG\", got \"%s\"\n", flat);
            print_all_chunks(doc);
            errors++;
        }
        free(flat);
    } else {
        printf("Fail: Multi-chunk insertion returned error %d\n", res);
        print_all_chunks(doc);
        errors++;
    }
    // Delete "BCD" (from pos 1, len 3) -> Expected: "AEFG"
    res = markdown_delete(doc, doc->version, 1, 3);
    if (res == 0) {
        markdown_increment_version(doc);
        flat = markdown_flatten(doc);
        if (strcmp(flat, "AEFG") == 0)
            printf("Pass: Deletion across chunks [Deleted \"BCD\"]\n");
        else {
            printf("Fail: Deletion across chunks expected \"AEFG\", got \"%s\"\n", flat);
            print_all_chunks(doc);
            errors++;
        }
        free(flat);
    } else {
        printf("Fail: Deletion across chunks returned error %d\n", res);
        print_all_chunks(doc);
        errors++;
    }
    // Insert "123" at pos 1 -> Expected: "A123EFG"
    res = markdown_insert(doc, doc->version, 1, "123");
    if (res == 0) {
        markdown_increment_version(doc);
        flat = markdown_flatten(doc);
        if (strcmp(flat, "A123EFG") == 0)
            printf("Pass: Insertion after deletion across chunks [Inserted \"123\" at pos 1]\n");
        else {
            printf("Fail: Insertion after deletion expected \"A123EFG\", got \"%s\"\n", flat);
            print_all_chunks(doc);
            errors++;
        }
        free(flat);
    } else {
        printf("Fail: Insertion after deletion across chunks returned error %d\n", res);
        print_all_chunks(doc);
        errors++;
    }
    
    // Test 10: Versioning test.
    // Attempt an operation with an outdated version; expect failure.
    res = markdown_insert(doc, doc->version - 2, 0, "Outdated ");
    if (res != 0)
        printf("Pass: Outdated version insertion correctly failed with error code: %d [Version check]\n", res);
    else {
        printf("Fail: Outdated version insertion expected failure but returned success\n");
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