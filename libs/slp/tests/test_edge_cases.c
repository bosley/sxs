#include "../slp.h"
#include "test.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  slp_object_t **objects;
  size_t count;
  size_t capacity;
  size_t error_count;
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

static void test_on_error(slp_error_type_e error_type, const char *message,
                          size_t position, slp_buffer_t *buffer,
                          void *context) {
  test_context_t *ctx = (test_context_t *)context;
  ctx->error_count++;
}

static test_context_t *test_context_new(void) {
  test_context_t *ctx = malloc(sizeof(test_context_t));
  ctx->objects = NULL;
  ctx->count = 0;
  ctx->capacity = 0;
  ctx->error_count = 0;
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
                               .on_virtual_list_start = NULL,
                               .on_virtual_list_end = NULL,
                               .on_error = test_on_error,
                               .context = ctx};
  return callbacks;
}

static void test_very_long_symbol(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(2048);
  char long_symbol[1024];
  for (int i = 0; i < 1023; i++) {
    long_symbol[i] = 'a';
  }
  long_symbol[1023] = '\0';
  slp_buffer_copy_to(buffer, (uint8_t *)long_symbol, strlen(long_symbol));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 1);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(ctx->objects[0]->value.buffer->count, 1023);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_deeply_nested_lists(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(128);
  const char *input = "((((((a))))))";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 1);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_SYMBOL);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_mixed_whitespace(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(64);
  const char *input = "a\t\tb  \t  c\n\nd";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 4);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_adjacent_delimiters(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(64);
  const char *input = "()[]{}";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 0);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_large_positive_integer(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(64);
  const char *input = "9223372036854775807";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 1);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(ctx->objects[0]->value.integer, INT64_MAX);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_large_negative_integer(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(64);
  const char *input = "-9223372036854775808";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 1);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_INTEGER);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_very_small_real(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(64);
  const char *input = "0.000001";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 1);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_REAL);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_very_large_real(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(64);
  const char *input = "123456789.987654321";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 1);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_REAL);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_symbol_with_many_special_chars(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(64);
  const char *input = "foo-bar+baz*qux/test";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 1);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_SYMBOL);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_empty_nested_lists(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(64);
  const char *input = "(() [] {})";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 0);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

int main(void) {
  test_very_long_symbol();
  test_deeply_nested_lists();
  test_mixed_whitespace();
  test_adjacent_delimiters();
  test_large_positive_integer();
  test_large_negative_integer();
  test_very_small_real();
  test_very_large_real();
  test_symbol_with_many_special_chars();
  test_empty_nested_lists();

  return 0;
}
