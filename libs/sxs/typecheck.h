#ifndef SXS_TYPECHECK_H
#define SXS_TYPECHECK_H

#include "forms.h"
#include "slp/slp.h"
#include "sxs.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct sxs_typecheck_context_s sxs_typecheck_context_t;

typedef int (*sxs_typecheck_fn)(sxs_typecheck_context_t *ctx,
                                sxs_callable_t *callable, slp_object_t **args,
                                size_t arg_count);

typedef struct sxs_typecheck_error_s {
  char *message;
  size_t position;
  char *function_name;
  char *expected_signature;
  char *received_signature;
} sxs_typecheck_error_t;

typedef struct sxs_typecheck_context_stack_s {
  size_t context_id;
  struct sxs_typecheck_context_stack_s *parent;
  slp_object_t *object_proc_list[SXS_OBJECT_PROC_LIST_SIZE];
  size_t proc_list_count;
  form_definition_t *result_type;
} sxs_typecheck_context_stack_t;

struct sxs_typecheck_context_s {
  sxs_typecheck_context_stack_t *current_context;
  size_t next_context_id;
  form_definition_t *register_types[SXS_OBJECT_STORAGE_SIZE];
  sxs_builtin_registry_t *builtin_registry;
  slp_buffer_unowned_ptr_t source_buffer;
  sxs_typecheck_error_t *errors;
  size_t error_count;
  size_t error_capacity;
  bool has_error;
  bool parsing_quoted_expression;
};

sxs_typecheck_context_t *
sxs_typecheck_context_create(sxs_builtin_registry_t *registry);
void sxs_typecheck_context_free(sxs_typecheck_context_t *ctx);

void sxs_typecheck_add_error(sxs_typecheck_context_t *ctx, const char *message,
                             size_t position);
void sxs_typecheck_add_detailed_error(sxs_typecheck_context_t *ctx,
                                      const char *message, size_t position,
                                      const char *function_name,
                                      const char *expected_sig,
                                      const char *received_sig);
void sxs_typecheck_print_errors(sxs_typecheck_context_t *ctx);

form_definition_t *sxs_typecheck_object(sxs_typecheck_context_t *ctx,
                                        slp_object_t *object);
form_definition_t *sxs_typecheck_list(sxs_typecheck_context_t *ctx,
                                      slp_object_t *list);

int sxs_typecheck_file(const char *filename, sxs_builtin_registry_t *registry,
                       sxs_typecheck_context_t **out_ctx);

slp_callbacks_t *sxs_typecheck_get_callbacks(sxs_typecheck_context_t *ctx);

int sxs_typecheck_generic(sxs_typecheck_context_t *ctx,
                          sxs_callable_t *callable, slp_object_t **args,
                          size_t arg_count);

#endif
