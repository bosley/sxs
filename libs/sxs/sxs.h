#ifndef SXS_SXS_H
#define SXS_SXS_H

#include "slp/slp.h"
#include <stddef.h>

#define SXS_OBJECT_STORAGE_SIZE 1024

typedef enum sxs_opcode_e {
  SXS_OPCODE_NOP = 0,
  SXS_OPCODE_LOADSTORE, // @
} sxs_opcode_e;

typedef struct sxs_context_s {
  size_t context_id;
  struct sxs_context_s *parent;

  sxs_opcode_e current_opcode;
  slp_object_t *object_storage[SXS_OBJECT_STORAGE_SIZE];
} sxs_context_t;

sxs_context_t *sxs_context_new(size_t context_id, sxs_context_t *parent);
void sxs_context_free(sxs_context_t *context);

#endif