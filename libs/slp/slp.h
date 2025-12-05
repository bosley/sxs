#ifndef SXS_SLP_SLP_H
#define SXS_SLP_SLP_H

#define SLP_PROCESSOR_COUNT 16 // for consistence . dont change
#define SLP_REGISTER_COUNT 32  // for consistence . dont change

#include "buffer/buffer.h"
#include "scanner/scanner.h"
#include "types/types.h"
#include <stdio.h>
#include <string.h>

typedef enum slp_type_e {
  SLP_TYPE_NONE = 0,
  SLP_TYPE_INTEGER,
  SLP_TYPE_REAL,
  SLP_TYPE_SYMBOL,
  SLP_TYPE_LIST_P,  // ()
  SLP_TYPE_LIST_C,  // {}
  SLP_TYPE_LIST_B,  // []
  SLP_TYPE_LIST_Q,  // ''
  SLP_TYPE_LIST_S,  // ""
  SLP_TYPE_QUOTED,  // ' prefixed
  SLP_TYPE_BUILTIN, // A C function
  SLP_TYPE_LAMBDA,  // A lambda function
} slp_type_e;

typedef struct slp_cell_s {

} slp_type_t;

typedef struct slp_processor_state_s {
  size_t tokens_processed;
  size_t errors;
} slp_processor_state_t;

void slp_print_static_type(slp_static_type_t *type, int depth);
void slp_process_group(slp_scanner_t *scanner, uint8_t start, uint8_t end,
                       const char *group_name, slp_processor_state_t *state,
                       slp_scanner_stop_symbols_t *stops, int depth);
void slp_process_tokens(slp_scanner_t *scanner, slp_processor_state_t *state,
                        slp_scanner_stop_symbols_t *stops, int depth);
int slp_process_buffer(slp_buffer_t *buffer);
int slp_process_file(char *file_name);

#endif