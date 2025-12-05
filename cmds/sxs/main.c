#include "buffer/buffer.h"
#include "scanner/scanner.h"
#include "types/types.h"
#include <libtcc.h>
#include <stdio.h>
#include <string.h>

typedef enum sxs_processor_state_e {
  SXS_PROCESSOR_STATE_NONE = 0,
  SXS_PROCESSOR_STATE_IDLE,
  SXS_PROCESSOR_STATE_READING,
  SXS_PROCESSOR_STATE_ERROR,
} sxs_processor_state_e;

typedef struct sxs_processor_state_s {
  sxs_processor_state_e state;
  size_t tokens_processed;
  size_t errors;
} sxs_processor_state_t;

void print_static_type(slp_static_type_t *type, int depth) {
  if (!type || !type->data) {
    return;
  }

  for (int i = 0; i < depth; i++) {
    printf("  ");
  }
  printf("[");
  switch (type->base) {
  case SLP_STATIC_BASE_INTEGER:
    printf("INTEGER");
    break;
  case SLP_STATIC_BASE_REAL:
    printf("REAL");
    break;
  case SLP_STATIC_BASE_SYMBOL:
    printf("SYMBOL");
    break;
  default:
    printf("UNKNOWN");
    break;
  }
  printf("] ");

  for (size_t i = 0; i < type->byte_length; i++) {
    printf("%c", type->data[i]);
  }
  printf("\n");
}

void process_tokens(slp_scanner_t *scanner, sxs_processor_state_t *state,
                    slp_scanner_stop_symbols_t *stops, int depth);

void process_group(slp_scanner_t *scanner, uint8_t start, uint8_t end,
                   const char *group_name, sxs_processor_state_t *state,
                   slp_scanner_stop_symbols_t *stops, int depth) {
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
        process_tokens(sub_scanner, state, stops, depth + 1);
        slp_scanner_free(sub_scanner);
      }
      slp_buffer_destroy(sub_buffer);
    }
  }

  scanner->position = group.index_of_closing_symbol + 1;
  state->tokens_processed++;
}

void process_tokens(slp_scanner_t *scanner, sxs_processor_state_t *state,
                    slp_scanner_stop_symbols_t *stops, int depth) {
  while (scanner->position < scanner->buffer->count) {
    if (!slp_scanner_goto_next_non_white(scanner)) {
      break;
    }

    uint8_t current = scanner->buffer->data[scanner->position];

    if (current == '(') {
      process_group(scanner, '(', ')', "LIST_P", state, stops, depth);
      continue;
    } else if (current == '[') {
      process_group(scanner, '[', ']', "LIST_B", state, stops, depth);
      continue;
    } else if (current == '{') {
      process_group(scanner, '{', '}', "LIST_C", state, stops, depth);
      continue;
    } else if (current == '"') {
      process_group(scanner, '"', '"', "LIST_S", state, stops, depth);
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
        for (int i = 0; i < depth; i++) {
          printf("  ");
        }
        printf("[QUOTED] ");
        for (size_t i = group.index_of_start_symbol;
             i <= group.index_of_closing_symbol; i++) {
          printf("%c", scanner->buffer->data[i]);
        }
        printf("\n");
        scanner->position = group.index_of_closing_symbol + 1;
        state->tokens_processed++;
      } else {
        slp_scanner_static_type_result_t result =
            slp_scanner_read_static_base_type(scanner, stops);
        if (result.success) {
          for (int i = 0; i < depth; i++) {
            printf("  ");
          }
          printf("[QUOTED] ");
          for (size_t i = 0; i < result.data.byte_length; i++) {
            printf("%c", result.data.data[i]);
          }
          printf("\n");
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
      state->state = SXS_PROCESSOR_STATE_ERROR;
      break;
    }

    print_static_type(&result.data, depth);
    state->tokens_processed++;
  }
}

int process_buffer(slp_buffer_t *buffer) {
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

  sxs_processor_state_t state = {
      .state = SXS_PROCESSOR_STATE_IDLE,
      .tokens_processed = 0,
      .errors = 0,
  };

  state.state = SXS_PROCESSOR_STATE_READING;

  uint8_t stop_symbols[] = {'(', ')', '[', ']', '{', '}', '"', '\''};
  slp_scanner_stop_symbols_t stops = {.symbols = stop_symbols,
                                      .count = sizeof(stop_symbols)};

  process_tokens(scanner, &state, &stops, 0);

  printf("\n[SUMMARY] Tokens processed: %zu, Errors: %zu\n",
         state.tokens_processed, state.errors);

  slp_scanner_free(scanner);

  return state.errors > 0 ? 1 : 0;
}

int process_file(char *file_name) {
  printf("[DEBUG] Processing file: %s\n", file_name);

  slp_buffer_t *buffer = slp_buffer_from_file(file_name);
  if (!buffer) {
    fprintf(stderr, "Failed to load file: %s\n", file_name);
    return 1;
  }

  int return_value = process_buffer(buffer);
  slp_buffer_destroy(buffer);
  return return_value;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <file>\n", argv[0]);
    return 1;
  }

  return process_file(argv[1]);
}