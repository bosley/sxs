#include "sxs/sxs.h"
#include <stdio.h>
#include <stdlib.h>

sxs_context_t *sxs_context_new(size_t context_id, sxs_context_t *parent) {
  sxs_context_t *context = malloc(sizeof(sxs_context_t));
  if (!context) {
    fprintf(stderr, "Failed to create context (nil)\n");
    return NULL;
  }

  context->context_id = context_id;
  context->parent = parent;

  return context;
}

void sxs_context_free(sxs_context_t *context) {
  if (!context) {
    fprintf(stderr, "Failed to free sxs context (nil)\n");
    return;
  }
  free(context);
}