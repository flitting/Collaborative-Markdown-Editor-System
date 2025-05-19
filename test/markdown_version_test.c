#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../libs/markdown.h"

// Helper to print current flattened content and version.
void print_flatten_and_version(document *doc) {
    char *flat = markdown_flatten(doc);
    printf("Version %ld: \"%s\"\n", doc->version, flat);
    free(flat);
}

int main(void) {
    int errors = 0;
    document *doc = markdown_init();

    // Set initial version.
    markdown_increment_version(doc); // version becomes 1, content: ""

    // Test 1: Valid insertion using current version.
    // Insert "Hello" at position 0 using version = current (1).
    int res = markdown_insert(doc, doc->version, 0, "Hello");
    if (res == 0) {
        markdown_increment_version(doc); // version becomes 2, content becomes "Hello"
        print_flatten_and_version(doc);  // Expect: "Hello"
    } else {
        printf("Fail: Insertion with current version returned error %d\n", res);
        errors++;
    }

    // Test 2: Valid insertion using outdated version.
    // Use cursor (position 0) obtained from version = doc->version - 1 (version 1).
    // In version 1 the document was empty, so the new text "Start: " should be prepended.
    res = markdown_insert(doc, doc->version - 1, 0, "Start: ");
    if (res == 0) {
        markdown_increment_version(doc); // version becomes 3, expected content: "Start: Hello"
        char *flat = markdown_flatten(doc);
        if (strcmp(flat, "Start: Hello") == 0)
            printf("Pass: Outdated version insertion succeeded: \"%s\"\n", flat);
        else {
            printf("Fail: Outdated version insertion expected \"Start: Hello\", got \"%s\"\n", flat);
            errors++;
        }
        free(flat);
    } else {
        printf("Fail: Outdated version insertion returned error %d\n", res);
        errors++;
    }

    // Test 3: Valid deletion using outdated version.
    // Use a deletion operation based on version = doc->version - 1 (version 2).

    int outdated_version = doc->version - 1;
    res = markdown_delete(doc, outdated_version, 0, 1);
    if (res == 0) {
        markdown_increment_version(doc); // version becomes 4, expected content: "Start: ello"
        char *flat = markdown_flatten(doc);
        if (strcmp(flat, "tart: Hello") == 0)
            printf("Pass: Outdated version deletion succeeded: \"%s\"\n", flat);
        else {
            printf("Fail: Outdated version deletion expected \"tart: Hello\", got \"%s\"\n", flat);
            errors++;
        }
        free(flat);
    } else {
        printf("Fail: Outdated version deletion returned error %d\n", res);
        errors++;
    }


    // Final summary.
    if (errors == 0)
        printf("Pass: All version editing tests passed.\n");
    else
        printf("Fail: %d version editing test(s) failed.\n", errors);

    markdown_free(doc);
    return 0;
}