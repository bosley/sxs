#ifndef SLP_SCANNER_H
#define SLP_SCANNER_H

#include "buffer/buffer.h"
#include "types/types.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct slp_scanner_s {
  slp_buffer_t *buffer;
  size_t position;
} slp_scanner_t;

slp_scanner_t *slp_scanner_new(slp_buffer_t *buffer, size_t position);

void slp_scanner_free(slp_scanner_t *scanner);

typedef struct slp_scanner_static_type_result_s {
  bool success;
  size_t start_position;
  slp_static_type_t data;
} slp_scanner_static_type_result_t;

// if read fails position unchanged.
// scanner uses current position to attempt reading a static base type
// off the buffer. if it fails to do so the result will indicate the
// where and why
slp_scanner_static_type_result_t
slp_scanner_read_static_base_type(slp_scanner_t *scanner);

#endif