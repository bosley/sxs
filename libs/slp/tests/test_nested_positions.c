#include "../slp.h"
#include "test.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
  slp_object_t **objects;
  size_t count;
  size_t capacity;
} test_context_t;

static void test_on_object(slp_object_t *object, void *context) {
  test_context_t *ctx = (test_context_t *)context;

  if (ctx->count >= ctx->capacity) {
    ctx->capacity = ctx->capacity == 0 ? 8 : ctx->capacity * 2;
    ctx->objects =
        realloc(ctx->objects, sizeof(slp_object_t *) * ctx->capacity);
  }

  ctx->objects[ctx->count++] = slp_object_copy(object);
}

static void test_on_list_start(slp_type_e list_type, void *context) {}

static void test_on_list_end(slp_type_e list_type, void *context) {}

static void test_on_error(slp_error_type_e error_type, const char *message,
                          size_t position, slp_buffer_t *buffer,
                          void *context) {}

static test_context_t *test_context_new(void) {
  test_context_t *ctx = malloc(sizeof(test_context_t));
  ctx->objects = NULL;
  ctx->count = 0;
  ctx->capacity = 0;
  return ctx;
}

static void test_context_free(test_context_t *ctx) {
  if (!ctx)
    return;
  for (size_t i = 0; i < ctx->count; i++) {
    slp_object_free(ctx->objects[i]);
  }
  free(ctx->objects);
  free(ctx);
}

static slp_callbacks_t test_callbacks_create(test_context_t *ctx) {
  slp_callbacks_t callbacks = {.on_object = test_on_object,
                               .on_list_start = test_on_list_start,
                               .on_list_end = test_on_list_end,
                               .on_virtual_list_start = NULL,
                               .on_virtual_list_end = NULL,
                               .on_error = test_on_error,
                               .context = ctx};
  return callbacks;
}

static void test_single_level_positions(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(64);
  const char *input = "(a b c)";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->count, 3);

  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(ctx->objects[0]->source_position, 1);

  ASSERT_EQ(ctx->objects[1]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(ctx->objects[1]->source_position, 3);

  ASSERT_EQ(ctx->objects[2]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(ctx->objects[2]->source_position, 5);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_nested_list_positions(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(64);
  const char *input = "(a (b c) d)";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->count, 4);

  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(ctx->objects[0]->source_position, 1);

  ASSERT_EQ(ctx->objects[1]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(ctx->objects[1]->source_position, 4);

  ASSERT_EQ(ctx->objects[2]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(ctx->objects[2]->source_position, 6);

  ASSERT_EQ(ctx->objects[3]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(ctx->objects[3]->source_position, 9);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_deeply_nested_positions(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(64);
  const char *input = "(a (b (c d)))";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->count, 4);

  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(ctx->objects[0]->source_position, 1);

  ASSERT_EQ(ctx->objects[1]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(ctx->objects[1]->source_position, 4);

  ASSERT_EQ(ctx->objects[2]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(ctx->objects[2]->source_position, 7);

  ASSERT_EQ(ctx->objects[3]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(ctx->objects[3]->source_position, 9);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_multiline_nested_positions(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(128);
  const char *input = "(@ 0 [9 8 7 6 5])\n\n(D (@ d))";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);

  slp_object_t *d_symbol = NULL;
  for (size_t i = 0; i < ctx->count; i++) {
    if (ctx->objects[i]->type == SLP_TYPE_SYMBOL &&
        ctx->objects[i]->value.buffer &&
        ctx->objects[i]->value.buffer->count == 1 &&
        ctx->objects[i]->value.buffer->data[0] == 'd') {
      d_symbol = ctx->objects[i];
      break;
    }
  }

  ASSERT_NOT_NULL(d_symbol);

  size_t d_position = d_symbol->source_position;
  size_t line = 1;
  size_t col = 1;

  for (size_t i = 0; i < d_position && i < buffer->count; i++) {
    if (buffer->data[i] == '\n') {
      line++;
      col = 1;
    } else {
      col++;
    }
  }

  ASSERT_EQ(line, 3);
  ASSERT_EQ(col, 7);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_bracket_list_nested_positions(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(64);
  const char *input = "(a [b c])";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->count, 3);

  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(ctx->objects[0]->source_position, 1);

  ASSERT_EQ(ctx->objects[1]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(ctx->objects[1]->source_position, 4);

  ASSERT_EQ(ctx->objects[2]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(ctx->objects[2]->source_position, 6);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_integer_positions_in_nested_list(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(64);
  const char *input = "(@ 0 [9 8])";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->count, 4);

  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(ctx->objects[0]->source_position, 1);

  ASSERT_EQ(ctx->objects[1]->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(ctx->objects[1]->source_position, 3);

  ASSERT_EQ(ctx->objects[2]->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(ctx->objects[2]->source_position, 6);

  ASSERT_EQ(ctx->objects[3]->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(ctx->objects[3]->source_position, 8);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

int main(void) {
  test_single_level_positions();
  test_nested_list_positions();
  test_deeply_nested_positions();
  test_multiline_nested_positions();
  test_bracket_list_nested_positions();
  test_integer_positions_in_nested_list();

  return 0;
}
