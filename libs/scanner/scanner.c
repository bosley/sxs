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

static bool is_whitespace(uint8_t c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static bool is_digit(uint8_t c) { return c >= '0' && c <= '9'; }

static bool is_stop_symbol(uint8_t c,
                           slp_scanner_stop_symbols_t *stop_symbols) {
  if (!stop_symbols || !stop_symbols->symbols) {
    return false;
  }
  for (size_t i = 0; i < stop_symbols->count; i++) {
    if (stop_symbols->symbols[i] == c) {
      return true;
    }
  }
  return false;
}

/*
============================================================================================================
This is the static type parser that yeets off of the buffer
it follows a simple state machine and only parses the most primitive types
"static base types" that represent some "thing" that does not have an "inner"
(conceptually) For instance: an integer absolutly has "bits" but if we consider
it from the mindset of "physics" these types would be like atoms. The bits are
there sure, but thats a different "scale" In classic lisps these are called
atoms, but we aren't necessarily parsing a lisp here so i wanted to stay away
from the terminology. Esepcially since AI have been doing docs and tests for me
- its just easier
============================================================================================================
*/

slp_scanner_static_type_result_t
slp_scanner_read_static_base_type(slp_scanner_t *scanner,
                                  slp_scanner_stop_symbols_t *stop_symbols) {
  if (!scanner) {
    return (slp_scanner_static_type_result_t){
        .success = false,
        .start_position = 0,
        .error_position = 0,
        .data = {.base = SLP_STATIC_BASE_NONE, .data = NULL, .byte_length = 0}};
  }

  size_t start_pos = scanner->position;
  size_t pos = start_pos;
  slp_buffer_t *buf = scanner->buffer;

  while (pos < buf->count && is_whitespace(buf->data[pos])) {
    pos++;
  }

  if (pos >= buf->count) {
    return (slp_scanner_static_type_result_t){
        .success = false,
        .start_position = start_pos,
        .error_position = pos,
        .data = {.base = SLP_STATIC_BASE_NONE, .data = NULL, .byte_length = 0}};
  }

  if (is_stop_symbol(buf->data[pos], stop_symbols)) {
    return (slp_scanner_static_type_result_t){
        .success = false,
        .start_position = start_pos,
        .error_position = pos,
        .data = {.base = SLP_STATIC_BASE_NONE, .data = NULL, .byte_length = 0}};
  }

  size_t token_start = pos;
  uint8_t first_char = buf->data[pos];

  typedef enum {
    STATE_START,
    STATE_SIGN,
    STATE_INTEGER,
    STATE_REAL,
    STATE_SYMBOL,
    STATE_ERROR
  } parse_state_t;

  parse_state_t state = STATE_START;
  bool has_sign = false;
  bool has_period = false;

  if (first_char == '+' || first_char == '-') {
    has_sign = true;
    pos++;

    if (pos >= buf->count) {
      state = STATE_SYMBOL;
      pos = token_start + 1;
    } else if (is_digit(buf->data[pos])) {
      state = STATE_INTEGER;
    } else if (is_whitespace(buf->data[pos])) {
      state = STATE_SYMBOL;
      pos = token_start + 1;
    } else {
      state = STATE_SYMBOL;
    }
  } else if (is_digit(first_char)) {
    state = STATE_INTEGER;
  } else {
    state = STATE_SYMBOL;
  }

  while (pos < buf->count && state != STATE_ERROR) {
    uint8_t c = buf->data[pos];

    if (is_whitespace(c)) {
      break;
    }

    if (is_stop_symbol(c, stop_symbols)) {
      break;
    }

    switch (state) {
    case STATE_INTEGER:
      if (is_digit(c)) {
        pos++;
      } else if (c == '.') {
        has_period = true;
        state = STATE_REAL;
        pos++;
      } else {
        state = STATE_ERROR;
      }
      break;

    case STATE_REAL:
      if (is_digit(c)) {
        pos++;
      } else if (c == '.') {
        state = STATE_ERROR;
      } else {
        state = STATE_ERROR;
      }
      break;

    case STATE_SYMBOL:
      pos++;
      break;

    default:
      state = STATE_ERROR;
      break;
    }
  }

  if (state == STATE_ERROR) {
    return (slp_scanner_static_type_result_t){
        .success = false,
        .start_position = start_pos,
        .error_position = pos,
        .data = {.base = SLP_STATIC_BASE_NONE, .data = NULL, .byte_length = 0}};
  }

  if (pos == token_start) {
    return (slp_scanner_static_type_result_t){
        .success = false,
        .start_position = start_pos,
        .error_position = pos,
        .data = {.base = SLP_STATIC_BASE_NONE, .data = NULL, .byte_length = 0}};
  }

  slp_static_base_e base_type;
  if (state == STATE_INTEGER) {
    base_type = SLP_STATIC_BASE_INTEGER;
  } else if (state == STATE_REAL) {
    base_type = SLP_STATIC_BASE_REAL;
  } else {
    base_type = SLP_STATIC_BASE_SYMBOL;
  }

  size_t token_length = pos - token_start;
  scanner->position = pos;

  return (slp_scanner_static_type_result_t){
      .success = true,
      .start_position = start_pos,
      .error_position = 0,
      .data = {.base = base_type,
               .data = &buf->data[token_start],
               .byte_length = token_length}};
}