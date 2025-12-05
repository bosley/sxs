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

/*
============================================================================================================
This scans the buffer from the current start position until some given end
symbol We offer the option of "escaping" briefly from detecting the end byte
============================================================================================================
*/

slp_scanner_find_group_result_t
slp_scanner_find_group(slp_scanner_t *scanner, uint8_t must_start_with,
                       uint8_t must_end_with, uint8_t *can_escape_with,
                       bool consume_leading_ws) {
  if (!scanner) {
    return (slp_scanner_find_group_result_t){.success = false,
                                             .index_of_start_symbol = 0,
                                             .index_of_closing_symbol = 0};
  }

  slp_buffer_t *buf = scanner->buffer;
  if (!buf) {
    return (slp_scanner_find_group_result_t){.success = false,
                                             .index_of_start_symbol = 0,
                                             .index_of_closing_symbol = 0};
  }

  size_t pos = scanner->position;

  if (pos >= buf->count) {
    return (slp_scanner_find_group_result_t){.success = false,
                                             .index_of_start_symbol = 0,
                                             .index_of_closing_symbol = 0};
  }

  if (consume_leading_ws) {
    while (pos < buf->count && is_whitespace(buf->data[pos])) {
      pos++;
    }

    if (pos >= buf->count) {
      return (slp_scanner_find_group_result_t){.success = false,
                                               .index_of_start_symbol = 0,
                                               .index_of_closing_symbol = 0};
    }
  }

  if (buf->data[pos] != must_start_with) {
    return (slp_scanner_find_group_result_t){.success = false,
                                             .index_of_start_symbol = 0,
                                             .index_of_closing_symbol = 0};
  }

  size_t start_index = pos;
  pos++;

  bool same_delimiters = (must_start_with == must_end_with);
  int depth = 1;

  while (pos < buf->count) {
    uint8_t current = buf->data[pos];

    bool is_escaped = false;
    if (can_escape_with != NULL && pos > start_index + 1) {
      if (buf->data[pos - 1] == *can_escape_with) {
        is_escaped = true;
      }
    }

    if (!is_escaped) {
      if (same_delimiters) {
        if (current == must_end_with) {
          scanner->position = pos;
          return (slp_scanner_find_group_result_t){
              .success = true,
              .index_of_start_symbol = start_index,
              .index_of_closing_symbol = pos};
        }
      } else {
        if (current == must_start_with) {
          depth++;
        } else if (current == must_end_with) {
          depth--;
          if (depth == 0) {
            scanner->position = pos;
            return (slp_scanner_find_group_result_t){
                .success = true,
                .index_of_start_symbol = start_index,
                .index_of_closing_symbol = pos};
          }
        }
      }
    }

    pos++;
  }

  return (slp_scanner_find_group_result_t){.success = false,
                                           .index_of_start_symbol = 0,
                                           .index_of_closing_symbol = 0};
}

bool slp_scanner_goto_next_non_white(slp_scanner_t *scanner) {
  if (!scanner) {
    return false;
  }

  slp_buffer_t *buf = scanner->buffer;
  if (!buf) {
    return false;
  }

  size_t pos = scanner->position;

  while (pos < buf->count && is_whitespace(buf->data[pos])) {
    pos++;
  }

  if (pos >= buf->count) {
    return false;
  }

  scanner->position = pos;
  return true;
}

bool slp_scanner_skip_whitespace_and_comments(slp_scanner_t *scanner) {
  if (!scanner) {
    return false;
  }

  slp_buffer_t *buf = scanner->buffer;
  if (!buf) {
    return false;
  }

  size_t pos = scanner->position;

  while (pos < buf->count) {
    if (is_whitespace(buf->data[pos])) {
      pos++;
      continue;
    }

    if (buf->data[pos] == ';') {
      while (pos < buf->count && buf->data[pos] != '\n') {
        pos++;
      }
      if (pos < buf->count && buf->data[pos] == '\n') {
        pos++;
      }
      continue;
    }

    break;
  }

  if (pos >= buf->count) {
    return false;
  }

  scanner->position = pos;
  return true;
}

bool slp_scanner_goto_next_target(slp_scanner_t *scanner, uint8_t target_byte) {
  if (!scanner) {
    return false;
  }

  slp_buffer_t *buf = scanner->buffer;
  if (!buf) {
    return false;
  }
  size_t pos = scanner->position;

  if (pos >= buf->count) {
    return false;
  }

  if (buf->data[pos] == target_byte) {
    scanner->position = pos;
    return true;
  }

  while (pos < buf->count && buf->data[pos] != target_byte) {
    pos++;
  }

  if (pos >= buf->count) {
    return false;
  }

  scanner->position = pos;
  return true;
}