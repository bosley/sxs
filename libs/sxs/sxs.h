#ifndef SXS_SXS_H
#define SXS_SXS_H

#include "forms.h"
#include "map/map.h"
#include "slp/slp.h"
#include <stdbool.h>
#include <stddef.h>

#define SXS_OBJECT_PROC_LIST_SIZE 16
#define SXS_OBJECT_STORAGE_SIZE 8192
#define SXS_CALLABLE_MAX_VARIANTS 5

typedef struct sxs_runtime_s sxs_runtime_t;
typedef struct sxs_callable_s sxs_callable_t;
typedef struct sxs_builtin_registry_s sxs_builtin_registry_t;
typedef struct sxs_typecheck_context_s sxs_typecheck_context_t;

typedef slp_object_t *(*sxs_builtin_fn)(sxs_runtime_t *runtime,
                                        sxs_callable_t *callable,
                                        slp_object_t **args, size_t arg_count);

typedef sxs_builtin_fn sxs_handler_fn_t;

typedef int (*sxs_typecheck_fn)(sxs_typecheck_context_t *ctx,
                                sxs_callable_t *callable, slp_object_t **args,
                                size_t arg_count);

typedef struct sxs_command_impl_s {
  const char *command;
  sxs_handler_fn_t handler;
} sxs_command_impl_t;

struct sxs_builtin_registry_s {
  map_void_t command_map;
};

typedef struct sxs_callable_param_s {
  char *name;
  form_definition_t *form;
} sxs_callable_param_t;

typedef struct sxs_callable_variant_s {
  sxs_callable_param_t *params;
  size_t param_count;
  form_definition_t *return_type;
} sxs_callable_variant_t;

struct sxs_callable_s {
  const char *name;
  sxs_callable_variant_t variants[SXS_CALLABLE_MAX_VARIANTS];
  size_t variant_count;
  bool is_builtin;
  union {
    sxs_builtin_fn builtin_fn;
    slp_object_t *lambda_body;
  } impl;
  sxs_typecheck_fn typecheck_fn;
};

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
  bool exception_active;
  bool parsing_quoted_expression;
  slp_buffer_unowned_ptr_t source_buffer;
  sxs_builtin_registry_t *builtin_registry;
} sxs_runtime_t;

sxs_builtin_registry_t *sxs_builtin_registry_create(size_t initial_capacity);
void sxs_builtin_registry_free(sxs_builtin_registry_t *registry);
int sxs_builtin_registry_add(sxs_builtin_registry_t *registry,
                             sxs_command_impl_t impl);
sxs_command_impl_t *
sxs_builtin_registry_lookup(sxs_builtin_registry_t *registry,
                            slp_buffer_t *symbol);

sxs_runtime_t *sxs_runtime_new(sxs_builtin_registry_t *registry);
void sxs_runtime_free(sxs_runtime_t *runtime);

int sxs_runtime_process_file(sxs_runtime_t *runtime, char *file_name);
slp_object_t *sxs_runtime_get_last_eval_obj(sxs_runtime_t *runtime);

slp_object_t *sxs_eval_object(sxs_runtime_t *runtime, slp_object_t *object);
slp_object_t *sxs_create_error_object(slp_error_type_e error_type,
                                      const char *message, size_t position,
                                      slp_buffer_unowned_ptr_t source_buffer);

void sxs_builtins_init(void);
void sxs_builtins_deinit(void);

void sxs_callable_free(sxs_callable_t *callable);
sxs_callable_t *sxs_get_callable_for_handler(sxs_handler_fn_t handler);

slp_object_t *sxs_get_builtin_load_store_object(void);
slp_object_t *sxs_get_builtin_debug_object(void);
slp_object_t *sxs_get_builtin_rotl_object(void);
slp_object_t *sxs_get_builtin_rotr_object(void);
slp_object_t *sxs_get_builtin_insist_object(void);
slp_object_t *sxs_get_builtin_catch_object(void);
slp_object_t *sxs_get_builtin_proc_object(void);
slp_object_t *sxs_get_builtin_do_object(void);

#endif