#include "sxs/sxs.h"

#include <stdio.h>
#include <stdlib.h>

static void print_object_simple(slp_object_t *obj, size_t index) {
  if (!obj) {
    printf("  [%zu] NULL\n", index);
    return;
  }

  printf("  [%zu] ", index);

  switch (obj->type) {
  case SLP_TYPE_INTEGER:
    printf("INTEGER: %lld\n", obj->value.integer);
    break;
  case SLP_TYPE_REAL:
    printf("REAL: %f\n", obj->value.real);
    break;
  case SLP_TYPE_SYMBOL:
    printf("SYMBOL: ");
    if (obj->value.buffer) {
      for (size_t i = 0; i < obj->value.buffer->count; i++) {
        printf("%c", obj->value.buffer->data[i]);
      }
    }
    printf("\n");
    break;
  case SLP_TYPE_QUOTED:
    printf("QUOTED: ");
    if (obj->value.buffer) {
      for (size_t i = 0; i < obj->value.buffer->count; i++) {
        printf("%c", obj->value.buffer->data[i]);
      }
    }
    printf("\n");
    break;
  case SLP_TYPE_LIST_S:
    printf("LIST_S: count=%zu\n", obj->value.list.count);
    break;
  case SLP_TYPE_LIST_P:
    printf("LIST_P: count=%zu\n", obj->value.list.count);
    break;
  case SLP_TYPE_LIST_B:
    printf("LIST_B: count=%zu\n", obj->value.list.count);
    break;
  case SLP_TYPE_LIST_C:
    printf("LIST_C: count=%zu\n", obj->value.list.count);
    break;
  case SLP_TYPE_BUILTIN:
    printf("BUILTIN\n");
    break;
  case SLP_TYPE_LAMBDA:
    printf("LAMBDA\n");
    break;
  case SLP_TYPE_NONE:
    printf("NONE\n");
    break;
  case SLP_TYPE_ERROR:
    printf("ERROR\n");
    break;
  default:
    printf("UNKNOWN_TYPE\n");
    break;
  }
}

static void print_buffer_hex(slp_buffer_t *buffer, size_t max_bytes) {
  if (!buffer || !buffer->data) {
    return;
  }
  size_t limit = buffer->count < max_bytes ? buffer->count : max_bytes;
  for (size_t i = 0; i < limit; i++) {
    printf("%02x ", (unsigned char)buffer->data[i]);
  }
  if (buffer->count > max_bytes) {
    printf("... (%zu more bytes)", buffer->count - max_bytes);
  }
}

static void print_object_full_recursive(slp_object_t *obj, size_t depth);

static void print_indent(size_t depth) {
  for (size_t i = 0; i < depth; i++) {
    printf("  ");
  }
}

static void print_object_full_recursive(slp_object_t *obj, size_t depth) {
  if (!obj) {
    print_indent(depth);
    printf("NULL object\n");
    return;
  }

  print_indent(depth);
  printf("Object @ %p\n", (void *)obj);

  print_indent(depth);
  printf("  type: ");
  switch (obj->type) {
  case SLP_TYPE_INTEGER:
    printf("INTEGER\n");
    print_indent(depth);
    printf("  value: %lld\n", obj->value.integer);
    break;
  case SLP_TYPE_REAL:
    printf("REAL\n");
    print_indent(depth);
    printf("  value: %f\n", obj->value.real);
    break;
  case SLP_TYPE_SYMBOL:
    printf("SYMBOL\n");
    print_indent(depth);
    printf("  buffer: %p\n", (void *)obj->value.buffer);
    if (obj->value.buffer) {
      print_indent(depth);
      printf("  count: %zu\n", obj->value.buffer->count);
      print_indent(depth);
      printf("  data: ");
      for (size_t i = 0; i < obj->value.buffer->count; i++) {
        printf("%c", obj->value.buffer->data[i]);
      }
      printf("\n");
      print_indent(depth);
      printf("  hex: ");
      print_buffer_hex(obj->value.buffer, 32);
      printf("\n");
    }
    break;
  case SLP_TYPE_QUOTED:
    printf("QUOTED\n");
    print_indent(depth);
    printf("  buffer: %p\n", (void *)obj->value.buffer);
    if (obj->value.buffer) {
      print_indent(depth);
      printf("  count: %zu\n", obj->value.buffer->count);
      print_indent(depth);
      printf("  data: ");
      for (size_t i = 0; i < obj->value.buffer->count; i++) {
        printf("%c", obj->value.buffer->data[i]);
      }
      printf("\n");
      print_indent(depth);
      printf("  hex: ");
      print_buffer_hex(obj->value.buffer, 32);
      printf("\n");
    }
    break;
  case SLP_TYPE_LIST_S:
    printf("LIST_S\n");
    print_indent(depth);
    printf("  count: %zu\n", obj->value.list.count);
    print_indent(depth);
    printf("  items: %p\n", (void *)obj->value.list.items);
    if (obj->value.list.items && obj->value.list.count > 0) {
      for (size_t i = 0; i < obj->value.list.count; i++) {
        print_indent(depth);
        printf("  [%zu]:\n", i);
        print_object_full_recursive(obj->value.list.items[i], depth + 2);
      }
    }
    break;
  case SLP_TYPE_LIST_P:
    printf("LIST_P\n");
    print_indent(depth);
    printf("  count: %zu\n", obj->value.list.count);
    print_indent(depth);
    printf("  items: %p\n", (void *)obj->value.list.items);
    if (obj->value.list.items && obj->value.list.count > 0) {
      for (size_t i = 0; i < obj->value.list.count; i++) {
        print_indent(depth);
        printf("  [%zu]:\n", i);
        print_object_full_recursive(obj->value.list.items[i], depth + 2);
      }
    }
    break;
  case SLP_TYPE_LIST_B:
    printf("LIST_B\n");
    print_indent(depth);
    printf("  count: %zu\n", obj->value.list.count);
    print_indent(depth);
    printf("  items: %p\n", (void *)obj->value.list.items);
    if (obj->value.list.items && obj->value.list.count > 0) {
      for (size_t i = 0; i < obj->value.list.count; i++) {
        print_indent(depth);
        printf("  [%zu]:\n", i);
        print_object_full_recursive(obj->value.list.items[i], depth + 2);
      }
    }
    break;
  case SLP_TYPE_LIST_C:
    printf("LIST_C\n");
    print_indent(depth);
    printf("  count: %zu\n", obj->value.list.count);
    print_indent(depth);
    printf("  items: %p\n", (void *)obj->value.list.items);
    if (obj->value.list.items && obj->value.list.count > 0) {
      for (size_t i = 0; i < obj->value.list.count; i++) {
        print_indent(depth);
        printf("  [%zu]:\n", i);
        print_object_full_recursive(obj->value.list.items[i], depth + 2);
      }
    }
    break;
  case SLP_TYPE_BUILTIN:
    printf("BUILTIN\n");
    print_indent(depth);
    printf("  fn_data: %p\n", obj->value.fn_data);
    break;
  case SLP_TYPE_LAMBDA:
    printf("LAMBDA\n");
    print_indent(depth);
    printf("  fn_data: %p\n", obj->value.fn_data);
    break;
  case SLP_TYPE_NONE:
    printf("NONE\n");
    break;
  case SLP_TYPE_ERROR:
    printf("ERROR\n");
    print_indent(depth);
    printf("  fn_data: %p\n", obj->value.fn_data);
    break;
  default:
    printf("UNKNOWN_TYPE (%d)\n", obj->type);
    break;
  }

  print_indent(depth);
  printf("  source_position: %zu\n", obj->source_position);
}

static void print_object_full(slp_object_t *obj, size_t index) {
  printf("=== Argument %zu ===\n", index);
  print_object_full_recursive(obj, 0);
  printf("\n");
}

slp_object_t *sxs_builtin_debug_simple(sxs_runtime_t *runtime,
                                       sxs_callable_t *callable,
                                       slp_object_t **args, size_t arg_count) {
  if (!runtime) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "d builtin: nil runtime", 0, NULL);
  }

  printf("[DEBUG SIMPLE] %zu argument%s\n", arg_count,
         arg_count == 1 ? "" : "s");

  if (arg_count == 0) {
    slp_object_t *none = malloc(sizeof(slp_object_t));
    if (none) {
      none->type = SLP_TYPE_NONE;
      none->source_position = 0;
    }
    return none;
  }

  slp_object_t **eval_args = malloc(sizeof(slp_object_t *) * arg_count);
  if (!eval_args) {
    return sxs_create_error_object(SLP_ERROR_ALLOCATION,
                                   "d builtin: failed to allocate eval_args", 0,
                                   runtime->source_buffer);
  }

  for (size_t i = 0; i < arg_count; i++) {
    eval_args[i] = NULL;
  }

  for (size_t i = 0; i < arg_count; i++) {
    if (!args[i]) {
      for (size_t j = 0; j < i; j++) {
        if (eval_args[j]) {
          slp_object_free(eval_args[j]);
        }
      }
      free(eval_args);
      return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                     "d builtin: nil argument", 0,
                                     runtime->source_buffer);
    }
    eval_args[i] = sxs_eval_object(runtime, args[i]);
    if (!eval_args[i]) {
      for (size_t j = 0; j < i; j++) {
        if (eval_args[j]) {
          slp_object_free(eval_args[j]);
        }
      }
      free(eval_args);
      return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                     "d builtin: eval failed", 0,
                                     runtime->source_buffer);
    }
    if (eval_args[i]->type == SLP_TYPE_ERROR) {
      slp_object_t *error = eval_args[i];
      for (size_t j = 0; j < i; j++) {
        if (eval_args[j]) {
          slp_object_free(eval_args[j]);
        }
      }
      free(eval_args);
      return error;
    }
  }

  for (size_t i = 0; i < arg_count; i++) {
    print_object_simple(eval_args[i], i);
  }

  for (size_t i = 0; i < arg_count; i++) {
    if (eval_args[i]) {
      slp_object_free(eval_args[i]);
    }
  }
  free(eval_args);

  slp_object_t *none = malloc(sizeof(slp_object_t));
  if (none) {
    none->type = SLP_TYPE_NONE;
    none->source_position = 0;
  }
  return none;
}

slp_object_t *sxs_builtin_debug_full(sxs_runtime_t *runtime,
                                     sxs_callable_t *callable,
                                     slp_object_t **args, size_t arg_count) {
  if (!runtime) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   "D builtin: nil runtime", 0, NULL);
  }

  printf("[DEBUG FULL] %zu argument%s\n", arg_count, arg_count == 1 ? "" : "s");
  printf("========================================\n");

  if (arg_count == 0) {
    printf("No arguments to debug.\n");
    printf("========================================\n");
    slp_object_t *none = malloc(sizeof(slp_object_t));
    if (none) {
      none->type = SLP_TYPE_NONE;
      none->source_position = 0;
    }
    return none;
  }

  slp_object_t **eval_args = malloc(sizeof(slp_object_t *) * arg_count);
  if (!eval_args) {
    return sxs_create_error_object(SLP_ERROR_ALLOCATION,
                                   "D builtin: failed to allocate eval_args", 0,
                                   runtime->source_buffer);
  }

  for (size_t i = 0; i < arg_count; i++) {
    eval_args[i] = NULL;
  }

  for (size_t i = 0; i < arg_count; i++) {
    if (!args[i]) {
      for (size_t j = 0; j < i; j++) {
        if (eval_args[j]) {
          slp_object_free(eval_args[j]);
        }
      }
      free(eval_args);
      return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                     "D builtin: nil argument", 0,
                                     runtime->source_buffer);
    }
    eval_args[i] = sxs_eval_object(runtime, args[i]);
    if (!eval_args[i]) {
      for (size_t j = 0; j < i; j++) {
        if (eval_args[j]) {
          slp_object_free(eval_args[j]);
        }
      }
      free(eval_args);
      return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                     "D builtin: eval failed", 0,
                                     runtime->source_buffer);
    }
    if (eval_args[i]->type == SLP_TYPE_ERROR) {
      slp_object_t *error = eval_args[i];
      for (size_t j = 0; j < i; j++) {
        if (eval_args[j]) {
          slp_object_free(eval_args[j]);
        }
      }
      free(eval_args);
      return error;
    }
  }

  for (size_t i = 0; i < arg_count; i++) {
    print_object_full(eval_args[i], i);
  }

  printf("========================================\n");

  for (size_t i = 0; i < arg_count; i++) {
    if (eval_args[i]) {
      slp_object_free(eval_args[i]);
    }
  }
  free(eval_args);

  slp_object_t *none = malloc(sizeof(slp_object_t));
  if (none) {
    none->type = SLP_TYPE_NONE;
    none->source_position = 0;
  }
  return none;
}
