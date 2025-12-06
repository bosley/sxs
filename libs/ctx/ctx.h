#ifndef SXS_CTX_H
#define SXS_CTX_H

#include "map/map.h"
#include "slp/slp.h"
#include <stdbool.h>

typedef struct ctx_s {
  struct ctx_s *parent;
  map_void_t data;
} ctx_t;

ctx_t *ctx_create(ctx_t *parent);

void ctx_free(ctx_t *ctx);

int ctx_set(ctx_t *ctx, const char *key, slp_object_t *obj);

slp_object_t *ctx_get(ctx_t *ctx, const char *key);

ctx_t *ctx_get_context_if_exists(ctx_t *ctx, const char *key,
                                 bool search_parents);

void ctx_remove(ctx_t *ctx, const char *key);

#endif
