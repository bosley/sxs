#include "sxs/sxs.h"
#include <stdio.h>
#include <stdlib.h>

/*
==========================================================================
builtin implementations
==========================================================================
*/

/*
function: @

acts as setter:     (@ <dest> <source>)
acts as getter:     (@ <source>)
acts as atomic CAS: (@ <dest> <compare_val> <source>)

Each argument is evaluated before checking for type

@ :int :int
@ :int
@ :int <any> :int


*/
static slp_object_t *sxs_builtin_load_store(sxs_runtime_t *runtime,
                                            slp_object_t **args,
                                            size_t arg_count) {
  printf("[BUILTIN LOAD_STORE] called with %zu args\n", arg_count);

  slp_object_t *none = malloc(sizeof(slp_object_t));
  if (none) {
    none->type = SLP_TYPE_INTEGER;
    none->value.integer = 42;
  }

  return none;
}

/*
==========================================================================
get builtins as objects
==========================================================================
*/
slp_object_t *
sxs_get_builtin_load_store_object_for_context(sxs_context_t *context) {
  if (!context) {
    fprintf(
        stderr,
        "Failed to get builtin load store object for context (nil context)\n");
    return NULL;
  }

  slp_object_t *builtin = malloc(sizeof(slp_object_t));
  if (!builtin) {
    fprintf(
        stderr,
        "Failed to get builtin load store object for context (nil builtin)\n");
    return NULL;
  }

  builtin->type = SLP_TYPE_BUILTIN;
  builtin->value.fn_data = (void *)sxs_builtin_load_store;

  return builtin;
}