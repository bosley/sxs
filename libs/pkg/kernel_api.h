#ifndef SXS_KERNEL_API_H
#define SXS_KERNEL_API_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  SXS_TYPE_NONE = 0,
  SXS_TYPE_SOME = 1,
  SXS_TYPE_PAREN_LIST = 2,
  SXS_TYPE_BRACE_LIST = 4,
  SXS_TYPE_STRING = 5,
  SXS_TYPE_SYMBOL = 7,
  SXS_TYPE_RUNE = 8,
  SXS_TYPE_INT = 9,
  SXS_TYPE_REAL = 10,
  SXS_TYPE_BRACKET_LIST = 11,
  SXS_TYPE_ERROR = 12,
  SXS_TYPE_DATUM = 13,
  SXS_TYPE_ABERRANT = 14,
} sxs_type_t;

typedef void *sxs_registry_t;
typedef void *sxs_context_t;
typedef void *sxs_object_t;

typedef sxs_object_t (*sxs_kernel_fn_t)(sxs_context_t ctx, sxs_object_t args);

typedef void (*sxs_kernel_lifecycle_fn_t)(const struct sxs_api_table_t *api);

typedef void (*sxs_register_fn_t)(sxs_registry_t registry, const char *name,
                                  sxs_kernel_fn_t function,
                                  sxs_type_t return_type, int variadic);

typedef sxs_object_t (*sxs_eval_fn_t)(sxs_context_t ctx, sxs_object_t obj);
typedef sxs_type_t (*sxs_get_type_fn_t)(sxs_object_t obj);
typedef long long (*sxs_as_int_fn_t)(sxs_object_t obj);
typedef double (*sxs_as_real_fn_t)(sxs_object_t obj);
typedef const char *(*sxs_as_string_fn_t)(sxs_object_t obj);
typedef void *(*sxs_as_list_fn_t)(sxs_object_t obj);
typedef size_t (*sxs_list_size_fn_t)(void *list);
typedef sxs_object_t (*sxs_list_at_fn_t)(void *list, size_t index);
typedef sxs_object_t (*sxs_create_int_fn_t)(long long value);
typedef sxs_object_t (*sxs_create_real_fn_t)(double value);
typedef sxs_object_t (*sxs_create_string_fn_t)(const char *value);
typedef sxs_object_t (*sxs_create_none_fn_t)();
typedef const char *(*sxs_as_symbol_fn_t)(sxs_object_t obj);
typedef sxs_object_t (*sxs_create_symbol_fn_t)(const char *name);
typedef sxs_object_t (*sxs_create_paren_list_fn_t)(sxs_object_t *objects,
                                                   size_t count);
typedef sxs_object_t (*sxs_create_bracket_list_fn_t)(sxs_object_t *objects,
                                                     size_t count);
typedef sxs_object_t (*sxs_create_brace_list_fn_t)(sxs_object_t *objects,
                                                   size_t count);
typedef int (*sxs_some_has_value_fn_t)(sxs_object_t obj);
typedef sxs_object_t (*sxs_some_get_value_fn_t)(sxs_object_t obj);

struct sxs_api_table_t {
  sxs_register_fn_t register_function;
  sxs_eval_fn_t eval;
  sxs_get_type_fn_t get_type;
  sxs_as_int_fn_t as_int;
  sxs_as_real_fn_t as_real;
  sxs_as_string_fn_t as_string;
  sxs_as_list_fn_t as_list;
  sxs_list_size_fn_t list_size;
  sxs_list_at_fn_t list_at;
  sxs_create_int_fn_t create_int;
  sxs_create_real_fn_t create_real;
  sxs_create_string_fn_t create_string;
  sxs_create_none_fn_t create_none;
  sxs_as_symbol_fn_t as_symbol;
  sxs_create_symbol_fn_t create_symbol;
  sxs_create_paren_list_fn_t create_paren_list;
  sxs_create_bracket_list_fn_t create_bracket_list;
  sxs_create_brace_list_fn_t create_brace_list;
  sxs_some_has_value_fn_t some_has_value;
  sxs_some_get_value_fn_t some_get_value;
};

void kernel_init(sxs_registry_t registry, const struct sxs_api_table_t *api);

#ifdef __cplusplus
}
#endif

#endif
