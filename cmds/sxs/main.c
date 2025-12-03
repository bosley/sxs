#include "buffer/buffer.h"
#include <libtcc.h>
#include <stdio.h>

int print_byte_hex(uint8_t *byte, size_t idx) {
  printf("0x%02X\n", *byte);
  return 1;
}

/*
here we do the primoridal procesing bootstrap

*/

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <file>\n", argv[0]);
    return 1;
  }

  slp_buffer_t *buffer = slp_buffer_from_file(argv[1]);
  if (!buffer) {
    fprintf(stderr, "Failed to load file: %s\n", argv[1]);
    return 1;
  }

  slp_buffer_for_each(buffer, print_byte_hex);

  slp_buffer_destroy(buffer);
  return 0;
}