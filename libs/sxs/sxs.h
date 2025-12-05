#ifndef SXS_SXS_H
#define SXS_SXS_H

#include "slp/slp.h"
#include <stddef.h>

#define SXS_OBJECT_PROC_LIST_SIZE 16
#define SXS_OBJECT_STORAGE_SIZE 8192

#define SXS_BUILTIN_LOAD_STORE_SYMBOL '@'

typedef struct sxs_context_s {
  size_t context_id;
  struct sxs_context_s *parent;

  slp_object_t *object_proc_list[SXS_OBJECT_PROC_LIST_SIZE];
  size_t proc_list_count;
} sxs_context_t;

typedef struct sxs_runtime_s {
  sxs_context_t *current_context;
  size_t next_context_id;
  slp_object_t *object_storage[SXS_OBJECT_STORAGE_SIZE];
} sxs_runtime_t;

sxs_runtime_t *sxs_runtime_new(void);
void sxs_runtime_free(sxs_runtime_t *runtime);

int sxs_runtime_process_file(sxs_runtime_t *runtime, char *file_name);

#endif