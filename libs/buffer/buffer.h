#ifndef SXS_SLP_BUFFER_H
#define SXS_SLP_BUFFER_H

#include <stddef.h>
#include <stdint.h>

typedef struct slp_buffer_s {
  uint8_t *data;
  size_t capacity;
  size_t count;
} slp_buffer_t;

typedef struct {
  slp_buffer_t *left;
  slp_buffer_t *right;
} split_buffer_t;

typedef int (*slp_buffer_iterator_fn)(uint8_t *byte, size_t idx);

slp_buffer_t *slp_buffer_create(size_t initial_size);

slp_buffer_t *slp_buffer_from_file(const char *filepath);

void slp_buffer_destroy(slp_buffer_t *buffer);

int slp_buffer_copy_to(slp_buffer_t *buffer, uint8_t *src, size_t len);

size_t slp_buffer_count(slp_buffer_t *buffer);

uint8_t *slp_buffer_data(slp_buffer_t *buffer);

void slp_buffer_clear(slp_buffer_t *buffer);

int slp_buffer_shrink_to_fit(slp_buffer_t *buffer);

void slp_buffer_for_each(slp_buffer_t *buffer, slp_buffer_iterator_fn fn);

slp_buffer_t *slp_buffer_sub_buffer(slp_buffer_t *buffer, size_t offset,
                                    size_t length, int *bytes_copied);

void slp_buffer_rotate_left(slp_buffer_t *buffer, size_t n);

void slp_buffer_rotate_right(slp_buffer_t *buffer, size_t n);

int slp_buffer_trim_left(slp_buffer_t *buffer, uint8_t byte);

int slp_buffer_trim_right(slp_buffer_t *buffer, uint8_t byte);

slp_buffer_t *slp_buffer_copy(slp_buffer_t *buffer);

split_buffer_t slp_buffer_split(slp_buffer_t *buffer, size_t index, size_t l,
                                size_t r);

void slp_split_buffer_destroy(split_buffer_t *split);

#endif
