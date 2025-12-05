#include "sxs/sxs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern slp_callbacks_t *sxs_runtime_get_callbacks(sxs_runtime_t *runtime);

sxs_context_t *sxs_context_new(size_t context_id, sxs_context_t *parent) {
  sxs_context_t *context = malloc(sizeof(sxs_context_t));
  if (!context) {
    fprintf(stderr, "Failed to create context (nil)\n");
    return NULL;
  }

  context->context_id = context_id;
  context->parent = parent;
  context->proc_list_count = 0;

  for (size_t i = 0; i < SXS_OBJECT_PROC_LIST_SIZE; i++) {
    context->object_proc_list[i] = NULL;
  }

  return context;
}

void sxs_context_free(sxs_context_t *context) {
  if (!context) {
    fprintf(stderr, "Failed to free sxs context (nil)\n");
    return;
  }

  for (size_t i = 0; i < context->proc_list_count; i++) {
    if (context->object_proc_list[i]) {
      slp_free_object(context->object_proc_list[i]);
    }
  }

  free(context);
}

sxs_runtime_t *sxs_runtime_new(void) {
  sxs_runtime_t *runtime = malloc(sizeof(sxs_runtime_t));
  if (!runtime) {
    fprintf(stderr, "Failed to create runtime (nil)\n");
    return NULL;
  }

  runtime->current_context = sxs_context_new(0, NULL);
  if (!runtime->current_context) {
    free(runtime);
    return NULL;
  }

  runtime->next_context_id = 1;

  for (size_t i = 0; i < SXS_OBJECT_STORAGE_SIZE; i++) {
    runtime->object_storage[i] = NULL;
  }

  return runtime;
}

void sxs_runtime_free(sxs_runtime_t *runtime) {
  if (!runtime) {
    return;
  }

  if (runtime->current_context) {
    sxs_context_free(runtime->current_context);
  }

  for (size_t i = 0; i < SXS_OBJECT_STORAGE_SIZE; i++) {
    if (runtime->object_storage[i]) {
      slp_free_object(runtime->object_storage[i]);
    }
  }

  free(runtime);
}

int sxs_runtime_process_file(sxs_runtime_t *runtime, char *file_name) {
  if (!runtime) {
    fprintf(stderr, "Failed to process file (nil runtime)\n");
    return 1;
  }

  slp_callbacks_t *callbacks = sxs_runtime_get_callbacks(runtime);
  if (!callbacks) {
    fprintf(stderr, "Failed to get callbacks\n");
    return 1;
  }

  return slp_process_file(file_name, callbacks);
}

/*
=========================


*/
