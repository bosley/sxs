#ifndef SXS_SXS_H
#define SXS_SXS_H

#include "slp/slp.h"
#include "forms.h"
#include <stdbool.h>
#include <stddef.h>

#define SXS_OBJECT_PROC_LIST_SIZE 16
#define SXS_OBJECT_STORAGE_SIZE 8192

#define SXS_BUILTIN_LOAD_STORE_SYMBOL '@'

typedef struct sxs_runtime_s sxs_runtime_t;

typedef slp_object_t *(*sxs_builtin_fn)(sxs_runtime_t *runtime,
                                        slp_object_t **args, size_t arg_count);

typedef struct sxs_callable_param_s {
  char *name;
  form_definition_t *form;
} sxs_callable_param_t;

typedef struct sxs_callable_s {
  sxs_callable_param_t *params;
  size_t param_count;
  union {
    sxs_builtin_fn builtin_fn;
    slp_buffer_t *lambda_body;
  } impl;
} sxs_callable_t;

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
  bool runtime_has_error;
} sxs_runtime_t;

sxs_runtime_t *sxs_runtime_new(void);
void sxs_runtime_free(sxs_runtime_t *runtime);

int sxs_runtime_process_file(sxs_runtime_t *runtime, char *file_name);
slp_object_t *sxs_runtime_get_last_eval_obj(sxs_runtime_t *runtime);

void sxs_callable_free(sxs_callable_t *callable);

#endif