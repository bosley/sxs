#include "sxs/sxs.h"
#include <stdlib.h>

slp_object_t *sxs_eval_object(sxs_runtime_t *runtime, slp_object_t *object) {
  if (!object) {
    fprintf(stderr, "Failed to eval object (nil)\n");
    return NULL;
  }

  switch (object->type) {
  case SLP_TYPE_INTEGER:
    printf("[EVAL INTEGER] %lld\n", object->value.integer);
    break;
  case SLP_TYPE_REAL:
    printf("[EVAL REAL] %f\n", object->value.real);
    break;
  case SLP_TYPE_SYMBOL:
    printf("[EVAL SYMBOL] ");
    if (object->value.buffer) {
      for (size_t i = 0; i < object->value.buffer->count; i++) {
        printf("%c", object->value.buffer->data[i]);
      }
    }
    printf("\n");
    break;
  case SLP_TYPE_QUOTED:
    printf("[EVAL QUOTED] ");
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
    printf("[EVAL LIST] count=%zu\n", object->value.list.count);
    break;
  default:
    printf("[EVAL UNKNOWN TYPE]\n");
    break;
  }

  slp_object_t *none = malloc(sizeof(slp_object_t));
  if (none) {
    none->type = SLP_TYPE_NONE;
  }
  return none;
}
