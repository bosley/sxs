#include "sxs/sxs.h"
#include <stdio.h>
#include <stdlib.h>

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
    printf("(error)\n");
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

  sxs_runtime_t *runtime = sxs_runtime_new();
  if (!runtime) {
    fprintf(stderr, "Failed to create runtime\n");
    return 1;
  }

  int result = sxs_runtime_process_file(runtime, argv[1]);

  if (result != 0) {
    sxs_runtime_free(runtime);
    return result;
  }

  slp_object_t *final_object = sxs_runtime_get_last_eval_obj(runtime);

  printf("\n[FINAL RESULT]\n");
  print_object(final_object);

  if (final_object) {
    slp_free_object(final_object);
  }

  sxs_runtime_free(runtime);

  return 0;
}