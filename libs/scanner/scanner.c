#include "scanner.h"
#include <stdlib.h>

slp_scanner_t *slp_scanner_new(slp_buffer_t *buffer, size_t position) {
  if (!buffer) {
    return NULL;
  }

  if (position > buffer->count) {
    return NULL;
  }

  slp_scanner_t *scanner = malloc(sizeof(slp_scanner_t));
  if (!scanner) {
    return NULL;
  }

  scanner->buffer = buffer;
  scanner->position = position;

  return scanner;
}

void slp_scanner_free(slp_scanner_t *scanner) {
  if (!scanner) {
    return;
  }

  free(scanner);
}

slp_scanner_static_type_result_t
slp_scanner_read_static_base_type(slp_scanner_t *scanner) {
  if (!scanner) {
    return (slp_scanner_static_type_result_t){
        .success = false,
        .start_position = 0,
        .data = {.base = SLP_STATIC_BASE_NONE, .data = {.none = NULL}}};
  }

  return (slp_scanner_static_type_result_t){
      .success = true,
      .start_position = scanner->position,
      .data = {.base = SLP_STATIC_BASE_NONE, .data = {.none = NULL}}};
}