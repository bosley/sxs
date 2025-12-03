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
  size_t error_position;
  slp_static_type_t data;
} slp_scanner_static_type_result_t;

typedef struct slp_scanner_stop_symbols_s {
  uint8_t *symbols;
  size_t count;
} slp_scanner_stop_symbols_t;

/*
reads from buffer:

  ints (with sign)
  real
  symbols

Terminates by default on all whitespace. 
Optionally accepts additional stop symbols that will terminate parsing
without consuming the stop character. stop_symbols may be NULL.
Stop symbols must NOT include '.', '+', or '-'.

Failure to read an int, real, r symbol will result in an error - indicated in the result
Success will automatically move the position of the scanner
*/
slp_scanner_static_type_result_t
slp_scanner_read_static_base_type(slp_scanner_t *scanner,
                                   slp_scanner_stop_symbols_t *stop_symbols);

#endif