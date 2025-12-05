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
  }

  free(object);
}

void slp_process_group(slp_scanner_t *scanner, uint8_t start, uint8_t end,
                       const char *group_name, slp_processor_state_t *state,
                       slp_scanner_stop_symbols_t *stops, int depth,
                       slp_object_consumer_fn consumer, void *context) {
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

  size_t content_start = group.index_of_start_symbol + 1;
  size_t content_len = group.index_of_closing_symbol - content_start;

  if (content_len > 0) {
    int bytes_copied = 0;
    slp_buffer_t *sub_buffer = slp_buffer_sub_buffer(
        scanner->buffer, content_start, content_len, &bytes_copied);

    if (sub_buffer && bytes_copied > 0) {
      slp_scanner_t *sub_scanner = slp_scanner_new(sub_buffer, 0);
      if (sub_scanner) {
        slp_process_tokens(sub_scanner, state, stops, depth + 1, consumer,
                           context);
        slp_scanner_free(sub_scanner);
      }
      slp_buffer_destroy(sub_buffer);
    }
  }

  scanner->position = group.index_of_closing_symbol + 1;
  state->tokens_processed++;
}

void slp_process_tokens(slp_scanner_t *scanner, slp_processor_state_t *state,
                        slp_scanner_stop_symbols_t *stops, int depth,
                        slp_object_consumer_fn consumer, void *context) {
  while (scanner->position < scanner->buffer->count) {
    if (!slp_scanner_goto_next_non_white(scanner)) {
      break;
    }

    uint8_t current = scanner->buffer->data[scanner->position];

    if (current == '(') {
      slp_process_group(scanner, '(', ')', "LIST_P", state, stops, depth,
                        consumer, context);
      continue;
    } else if (current == '[') {
      slp_process_group(scanner, '[', ']', "LIST_B", state, stops, depth,
                        consumer, context);
      continue;
    } else if (current == '{') {
      slp_process_group(scanner, '{', '}', "LIST_C", state, stops, depth,
                        consumer, context);
      continue;
    } else if (current == '"') {
      slp_process_group(scanner, '"', '"', "LIST_S", state, stops, depth,
                        consumer, context);
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
            consumer(object, context);
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
              consumer(object, context);
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

    consumer(object, context);
    state->tokens_processed++;
  }
}

int slp_process_buffer(slp_buffer_t *buffer, slp_object_consumer_fn consumer,
                       void *context) {
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

  slp_process_tokens(scanner, &state, &stops, 0, consumer, context);

  printf("\n[SUMMARY] Tokens processed: %zu, Errors: %zu\n",
         state.tokens_processed, state.errors);

  slp_scanner_free(scanner);

  return state.errors > 0 ? 1 : 0;
}

int slp_process_file(char *file_name, slp_object_consumer_fn consumer,
                     void *context) {
  printf("[DEBUG] Processing file: %s\n", file_name);

  slp_buffer_t *buffer = slp_buffer_from_file(file_name);
  if (!buffer) {
    fprintf(stderr, "Failed to load file: %s\n", file_name);
    return 1;
  }

  int return_value = slp_process_buffer(buffer, consumer, context);
  slp_buffer_destroy(buffer);
  return return_value;
}
