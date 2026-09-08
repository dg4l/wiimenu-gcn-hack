#include "bytearray.h"

// recent changes (as of 2025-11-27) have
// slightly changed the way this library handles errors.
// I decided that rather than just leaving it up to the
// user of this library to figure out what the problem is
// I have decided to include some more early returns,
// so that when you have an issue, you can place a breakpoint
// on the function that is giving you trouble, and see exactly where
// something went wrong, this is especially helpful when multiple things
// could go wrong.

bool insert_zeroes(size_t pos, size_t amt, ByteArray** ba) {
    if (!ba || !*ba) return false;
    if (pos > (*ba)->size) return false;
    ByteArray* tmp = create_empty_byte_array((*ba)->size + amt);
    if (!tmp) return false;
    memcpy(tmp->buf, (*ba)->buf, pos);
    memset(&tmp->buf[pos], 0, amt);
    if (pos < (*ba)->size) memcpy(&tmp->buf[pos + amt], &(*ba)->buf[pos], (*ba)->size - pos);
    cleanup_bytearray(ba);
    *ba = tmp;
    return true;
}

size_t get_file_size(FILE* f) {
    struct stat stats;
    int fd = fileno(f); 
    if (fd == -1) return 0;
    if (fstat(fd, &stats) == -1) return 0;
    return stats.st_size;
}

void print_byte_array(ByteArray* ba) {
    if (!ba) {
        fprintf(stderr, "argument to print_byte_array is NULL!!!\n");
        return;
    }
    for (size_t i = 0; i < ba->size; ++i) {
        if (!(i % 16) && i) printf("\n");
        printf("%02X ", ba->buf[i]);
    }
    printf("\n");
}

void cleanup_bytearray(ByteArray** ba) {
    free((*ba)->buf);
    free(*ba);
    *ba = NULL;
}

ByteArray* clone_byte_array(ByteArray* ba) {
    if (!ba) return NULL;
    ByteArray* ret = create_empty_byte_array(ba->size);
    if (!ret) return NULL;
    memcpy(ret->buf, ba->buf, ba->size);
    return ret;
}

ByteArray* create_empty_byte_array(size_t size) {
    ByteArray* b = malloc(sizeof(ByteArray));
    if (!b) {
        return NULL;
    }
    b->size = size;
    b->buf = calloc(b->size, 1);
    if (!b->buf) {
        free(b);
        return NULL;
    }
    return b;
}

bool byte_array_to_file(ByteArray* ba, const char* filename) {
    if (!ba) return false;
    FILE* fp = fopen(filename, "wb");
    if (!fp) return false;
    fwrite(ba->buf, 1, ba->size, fp);
    fclose(fp);
    return true;
}

ByteArray* file_to_byte_array(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) return NULL;
    size_t file_size = get_file_size(f);
    if (!file_size) {
        fclose(f);
        return NULL;
    }
    ByteArray* b = create_empty_byte_array(file_size);
    if (!b) {
        fclose(f);
        return NULL;
    }
    fread(b->buf, 1, b->size, f);
    fclose(f);
    return b;
}

// general helpers I created while working on something else,
// thought they would be useful in the library.
bool put_int32_le(ByteArray* b, int32_t value, unsigned int idx) {
    if (!b) return false;
    if (idx + 4 > b->size) return false;
    for (size_t i = 0; i < 4; ++i) {
        b->buf[idx + i] = (value >> (i * 8)) & 0xFF;
    }
    return true;
}

bool put_int16_le(ByteArray* b, int16_t value, unsigned int idx) {
    if (!b) return false;
    if (idx + 2 > b->size) return false;
    for (size_t i = 0; i < 2; ++i) {
        b->buf[idx + i] = (value >> (i * 8)) & 0xFF;
    }
    return true;
}

int32_t get_int32_le(ByteArray* b, unsigned int idx) {
    if (!b) return -1;
    if (idx + 4 > b->size) return -1;
    return b->buf[idx + 3] << 24 | b->buf[idx + 2] << 16 | b->buf[idx + 1] << 8 | b->buf[idx];
}

int16_t get_int16_le(ByteArray* b, unsigned int idx) {
    if (!b) return -1;
    if (idx + 2 > b->size) return -1;
    return b->buf[idx + 1] << 8 | b->buf[idx];
}

bool put_int32_be(ByteArray* b, int32_t value, unsigned int idx) {
    if (!b) return false;
    if (idx + 4 > b->size) return false;
    for (size_t i = 0; i < 4; ++i) {
        b->buf[idx + i] = (value >> 24 - ((i * 8))) & 0xFF;
    }
    return true;
}

bool put_int16_be(ByteArray* b, int16_t value, unsigned int idx) {
    if (!b) return false;
    if (idx + 4 > b->size) return false;
    for (size_t i = 0; i < 2; ++i) {
        b->buf[idx + i] = (value >> 8 - ((i * 8))) & 0xFF;
    }
    return true;
}

int16_t get_int16_be(ByteArray* b, unsigned int idx) {
    if (!b) return -1;
    if (idx + 2 > b->size) return -1;
    return b->buf[idx] << 8 | b->buf[idx + 1];
}

int32_t get_int32_be(ByteArray* b, unsigned int idx) {
    if (!b) return -1;
    if (idx + 4 > b->size) return -1;
    return b->buf[idx] << 24 | b->buf[idx + 1] << 16 | b->buf[idx + 2] << 8 | b->buf[idx + 3];
}
