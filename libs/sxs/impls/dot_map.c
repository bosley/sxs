#include "sxs/sxs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

slp_object_t *sxs_builtin_dot_map(sxs_runtime_t *runtime,
                                  sxs_callable_t *callable, slp_object_t **args,
                                  size_t arg_count) {
  if (!runtime) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   ".map builtin: nil runtime", 0, NULL);
  }

  if (arg_count != 2) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   ".map builtin: expected 2 arguments", 0,
                                   runtime->source_buffer);
  }

  if (!args[0] || args[0]->type != SLP_TYPE_SYMBOL) {
    size_t pos = args[0] ? args[0]->source_position : 0;
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   ".map builtin: first arg must be symbol",
                                   pos, runtime->source_buffer);
  }

  if (!args[1]) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   ".map builtin: nil second argument", 0,
                                   runtime->source_buffer);
  }

  if (!runtime->symbols) {
    return sxs_create_error_object(SLP_ERROR_PARSE_TOKEN,
                                   ".map builtin: nil symbols context", 0,
                                   runtime->source_buffer);
  }

  if (!args[0]->value.buffer || !args[0]->value.buffer->data) {
    return sxs_create_error_object(
        SLP_ERROR_PARSE_TOKEN, ".map builtin: symbol has nil buffer",
        args[0]->source_position, runtime->source_buffer);
  }

  char *symbol_name = malloc(args[0]->value.buffer->count + 1);
  if (!symbol_name) {
    return sxs_create_error_object(
        SLP_ERROR_ALLOCATION, ".map builtin: failed to allocate symbol name", 0,
        runtime->source_buffer);
  }
  memcpy(symbol_name, args[0]->value.buffer->data,
         args[0]->value.buffer->count);
  symbol_name[args[0]->value.buffer->count] = '\0';

  if (ctx_set(runtime->symbols, symbol_name, args[1]) != 0) {
    free(symbol_name);
    return sxs_create_error_object(
        SLP_ERROR_PARSE_TOKEN, ".map builtin: failed to set symbol in context",
        args[0]->source_position, runtime->source_buffer);
  }

  free(symbol_name);

  slp_object_t *none = malloc(sizeof(slp_object_t));
  if (!none) {
    return sxs_create_error_object(SLP_ERROR_ALLOCATION,
                                   ".map builtin: failed to allocate none", 0,
                                   runtime->source_buffer);
  }
  none->type = SLP_TYPE_NONE;
  none->source_position = 0;

  return none;
}
