#include "../slp.h"
#include "test.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
  slp_object_t **objects;
  size_t count;
  size_t capacity;
  size_t virtual_list_start_count;
  size_t virtual_list_end_count;
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

static void test_on_virtual_list_start(void *context) {
  test_context_t *ctx = (test_context_t *)context;
  ctx->virtual_list_start_count++;
}

static void test_on_virtual_list_end(void *context) {
  test_context_t *ctx = (test_context_t *)context;
  ctx->virtual_list_end_count++;
}

static void test_on_error(slp_error_type_e error_type, const char *message,
                          size_t position, slp_buffer_t *buffer,
                          void *context) {}

static test_context_t *test_context_new(void) {
  test_context_t *ctx = malloc(sizeof(test_context_t));
  ctx->objects = NULL;
  ctx->count = 0;
  ctx->capacity = 0;
  ctx->virtual_list_start_count = 0;
  ctx->virtual_list_end_count = 0;
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
                               .on_list_start = NULL,
                               .on_list_end = NULL,
                               .on_virtual_list_start =
                                   test_on_virtual_list_start,
                               .on_virtual_list_end = test_on_virtual_list_end,
                               .on_error = test_on_error,
                               .context = ctx};
  return callbacks;
}

static void test_single_line_virtual_list(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(32);
  const char *input = "foo bar baz";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->virtual_list_start_count, 1);
  ASSERT_EQ(ctx->virtual_list_end_count, 1);
  ASSERT_EQ(ctx->count, 3);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_multiple_virtual_lists_separated_by_newlines(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(64);
  const char *input = "foo bar\nbaz qux";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->virtual_list_start_count, 2);
  ASSERT_EQ(ctx->virtual_list_end_count, 2);
  ASSERT_EQ(ctx->count, 4);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_virtual_list_with_nested_explicit_list(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);
  callbacks.on_list_start = NULL;
  callbacks.on_list_end = NULL;

  slp_buffer_t *buffer = slp_buffer_new(64);
  const char *input = "foo (bar baz)";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->virtual_list_start_count, 1);
  ASSERT_EQ(ctx->virtual_list_end_count, 1);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_virtual_list_ends_at_newline(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(64);
  const char *input = "a b c\n";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->virtual_list_start_count, 1);
  ASSERT_EQ(ctx->virtual_list_end_count, 1);
  ASSERT_EQ(ctx->count, 3);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

int main(void) {
  test_single_line_virtual_list();
  test_multiple_virtual_lists_separated_by_newlines();
  test_virtual_list_with_nested_explicit_list();
  test_virtual_list_ends_at_newline();

  return 0;
}
