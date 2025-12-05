#include "sxs/sxs.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <file>\n", argv[0]);
    return 1;
  }

  sxs_runtime_t *runtime = sxs_runtime_new();
  if (!runtime) {
    fprintf(stderr, "Failed to create runtime\n");
    return 1;
  }

  int result = sxs_runtime_process_file(runtime, argv[1]);

  sxs_runtime_free(runtime);

  return result;
}