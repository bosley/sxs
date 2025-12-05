#include "slp.h"
#include <stdlib.h>

void slp_free_object(slp_object_t *object) {
  if (!object) {
    return;
  }

  if (object->type == SLP_TYPE_SYMBOL || object->type == SLP_TYPE_QUOTED) {
    if (object->value.buffer) {
      slp_buffer_destroy(object->value.buffer);
    }
  } else if (object->type == SLP_TYPE_LIST_P ||
             object->type == SLP_TYPE_LIST_B ||
             object->type == SLP_TYPE_LIST_C) {
    if (object->value.list.items) {
      for (size_t i = 0; i < object->value.list.count; i++) {
        slp_free_object(object->value.list.items[i]);
      }
      free(object->value.list.items);
    }
  } else if (object->type == SLP_TYPE_BUILTIN) {
    if (object->value.fn_data) {
      // TODO: Free builtin function data (no structs yet)
    }
  } else if (object->type == SLP_TYPE_LAMBDA) {
    if (object->value.fn_data) {
      // TODO: Free lambda function data (we dont have a structure for it yet)
    }
  }

  free(object);
}

slp_object_t *slp_object_copy(slp_object_t *object) {
  if (!object) {
    return NULL;
  }

  slp_object_t *clone = malloc(sizeof(slp_object_t));
  if (!clone) {
    return NULL;
  }

  clone->type = object->type;

  switch (object->type) {
  case SLP_TYPE_INTEGER:
    clone->value.integer = object->value.integer;
    break;
  case SLP_TYPE_REAL:
    clone->value.real = object->value.real;
    break;
  case SLP_TYPE_SYMBOL:
  case SLP_TYPE_QUOTED:
    if (object->value.buffer) {
      clone->value.buffer = slp_buffer_copy(object->value.buffer);
      if (!clone->value.buffer) {
        free(clone);
        return NULL;
      }
    } else {
      clone->value.buffer = NULL;
    }
    break;

  // We don't clone C function data for builtins
  case SLP_TYPE_BUILTIN:
    clone->value.fn_data = object->value.fn_data;
    break;
  case SLP_TYPE_LAMBDA:

    /*
TODO: Once we define the structgure of lambda we need to ompement a copy

    */
    break;
  case SLP_TYPE_LIST_P:
  case SLP_TYPE_LIST_B:
  case SLP_TYPE_LIST_C:
    if (object->value.list.items && object->value.list.count > 0) {
      clone->value.list.count = object->value.list.count;
      clone->value.list.items =
          malloc(sizeof(slp_object_t *) * object->value.list.count);
      if (!clone->value.list.items) {
        free(clone);
        return NULL;
      }
      for (size_t i = 0; i < object->value.list.count; i++) {
        clone->value.list.items[i] =
            slp_object_copy(object->value.list.items[i]);
        if (!clone->value.list.items[i]) {
          for (size_t j = 0; j < i; j++) {
            slp_free_object(clone->value.list.items[j]);
          }
          free(clone->value.list.items);
          free(clone);
          return NULL;
        }
      }
    } else {
      clone->value.list.items = NULL;
      clone->value.list.count = 0;
    }
    break;
  default:
    clone->value.buffer = NULL;
    break;
  }

  return clone;
}

void slp_process_group(slp_scanner_t *scanner, uint8_t start, uint8_t end,
                       const char *group_name, slp_type_e list_type,
                       slp_processor_state_t *state,
                       slp_scanner_stop_symbols_t *stops, int depth,
                       slp_callbacks_t *callbacks) {
  slp_scanner_find_group_result_t group =
      slp_scanner_find_group(scanner, start, end, NULL, false);

  if (!group.success) {
    fprintf(stderr, "[ERROR] Failed to find closing '%c' for group\n", end);
    state->errors++;
    return;
  }

  for (int i = 0; i < depth; i++) {
    printf("  ");
  }
  printf("[%s]\n", group_name);

  if (callbacks->on_list_start) {
    callbacks->on_list_start(list_type, callbacks->context);
  }

  size_t content_start = group.index_of_start_symbol + 1;
  size_t content_len = group.index_of_closing_symbol - content_start;

  if (content_len > 0) {
    int bytes_copied = 0;
    slp_buffer_t *sub_buffer = slp_buffer_sub_buffer(
        scanner->buffer, content_start, content_len, &bytes_copied);

    if (sub_buffer && bytes_copied > 0) {
      slp_scanner_t *sub_scanner = slp_scanner_new(sub_buffer, 0);
      if (sub_scanner) {
        slp_process_tokens(sub_scanner, state, stops, depth + 1, callbacks);
        slp_scanner_free(sub_scanner);
      }
      slp_buffer_destroy(sub_buffer);
    }
  }

  if (callbacks->on_list_end) {
    callbacks->on_list_end(list_type, callbacks->context);
  }

  scanner->position = group.index_of_closing_symbol + 1;
  state->tokens_processed++;
}

void slp_process_tokens(slp_scanner_t *scanner, slp_processor_state_t *state,
                        slp_scanner_stop_symbols_t *stops, int depth,
                        slp_callbacks_t *callbacks) {
  while (scanner->position < scanner->buffer->count) {
    if (!slp_scanner_goto_next_non_white(scanner)) {
      break;
    }

    uint8_t current = scanner->buffer->data[scanner->position];

    if (current == '(') {
      slp_process_group(scanner, '(', ')', "LIST_P", SLP_TYPE_LIST_P, state,
                        stops, depth, callbacks);
      continue;
    } else if (current == '[') {
      slp_process_group(scanner, '[', ']', "LIST_B", SLP_TYPE_LIST_B, state,
                        stops, depth, callbacks);
      continue;
    } else if (current == '{') {
      slp_process_group(scanner, '{', '}', "LIST_C", SLP_TYPE_LIST_C, state,
                        stops, depth, callbacks);
      continue;
    } else if (current == '"') {
      slp_process_group(scanner, '"', '"', "LIST_S", SLP_TYPE_LIST_S, state,
                        stops, depth, callbacks);
      continue;
    } else if (current == '\'') {
      scanner->position++;

      if (!slp_scanner_goto_next_non_white(scanner)) {
        break;
      }

      if (scanner->position >= scanner->buffer->count) {
        break;
      }

      uint8_t quoted_char = scanner->buffer->data[scanner->position];
      uint8_t start_delim = 0;
      uint8_t end_delim = 0;

      switch (quoted_char) {
      case '(':
        start_delim = '(';
        end_delim = ')';
        break;
      case '[':
        start_delim = '[';
        end_delim = ']';
        break;
      case '{':
        start_delim = '{';
        end_delim = '}';
        break;
      case '"':
        start_delim = '"';
        end_delim = '"';
        break;
      default:
        break;
      }

      if (start_delim != 0) {
        slp_scanner_find_group_result_t group = slp_scanner_find_group(
            scanner, start_delim, end_delim, NULL, false);
        if (!group.success) {
          fprintf(stderr,
                  "[ERROR] Failed to find closing '%c' for quoted group\n",
                  end_delim);
          state->errors++;
          break;
        }

        size_t content_len =
            group.index_of_closing_symbol - group.index_of_start_symbol + 1;
        int bytes_copied = 0;
        slp_buffer_t *quoted_buffer =
            slp_buffer_sub_buffer(scanner->buffer, group.index_of_start_symbol,
                                  content_len, &bytes_copied);

        if (quoted_buffer && bytes_copied > 0) {
          slp_object_t *object = malloc(sizeof(slp_object_t));
          if (object) {
            object->type = SLP_TYPE_QUOTED;
            object->value.buffer = quoted_buffer;
            if (callbacks->on_object) {
              callbacks->on_object(object, callbacks->context);
            }
          } else {
            slp_buffer_destroy(quoted_buffer);
          }
        }

        scanner->position = group.index_of_closing_symbol + 1;
        state->tokens_processed++;
      } else {
        slp_scanner_static_type_result_t result =
            slp_scanner_read_static_base_type(scanner, stops);
        if (result.success) {
          int bytes_copied = 0;
          slp_buffer_t *quoted_buffer = slp_buffer_sub_buffer(
              scanner->buffer, scanner->position - result.data.byte_length,
              result.data.byte_length, &bytes_copied);

          if (quoted_buffer && bytes_copied > 0) {
            slp_object_t *object = malloc(sizeof(slp_object_t));
            if (object) {
              object->type = SLP_TYPE_QUOTED;
              object->value.buffer = quoted_buffer;
              if (callbacks->on_object) {
                callbacks->on_object(object, callbacks->context);
              }
            } else {
              slp_buffer_destroy(quoted_buffer);
            }
          }

          state->tokens_processed++;
        } else {
          fprintf(stderr,
                  "[ERROR] Failed to parse quoted token at position %zu\n",
                  result.error_position);
          state->errors++;
          break;
        }
      }

      continue;
    }

    slp_scanner_static_type_result_t result =
        slp_scanner_read_static_base_type(scanner, stops);

    if (!result.success) {
      fprintf(stderr, "[ERROR] Failed to parse token at position %zu\n",
              result.error_position);
      state->errors++;
      break;
    }

    slp_object_t *object = malloc(sizeof(slp_object_t));
    if (!object) {
      fprintf(stderr, "[ERROR] Failed to allocate object\n");
      state->errors++;
      break;
    }

    if (result.data.base == SLP_STATIC_BASE_INTEGER) {
      object->type = SLP_TYPE_INTEGER;
      char *temp = malloc(result.data.byte_length + 1);
      if (temp) {
        memcpy(temp, result.data.data, result.data.byte_length);
        temp[result.data.byte_length] = '\0';
        object->value.integer = strtoll(temp, NULL, 10);
        free(temp);
      } else {
        free(object);
        state->errors++;
        break;
      }
    } else if (result.data.base == SLP_STATIC_BASE_REAL) {
      object->type = SLP_TYPE_REAL;
      char *temp = malloc(result.data.byte_length + 1);
      if (temp) {
        memcpy(temp, result.data.data, result.data.byte_length);
        temp[result.data.byte_length] = '\0';
        object->value.real = strtod(temp, NULL);
        free(temp);
      } else {
        free(object);
        state->errors++;
        break;
      }
    } else if (result.data.base == SLP_STATIC_BASE_SYMBOL) {
      object->type = SLP_TYPE_SYMBOL;
      int bytes_copied = 0;
      slp_buffer_t *symbol_buffer = slp_buffer_sub_buffer(
          scanner->buffer, scanner->position - result.data.byte_length,
          result.data.byte_length, &bytes_copied);

      if (symbol_buffer && bytes_copied > 0) {
        object->value.buffer = symbol_buffer;
      } else {
        free(object);
        if (symbol_buffer) {
          slp_buffer_destroy(symbol_buffer);
        }
        state->errors++;
        break;
      }
    }

    if (callbacks->on_object) {
      callbacks->on_object(object, callbacks->context);
    }
    state->tokens_processed++;
  }
}

int slp_process_buffer(slp_buffer_t *buffer, slp_callbacks_t *callbacks) {
  if (!buffer) {
    fprintf(stderr, "Failed to process buffer (nil)\n");
    return 1;
  }

  printf("[DEBUG] Processing buffer: %zu bytes\n", buffer->count);

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  if (!scanner) {
    fprintf(stderr, "Failed to create scanner\n");
    return 1;
  }

  slp_processor_state_t state = {
      .tokens_processed = 0,
      .errors = 0,
  };

  uint8_t stop_symbols[] = {'(', ')', '[', ']', '{', '}', '"', '\''};
  slp_scanner_stop_symbols_t stops = {.symbols = stop_symbols,
                                      .count = sizeof(stop_symbols)};

  slp_process_tokens(scanner, &state, &stops, 0, callbacks);

  printf("\n[SUMMARY] Tokens processed: %zu, Errors: %zu\n",
         state.tokens_processed, state.errors);

  slp_scanner_free(scanner);

  return state.errors > 0 ? 1 : 0;
}

int slp_process_file(char *file_name, slp_callbacks_t *callbacks) {
  printf("[DEBUG] Processing file: %s\n", file_name);

  slp_buffer_t *buffer = slp_buffer_from_file(file_name);
  if (!buffer) {
    fprintf(stderr, "Failed to load file: %s\n", file_name);
    return 1;
  }

  int return_value = slp_process_buffer(buffer, callbacks);
  slp_buffer_destroy(buffer);
  return return_value;
}
