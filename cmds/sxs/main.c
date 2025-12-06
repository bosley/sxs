#include "sxs/impls/impls.h"
#include "sxs/sxs.h"
#include <stdio.h>
#include <stdlib.h>

/*
I moved these to print.c so they wouldn't make the main impl muddy
as they are just simple helper functions anywya
*/
extern void print_object(slp_object_t *object);
extern void print_source_context(slp_buffer_t *buffer, size_t error_position);
extern void print_error(slp_object_t *object);

/*
sxs uses slp and slp buffers to make runtime objects that are then evaluated
the slp has a simple state machine that consumes a lisp-like language that
permits an optinal "outer" set of parens. see "min.sxs" for minimal examples
*/
int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <file>\n", argv[0]);
    return 1;
  }

  sxs_builtins_init();

  sxs_builtin_registry_t *registry = sxs_builtin_registry_create(0);
  if (!registry) {
    fprintf(stderr, "Failed to create builtin registry\n");
    sxs_builtins_deinit();
    return 1;
  }

  sxs_builtin_registry_add(registry, sxs_impl_get_load_store());
  sxs_builtin_registry_add(registry, sxs_impl_get_debug());
  sxs_builtin_registry_add(registry, sxs_impl_get_rotl());
  sxs_builtin_registry_add(registry, sxs_impl_get_rotr());

  sxs_runtime_t *runtime = sxs_runtime_new(registry);
  if (!runtime) {
    fprintf(stderr, "Failed to create runtime\n");
    sxs_builtin_registry_free(registry);
    sxs_builtins_deinit();
    return 1;
  }

  int result = sxs_runtime_process_file(runtime, argv[1]);

  if (result != 0) {
    sxs_runtime_free(runtime);
    sxs_builtins_deinit();
    return result;
  }

  slp_object_t *final_object = sxs_runtime_get_last_eval_obj(runtime);

  printf("\n[FINAL RESULT]\n");
  print_object(final_object);

  if (final_object) {
    slp_object_free(final_object);
  }

  sxs_runtime_free(runtime);

  sxs_builtins_deinit();

  return 0;
}
