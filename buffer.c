/*
** Buffer utility.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "head.h"

Buffer* buffer_create(size_t cap) {
    Buffer* x = malloc(sizeof(Buffer));
    x->cap = cap;
    x->siz = 0;
    x->data = malloc(cap);
    return x;
}

void zero_buffer(Buffer* buf) {
    for (int i = 0; i < buf->cap; i++)
        buf->data[i] = 0;
    buf->siz = 0;
}

void buffer_free(Buffer* buf) {
    free(buf->data);
    free(buf);
}

void buffer_expand(Buffer* buf, size_t to) {
    buf->cap = to;
    buf->data = realloc(buf->data, to);
}

void buffer_write(Buffer* buf, size_t location, char CHR) {
    if (buf->cap < location) {
        fprintf(stderr, "buffer.c: Bad write to [%lu].\n", location);
        exit(1);
    }
    
    buf->data[location] = CHR;
    buf->siz++;
}

void buffer_write_long(Buffer *buf, size_t location, char *STR) {
    if (buf->cap < location + strlen(STR)) {
        fprintf(stderr, "buffer.c: Bad write to [%lu].\n", location);
        exit(1);
    }

    for (int i = 0; i < strlen(STR); i++) {
        buffer_write(buf, location+i, STR[i]);
    }
}

char buffer_read(Buffer* buf, size_t location) {
    if (buf->cap < location) {
        fprintf(stderr, "buffer.c: Bad read at [%lu].\n", location);
        exit(1);
    }

    return buf->data[location];
}
