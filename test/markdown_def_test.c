#include "../libs/markdown.h"

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

int main(){
    document * doc = markdown_init();
    markdown_insert(doc,0,0,"Hello,World.");
    markdown_increment_version(doc);
    markdown_delete(doc,1,0,20);
    print_chunk_memory(doc->start_empty_chunk->next);
    markdown_insert(doc,1,2,"bar");
    print_chunk_memory(doc->start_empty_chunk->next);
    markdown_insert(doc,1,1,"Foo");
    print_chunk_memory(doc->start_empty_chunk->next);
    markdown_increment_version(doc);
    markdown_print(doc,stdout);
    markdown_free(doc);

}