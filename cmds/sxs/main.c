#include "sxs/sxs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[31m"
#define COLOR_GRAY "\033[90m"

static void print_source_context(slp_buffer_t *buffer, size_t error_position) {
  if (!buffer || !buffer->data || error_position >= buffer->count) {
    return;
  }

  size_t line = 1;
  size_t col = 1;
  size_t line_start = 0;

  for (size_t i = 0; i < error_position && i < buffer->count; i++) {
    if (buffer->data[i] == '\n') {
      line++;
      col = 1;
      line_start = i + 1;
    } else {
      col++;
    }
  }

  size_t line_end = line_start;
  while (line_end < buffer->count && buffer->data[line_end] != '\n') {
    line_end++;
  }

  printf("\n  %sSource:%s\n", COLOR_GRAY, COLOR_RESET);
  printf("  %s%4zu |%s ", COLOR_GRAY, line, COLOR_RESET);

  for (size_t i = line_start; i < line_end; i++) {
    printf("%c", buffer->data[i]);
  }
  printf("\n");

  printf("  %s     |%s ", COLOR_GRAY, COLOR_RESET);
  for (size_t i = 0; i < col - 1; i++) {
    printf(" ");
  }
  printf("%s^%s\n", COLOR_RED, COLOR_RESET);

  printf("  %s     |%s ", COLOR_GRAY, COLOR_RESET);
  for (size_t i = 0; i < col - 1; i++) {
    printf(" ");
  }
  printf("%s└─ here%s\n", COLOR_RED, COLOR_RESET);
}

static void print_error(slp_object_t *object) {
  printf("\n");
  printf("╔════════════════════════════════════════════════════════════════════"
         "════════╗\n");
  printf("║ ERROR                                                              "
         "        ║\n");
  printf("╚════════════════════════════════════════════════════════════════════"
         "════════╝\n");

  if (!object->value.fn_data) {
    printf("  Unknown error\n");
    printf("───────────────────────────────────────────────────────────────────"
           "─────────────\n");
    printf("\n");
    return;
  }

  slp_error_data_t *error_data = (slp_error_data_t *)object->value.fn_data;

  if (error_data->message) {
    printf("  %s\n", error_data->message);
  }

  if (error_data->source_buffer && error_data->position > 0) {
    print_source_context(error_data->source_buffer, error_data->position);
  }

  printf("─────────────────────────────────────────────────────────────────────"
         "───────────\n");
  printf("\n");
}

static void print_object(slp_object_t *object) {
  if (!object) {
    printf("(nil)\n");
    return;
  }

  switch (object->type) {
  case SLP_TYPE_NONE:
    printf("(none)\n");
    break;
  case SLP_TYPE_INTEGER:
    printf("%lld\n", object->value.integer);
    break;
  case SLP_TYPE_REAL:
    printf("%f\n", object->value.real);
    break;
  case SLP_TYPE_SYMBOL:
    if (object->value.buffer) {
      for (size_t i = 0; i < object->value.buffer->count; i++) {
        printf("%c", object->value.buffer->data[i]);
      }
    }
    printf("\n");
    break;
  case SLP_TYPE_QUOTED:
    printf("'");
    if (object->value.buffer) {
      for (size_t i = 0; i < object->value.buffer->count; i++) {
        printf("%c", object->value.buffer->data[i]);
      }
    }
    printf("\n");
    break;
  case SLP_TYPE_LIST_P:
  case SLP_TYPE_LIST_B:
  case SLP_TYPE_LIST_C:
    printf("(list count=%zu)\n", object->value.list.count);
    break;
  case SLP_TYPE_BUILTIN:
    printf("(builtin)\n");
    break;
  case SLP_TYPE_LAMBDA:
    printf("(lambda)\n");
    break;
  case SLP_TYPE_ERROR:
    print_error(object);
    break;
  default:
    printf("(unknown)\n");
    break;
  }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <file>\n", argv[0]);
    return 1;
  }

  sxs_builtins_init();

  sxs_runtime_t *runtime = sxs_runtime_new();
  if (!runtime) {
    fprintf(stderr, "Failed to create runtime\n");
    sxs_builtins_deinit();
    return 1;
  }

  int result = sxs_runtime_process_file(runtime, argv[1]);

  if (result != 0) {
    sxs_runtime_free(runtime);
    sxs_builtins_deinit();
    return result;
  }

  slp_object_t *final_object = sxs_runtime_get_last_eval_obj(runtime);

  printf("\n[FINAL RESULT]\n");
  print_object(final_object);

  if (final_object) {
    slp_object_free(final_object);
  }

  sxs_runtime_free(runtime);

  sxs_builtins_deinit();

  return 0;
}
