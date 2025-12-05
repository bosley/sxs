#include "../slp.h"
#include "test.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
  slp_object_t **objects;
  size_t count;
  size_t capacity;
  size_t list_start_count;
  size_t list_end_count;
  size_t virtual_list_start_count;
  size_t virtual_list_end_count;
  size_t error_count;
  slp_error_type_e last_error_type;
  size_t last_error_position;
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

static void test_on_list_start(slp_type_e list_type, void *context) {
  test_context_t *ctx = (test_context_t *)context;
  ctx->list_start_count++;
}

static void test_on_list_end(slp_type_e list_type, void *context) {
  test_context_t *ctx = (test_context_t *)context;
  ctx->list_end_count++;
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
                          void *context) {
  test_context_t *ctx = (test_context_t *)context;
  ctx->error_count++;
  ctx->last_error_type = error_type;
  ctx->last_error_position = position;
}

static test_context_t *test_context_new(void) {
  test_context_t *ctx = malloc(sizeof(test_context_t));
  ctx->objects = NULL;
  ctx->count = 0;
  ctx->capacity = 0;
  ctx->list_start_count = 0;
  ctx->list_end_count = 0;
  ctx->virtual_list_start_count = 0;
  ctx->virtual_list_end_count = 0;
  ctx->error_count = 0;
  ctx->last_error_type = SLP_ERROR_UNCLOSED_GROUP;
  ctx->last_error_position = 0;
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
                               .on_virtual_list_start =
                                   test_on_virtual_list_start,
                               .on_virtual_list_end = test_on_virtual_list_end,
                               .on_error = test_on_error,
                               .context = ctx};
  return callbacks;
}

static void test_parse_positive_integer(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(16);
  const char *input = "42";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 1);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(ctx->objects[0]->value.integer, 42);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_parse_negative_integer(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(16);
  const char *input = "-123";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 1);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(ctx->objects[0]->value.integer, -123);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_parse_zero(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(16);
  const char *input = "0";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 1);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(ctx->objects[0]->value.integer, 0);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_parse_real_number(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(16);
  const char *input = "3.14";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 1);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_REAL);
  ASSERT_TRUE(ctx->objects[0]->value.real > 3.13 &&
              ctx->objects[0]->value.real < 3.15);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_parse_negative_real(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(16);
  const char *input = "-2.5";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 1);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_REAL);
  ASSERT_TRUE(ctx->objects[0]->value.real > -2.6 &&
              ctx->objects[0]->value.real < -2.4);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_parse_simple_symbol(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(16);
  const char *input = "hello";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 1);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_SYMBOL);
  ASSERT_NOT_NULL(ctx->objects[0]->value.buffer);
  ASSERT_EQ(ctx->objects[0]->value.buffer->count, 5);
  ASSERT_EQ(memcmp(ctx->objects[0]->value.buffer->data, "hello", 5), 0);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_parse_symbol_with_special_chars(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(16);
  const char *input = "foo-bar+baz";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 1);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(ctx->objects[0]->value.buffer->count, 11);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_parse_multiple_integers(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(32);
  const char *input = "1 2 3";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 3);
  ASSERT_EQ(ctx->objects[0]->value.integer, 1);
  ASSERT_EQ(ctx->objects[1]->value.integer, 2);
  ASSERT_EQ(ctx->objects[2]->value.integer, 3);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_parse_empty_buffer(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(16);

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 0);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_parse_whitespace_only(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(16);
  const char *input = "   \t\n  ";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 0);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

int main(void) {
  test_parse_positive_integer();
  test_parse_negative_integer();
  test_parse_zero();
  test_parse_real_number();
  test_parse_negative_real();
  test_parse_simple_symbol();
  test_parse_symbol_with_special_chars();
  test_parse_multiple_integers();
  test_parse_empty_buffer();
  test_parse_whitespace_only();

  return 0;
}
