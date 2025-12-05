#include "slp.h"
#include <stdlib.h>

static slp_fn_data_free_fn g_builtin_free_fn = NULL;
static slp_fn_data_copy_fn g_builtin_copy_fn = NULL;
static slp_fn_data_free_fn g_lambda_free_fn = NULL;
static slp_fn_data_copy_fn g_lambda_copy_fn = NULL;
static slp_fn_data_equal_fn g_lambda_equal_fn = NULL;

void slp_register_builtin_handlers(slp_fn_data_free_fn free_fn,
                                   slp_fn_data_copy_fn copy_fn) {
  g_builtin_free_fn = free_fn;
  g_builtin_copy_fn = copy_fn;
}

void slp_register_lambda_handlers(slp_fn_data_free_fn free_fn,
                                  slp_fn_data_copy_fn copy_fn,
                                  slp_fn_data_equal_fn equal_fn) {
  g_lambda_free_fn = free_fn;
  g_lambda_copy_fn = copy_fn;
  g_lambda_equal_fn = equal_fn;
}

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
    if (object->value.fn_data && g_builtin_free_fn) {
      g_builtin_free_fn(object->value.fn_data);
    }
  } else if (object->type == SLP_TYPE_LAMBDA) {
    if (object->value.fn_data && g_lambda_free_fn) {
      g_lambda_free_fn(object->value.fn_data);
    }
  } else if (object->type == SLP_TYPE_ERROR) {
    if (object->value.fn_data) {
      slp_error_data_t *error_data = (slp_error_data_t *)object->value.fn_data;
      if (error_data->message) {
        free(error_data->message);
      }
      if (error_data->source_buffer) {
        slp_buffer_destroy(error_data->source_buffer);
      }
      free(error_data);
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
  clone->source_position = object->source_position;

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

  case SLP_TYPE_BUILTIN:
    if (object->value.fn_data && g_builtin_copy_fn) {
      clone->value.fn_data = g_builtin_copy_fn(object->value.fn_data);
      if (!clone->value.fn_data) {
        free(clone);
        return NULL;
      }
    } else {
      clone->value.fn_data = object->value.fn_data;
    }
    break;
  case SLP_TYPE_LAMBDA:
    if (object->value.fn_data && g_lambda_copy_fn) {
      clone->value.fn_data = g_lambda_copy_fn(object->value.fn_data);
      if (!clone->value.fn_data) {
        free(clone);
        return NULL;
      }
    } else {
      clone->value.fn_data = NULL;
    }
    break;
  case SLP_TYPE_ERROR:
    if (object->value.fn_data) {
      slp_error_data_t *original_error =
          (slp_error_data_t *)object->value.fn_data;
      slp_error_data_t *cloned_error = malloc(sizeof(slp_error_data_t));
      if (!cloned_error) {
        free(clone);
        return NULL;
      }
      cloned_error->position = original_error->position;
      cloned_error->error_type = original_error->error_type;
      if (original_error->message) {
        cloned_error->message = malloc(strlen(original_error->message) + 1);
        if (!cloned_error->message) {
          free(cloned_error);
          free(clone);
          return NULL;
        }
        strcpy(cloned_error->message, original_error->message);
      } else {
        cloned_error->message = NULL;
      }
      if (original_error->source_buffer) {
        cloned_error->source_buffer =
            slp_buffer_copy(original_error->source_buffer);
        if (!cloned_error->source_buffer) {
          if (cloned_error->message) {
            free(cloned_error->message);
          }
          free(cloned_error);
          free(clone);
          return NULL;
        }
      } else {
        cloned_error->source_buffer = NULL;
      }
      clone->value.fn_data = cloned_error;
    } else {
      clone->value.fn_data = NULL;
    }
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

bool slp_objects_equal(slp_object_t *a, slp_object_t *b) {
  if (!a && !b) {
    return true;
  }
  if (!a || !b) {
    return false;
  }

  if (a->type != b->type) {
    return false;
  }

  switch (a->type) {
  case SLP_TYPE_NONE:
    return true;

  case SLP_TYPE_INTEGER:
    return memcmp(&a->value.integer, &b->value.integer, sizeof(int64_t)) == 0;

  case SLP_TYPE_REAL:
    return memcmp(&a->value.real, &b->value.real, sizeof(double)) == 0;

  case SLP_TYPE_SYMBOL:
  case SLP_TYPE_LIST_S:
  case SLP_TYPE_QUOTED:
    if (!a->value.buffer && !b->value.buffer) {
      return true;
    }
    if (!a->value.buffer || !b->value.buffer) {
      return false;
    }
    if (a->value.buffer->count != b->value.buffer->count) {
      return false;
    }
    return memcmp(a->value.buffer->data, b->value.buffer->data,
                  a->value.buffer->count) == 0;

  case SLP_TYPE_LIST_P:
  case SLP_TYPE_LIST_C:
  case SLP_TYPE_LIST_B:
    if (a->value.list.count != b->value.list.count) {
      return false;
    }
    for (size_t i = 0; i < a->value.list.count; i++) {
      if (!slp_objects_equal(a->value.list.items[i], b->value.list.items[i])) {
        return false;
      }
    }
    return true;

  case SLP_TYPE_BUILTIN:
    return a->value.fn_data == b->value.fn_data;

  case SLP_TYPE_LAMBDA:
    if (!a->value.fn_data && !b->value.fn_data) {
      return true;
    }
    if (!a->value.fn_data || !b->value.fn_data) {
      return false;
    }
    if (g_lambda_equal_fn) {
      return g_lambda_equal_fn(a->value.fn_data, b->value.fn_data);
    }
    return a->value.fn_data == b->value.fn_data;

  case SLP_TYPE_ERROR:
    return a->value.fn_data == b->value.fn_data;

  default:
    return false;
  }
}

void slp_process_group(slp_scanner_t *scanner, uint8_t start, uint8_t end,
                       const char *group_name, slp_type_e list_type,
                       slp_processor_state_t *state,
                       slp_scanner_stop_symbols_t *stops, int depth,
                       slp_callbacks_t *callbacks) {
  slp_scanner_find_group_result_t group =
      slp_scanner_find_group(scanner, start, end, NULL, false);

  if (!group.success) {
    if (callbacks->on_error) {
      char msg[128];
      snprintf(msg, sizeof(msg), "Failed to find closing '%c' for group", end);
      callbacks->on_error(SLP_ERROR_UNCLOSED_GROUP, msg, scanner->position,
                          scanner->buffer, callbacks->context);
    } else {
      fprintf(stderr, "[ERROR] Failed to find closing '%c' for group\n", end);
    }
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
    uint8_t current = scanner->buffer->data[scanner->position];

    if (depth == 0 && current == '\n') {
      scanner->position++;
      if (state->virtual_paren_active) {
        if (callbacks->on_virtual_list_end) {
          callbacks->on_virtual_list_end(callbacks->context);
        }
        state->virtual_paren_active = 0;
      }
      continue;
    }

    if (!slp_scanner_goto_next_non_white(scanner)) {
      break;
    }

    current = scanner->buffer->data[scanner->position];

    if (current == '(') {
      state->current_depth++;
      size_t errors_before = state->errors;
      slp_process_group(scanner, '(', ')', "LIST_P", SLP_TYPE_LIST_P, state,
                        stops, depth, callbacks);
      state->current_depth--;
      if (state->errors > errors_before) {
        break;
      }
      continue;
    } else if (current == '[') {
      state->current_depth++;
      size_t errors_before = state->errors;
      slp_process_group(scanner, '[', ']', "LIST_B", SLP_TYPE_LIST_B, state,
                        stops, depth, callbacks);
      state->current_depth--;
      if (state->errors > errors_before) {
        break;
      }
      continue;
    } else if (current == '{') {
      state->current_depth++;
      size_t errors_before = state->errors;
      slp_process_group(scanner, '{', '}', "LIST_C", SLP_TYPE_LIST_C, state,
                        stops, depth, callbacks);
      state->current_depth--;
      if (state->errors > errors_before) {
        break;
      }
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
          if (callbacks->on_error) {
            char msg[128];
            snprintf(msg, sizeof(msg),
                     "Failed to find closing '%c' for quoted group", end_delim);
            callbacks->on_error(SLP_ERROR_UNCLOSED_QUOTED_GROUP, msg,
                                scanner->position, scanner->buffer,
                                callbacks->context);
          } else {
            fprintf(stderr,
                    "[ERROR] Failed to find closing '%c' for quoted group\n",
                    end_delim);
          }
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
            object->source_position = group.index_of_start_symbol;
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
              object->source_position =
                  scanner->position - result.data.byte_length;
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
          if (callbacks->on_error) {
            char msg[128];
            snprintf(msg, sizeof(msg), "Failed to parse quoted token");
            callbacks->on_error(SLP_ERROR_PARSE_QUOTED_TOKEN, msg,
                                result.error_position, scanner->buffer,
                                callbacks->context);
          } else {
            fprintf(stderr,
                    "[ERROR] Failed to parse quoted token at position %zu\n",
                    result.error_position);
          }
          state->errors++;
          break;
        }
      }

      continue;
    }

    if (depth == 0 && !state->virtual_paren_active) {
      if (callbacks->on_virtual_list_start) {
        callbacks->on_virtual_list_start(callbacks->context);
      }
      state->virtual_paren_active = 1;
    }

    slp_scanner_static_type_result_t result =
        slp_scanner_read_static_base_type(scanner, stops);

    if (!result.success) {
      if (callbacks->on_error) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Failed to parse token");
        callbacks->on_error(SLP_ERROR_PARSE_TOKEN, msg, result.error_position,
                            scanner->buffer, callbacks->context);
      } else {
        fprintf(stderr, "[ERROR] Failed to parse token at position %zu\n",
                result.error_position);
      }
      state->errors++;
      break;
    }

    slp_object_t *object = malloc(sizeof(slp_object_t));
    if (!object) {
      if (callbacks->on_error) {
        callbacks->on_error(SLP_ERROR_ALLOCATION, "Failed to allocate object",
                            scanner->position, scanner->buffer,
                            callbacks->context);
      } else {
        fprintf(stderr, "[ERROR] Failed to allocate object\n");
      }
      state->errors++;
      break;
    }

    object->source_position = scanner->position - result.data.byte_length;

    if (result.data.base == SLP_STATIC_BASE_INTEGER) {
      object->type = SLP_TYPE_INTEGER;
      char *temp = malloc(result.data.byte_length + 1);
      if (temp) {
        memcpy(temp, result.data.data, result.data.byte_length);
        temp[result.data.byte_length] = '\0';
        object->value.integer = strtoll(temp, NULL, 10);
        free(temp);
      } else {
        if (callbacks->on_error) {
          callbacks->on_error(SLP_ERROR_ALLOCATION,
                              "Failed to allocate temp buffer for integer",
                              scanner->position, scanner->buffer,
                              callbacks->context);
        } else {
          fprintf(stderr,
                  "[ERROR] Failed to allocate temp buffer for integer\n");
        }
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
        if (callbacks->on_error) {
          callbacks->on_error(
              SLP_ERROR_ALLOCATION, "Failed to allocate temp buffer for real",
              scanner->position, scanner->buffer, callbacks->context);
        } else {
          fprintf(stderr, "[ERROR] Failed to allocate temp buffer for real\n");
        }
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
        if (callbacks->on_error) {
          callbacks->on_error(
              SLP_ERROR_BUFFER_OPERATION, "Failed to create symbol buffer",
              scanner->position, scanner->buffer, callbacks->context);
        } else {
          fprintf(stderr, "[ERROR] Failed to create symbol buffer\n");
        }
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

  if (depth == 0 && state->virtual_paren_active) {
    if (callbacks->on_virtual_list_end) {
      callbacks->on_virtual_list_end(callbacks->context);
    }
    state->virtual_paren_active = 0;
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
      .virtual_paren_active = 0,
      .current_depth = 0,
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
