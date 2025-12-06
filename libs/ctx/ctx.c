#include "ctx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ctx_t *ctx_create(ctx_t *parent) {
  ctx_t *ctx = malloc(sizeof(ctx_t));
  if (!ctx) {
    fprintf(stderr, "Failed to allocate context\n");
    return NULL;
  }

  ctx->parent = parent;
  map_init(&ctx->data);

  return ctx;
}

void ctx_free(ctx_t *ctx) {
  if (!ctx) {
    return;
  }

  map_iter_t iter = map_iter(&ctx->data);
  const char *key;
  while ((key = map_next(&ctx->data, &iter))) {
    void **value = map_get(&ctx->data, key);
    if (value && *value) {
      slp_object_t *obj = (slp_object_t *)*value;
      slp_object_free(obj);
    }
  }

  map_deinit(&ctx->data);
  free(ctx);
}

int ctx_set(ctx_t *ctx, const char *key, slp_object_t *obj) {
  if (!ctx || !key || !obj) {
    return -1;
  }

  void **existing = map_get(&ctx->data, key);
  if (existing && *existing) {
    slp_object_t *old_obj = (slp_object_t *)*existing;
    slp_object_free(old_obj);
  }

  slp_object_t *obj_copy = slp_object_copy(obj);
  if (!obj_copy) {
    fprintf(stderr, "Failed to copy object for ctx_set\n");
    return -1;
  }

  if (map_set(&ctx->data, key, obj_copy) != 0) {
    slp_object_free(obj_copy);
    return -1;
  }

  return 0;
}

slp_object_t *ctx_get(ctx_t *ctx, const char *key) {
  if (!ctx || !key) {
    return NULL;
  }

  void **value = map_get(&ctx->data, key);
  if (!value) {
    return NULL;
  }

  return (slp_object_t *)*value;
}

ctx_t *ctx_get_context_if_exists(ctx_t *ctx, const char *key,
                                 bool search_parents) {
  if (!ctx || !key) {
    return NULL;
  }

  void **value = map_get(&ctx->data, key);
  if (value) {
    return ctx;
  }

  if (search_parents && ctx->parent) {
    return ctx_get_context_if_exists(ctx->parent, key, search_parents);
  }

  return NULL;
}

void ctx_remove(ctx_t *ctx, const char *key) {
  if (!ctx || !key) {
    return;
  }

  void **value = map_get(&ctx->data, key);
  if (value && *value) {
    slp_object_t *obj = (slp_object_t *)*value;
    slp_object_free(obj);
  }

  map_remove(&ctx->data, key);
}
