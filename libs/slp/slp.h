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
  SLP_TYPE_LIST_S,  // ""
  SLP_TYPE_QUOTED,  // ' prefixed
  SLP_TYPE_BUILTIN, // A C function
  SLP_TYPE_LAMBDA,  // A lambda function
  SLP_TYPE_ERROR,   // An error object (can be handled by the runtime)
} slp_type_e;

typedef enum slp_error_type_e {
  SLP_ERROR_UNCLOSED_GROUP = 0,
  SLP_ERROR_UNCLOSED_QUOTED_GROUP,
  SLP_ERROR_PARSE_QUOTED_TOKEN,
  SLP_ERROR_PARSE_TOKEN,
  SLP_ERROR_ALLOCATION,
  SLP_ERROR_BUFFER_OPERATION,
} slp_error_type_e;

typedef struct slp_error_data_s {
  size_t position;
  slp_error_type_e error_type;
  char *message;
  slp_buffer_unowned_ptr_t source_buffer;
} slp_error_data_t;

/*

*/
typedef struct slp_object_s {
  slp_type_e type;
  size_t source_position;
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
  void (*on_virtual_list_start)(void *context);
  void (*on_virtual_list_end)(void *context);
  void (*on_error)(slp_error_type_e error_type, const char *message,
                   size_t position, slp_buffer_t *buffer, void *context);
  void *context;
} slp_callbacks_t;

typedef void (*slp_fn_data_free_fn)(void *fn_data);
typedef void *(*slp_fn_data_copy_fn)(void *fn_data);
typedef bool (*slp_fn_data_equal_fn)(void *fn_data_a, void *fn_data_b);

void slp_register_builtin_handlers(slp_fn_data_free_fn free_fn,
                                   slp_fn_data_copy_fn copy_fn);
void slp_register_lambda_handlers(slp_fn_data_free_fn free_fn,
                                  slp_fn_data_copy_fn copy_fn,
                                  slp_fn_data_equal_fn equal_fn);

void slp_free_object(slp_object_t *object);
slp_object_t *slp_object_copy(slp_object_t *object);
bool slp_objects_equal(slp_object_t *a, slp_object_t *b);

/*

*/
typedef struct slp_processor_state_s {
  size_t tokens_processed;
  size_t errors;
  int virtual_paren_active;
  int current_depth;
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