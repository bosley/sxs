#ifndef SXS_SLP_SLP_H
#define SXS_SLP_SLP_H

#include "buffer/buffer.h"
#include "scanner/scanner.h"
#include <stdint.h>
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

/*

*/
typedef struct slp_object_s {
  slp_type_e type;
  union {
    int64_t integer;
    double real;
    slp_buffer_t *buffer;
    struct {
      struct slp_object_s **items;
      size_t count;
    } list;
    void *fn_data; // depends on type for fn data (lambda v builtin)
  } value;
} slp_object_t;

typedef void (*slp_object_consumer_fn)(slp_object_t *object, void *context);

typedef struct slp_callbacks_s {
  void (*on_object)(slp_object_t *object, void *context);
  void (*on_list_start)(slp_type_e list_type, void *context);
  void (*on_list_end)(slp_type_e list_type, void *context);
  void *context;
} slp_callbacks_t;

void slp_free_object(slp_object_t *object);
slp_object_t *slp_object_copy(slp_object_t *object);

/*

*/
typedef struct slp_processor_state_s {
  size_t tokens_processed;
  size_t errors;
} slp_processor_state_t;

void slp_process_group(slp_scanner_t *scanner, uint8_t start, uint8_t end,
                       const char *group_name, slp_type_e list_type,
                       slp_processor_state_t *state,
                       slp_scanner_stop_symbols_t *stops, int depth,
                       slp_callbacks_t *callbacks);
void slp_process_tokens(slp_scanner_t *scanner, slp_processor_state_t *state,
                        slp_scanner_stop_symbols_t *stops, int depth,
                        slp_callbacks_t *callbacks);
int slp_process_buffer(slp_buffer_t *buffer, slp_callbacks_t *callbacks);
int slp_process_file(char *file_name, slp_callbacks_t *callbacks);

#endif