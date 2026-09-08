#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

typedef struct {
    uint8_t* buf;
    size_t size;
} ByteArray; 

/*                          API                                 */
/* ------------------------------------------------------------ */

size_t get_file_size(FILE* f);

void print_byte_array(ByteArray* ba);

void cleanup_bytearray(ByteArray **ba);

ByteArray* create_empty_byte_array(size_t size);

ByteArray* clone_byte_array(ByteArray* ba);

ByteArray* file_to_byte_array(const char* filename);

bool byte_array_to_file(ByteArray* ba, const char* filename);

bool insert_zeroes(size_t pos, size_t amt, ByteArray** ba);

bool put_int32_le(ByteArray* b, int32_t value, unsigned int idx);

bool put_int16_le(ByteArray* b, int16_t value, unsigned int idx);

int32_t get_int32_le(ByteArray* b, unsigned int idx);

int16_t get_int16_le(ByteArray* b, unsigned int idx);

bool put_int32_be(ByteArray* b, int32_t value, unsigned int idx);

bool put_int16_be(ByteArray* b, int16_t value, unsigned int idx);

int32_t get_int32_be(ByteArray* b, unsigned int idx);

int16_t get_int16_be(ByteArray* b, unsigned int idx);

#ifdef __cplusplus
}
#endif

/* -------------------------------------------------------------- */

