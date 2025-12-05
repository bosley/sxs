#include "slp/slp.h"
#include "sxs/sxs.h"
#include <stdio.h>
#include <stdlib.h>

void handle_leaf(slp_object_t *object, void *context) {
  if (!object) {
    fprintf(stderr, "Failed to process object (nil)\n");
    return;
  }

  if (!context) {
    fprintf(stderr, "Failed to process object (nil context)\n");
    return;
  }

  sxs_context_t *sxs_context = (sxs_context_t *)context;

  switch (object->type) {
  case SLP_TYPE_INTEGER:
    printf("[INTEGER] %lld\n", object->value.integer);
    break;
  case SLP_TYPE_REAL:
    printf("[REAL] %f\n", object->value.real);
    break;
  case SLP_TYPE_SYMBOL:
    printf("[SYMBOL] ");
    if (object->value.buffer) {
      for (size_t i = 0; i < object->value.buffer->count; i++) {
        printf("%c", object->value.buffer->data[i]);
      }
    }
    printf("\n");

    break;
  case SLP_TYPE_QUOTED:

    // For quoted we can get an object by doing slp_process_buffer
    // with a different callback/context and we can get the new allocated
    // object

    printf("[QUOTED] ");
    if (object->value.buffer) {
      for (size_t i = 0; i < object->value.buffer->count; i++) {
        printf("%c", object->value.buffer->data[i]);
      }
    }
    printf("\n");
    break;
  default:
    printf("[UNKNOWN TYPE]\n");
    break;
  }

  slp_free_object(object);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <file>\n", argv[0]);
    return 1;
  }

  sxs_context_t *context = sxs_context_new(0, NULL);
  if (!context) {
    fprintf(stderr, "Failed to create context\n");
    return 1;
  }

  return slp_process_file(argv[1], handle_leaf, context);
}