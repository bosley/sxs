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
  slp_type_e last_list_type;
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
  ctx->last_list_type = list_type;
}

static void test_on_list_end(slp_type_e list_type, void *context) {
  test_context_t *ctx = (test_context_t *)context;
  ctx->list_end_count++;
}

static void test_on_error(slp_error_type_e error_type, const char *message,
                          size_t position, slp_buffer_t *buffer,
                          void *context) {}

static test_context_t *test_context_new(void) {
  test_context_t *ctx = malloc(sizeof(test_context_t));
  ctx->objects = NULL;
  ctx->count = 0;
  ctx->capacity = 0;
  ctx->list_start_count = 0;
  ctx->list_end_count = 0;
  ctx->last_list_type = SLP_TYPE_NONE;
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

static void test_parse_empty_paren_list(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(16);
  const char *input = "()";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->list_start_count, 1);
  ASSERT_EQ(ctx->list_end_count, 1);
  ASSERT_EQ(ctx->last_list_type, SLP_TYPE_LIST_P);
  ASSERT_EQ(ctx->count, 0);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_parse_empty_bracket_list(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(16);
  const char *input = "[]";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->list_start_count, 1);
  ASSERT_EQ(ctx->list_end_count, 1);
  ASSERT_EQ(ctx->last_list_type, SLP_TYPE_LIST_B);
  ASSERT_EQ(ctx->count, 0);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_parse_empty_curly_list(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(16);
  const char *input = "{}";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->list_start_count, 1);
  ASSERT_EQ(ctx->list_end_count, 1);
  ASSERT_EQ(ctx->last_list_type, SLP_TYPE_LIST_C);
  ASSERT_EQ(ctx->count, 0);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_parse_paren_list_with_symbols(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(32);
  const char *input = "(a b c)";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->list_start_count, 1);
  ASSERT_EQ(ctx->list_end_count, 1);
  ASSERT_EQ(ctx->count, 3);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(ctx->objects[1]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(ctx->objects[2]->type, SLP_TYPE_SYMBOL);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_parse_bracket_list_with_integers(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(32);
  const char *input = "[1 2 3]";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->list_start_count, 1);
  ASSERT_EQ(ctx->list_end_count, 1);
  ASSERT_EQ(ctx->count, 3);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(ctx->objects[0]->value.integer, 1);
  ASSERT_EQ(ctx->objects[1]->value.integer, 2);
  ASSERT_EQ(ctx->objects[2]->value.integer, 3);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_parse_curly_list_with_mixed(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(32);
  const char *input = "{x 42 y}";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->list_start_count, 1);
  ASSERT_EQ(ctx->list_end_count, 1);
  ASSERT_EQ(ctx->count, 3);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(ctx->objects[1]->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(ctx->objects[2]->type, SLP_TYPE_SYMBOL);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_parse_string_list(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(32);
  const char *input = "\"hello world\"";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->list_start_count, 1);
  ASSERT_EQ(ctx->list_end_count, 1);
  ASSERT_EQ(ctx->last_list_type, SLP_TYPE_LIST_S);
  ASSERT_EQ(ctx->count, 2);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(ctx->objects[1]->type, SLP_TYPE_SYMBOL);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_parse_empty_string(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(16);
  const char *input = "\"\"";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->list_start_count, 1);
  ASSERT_EQ(ctx->list_end_count, 1);
  ASSERT_EQ(ctx->count, 0);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_parse_nested_lists_simple(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(32);
  const char *input = "(a [b])";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->list_start_count, 2);
  ASSERT_EQ(ctx->list_end_count, 2);
  ASSERT_EQ(ctx->count, 2);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(ctx->objects[1]->type, SLP_TYPE_SYMBOL);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_parse_nested_lists_complex(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(64);
  const char *input = "(a [b {c}])";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->list_start_count, 3);
  ASSERT_EQ(ctx->list_end_count, 3);
  ASSERT_EQ(ctx->count, 3);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(ctx->objects[1]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(ctx->objects[2]->type, SLP_TYPE_SYMBOL);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_parse_deeply_nested_lists(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(64);
  const char *input = "(((a)))";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->list_start_count, 3);
  ASSERT_EQ(ctx->list_end_count, 3);
  ASSERT_EQ(ctx->count, 1);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_SYMBOL);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_parse_adjacent_lists(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(64);
  const char *input = "()[]{}";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->list_start_count, 3);
  ASSERT_EQ(ctx->list_end_count, 3);
  ASSERT_EQ(ctx->count, 0);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_parse_list_with_whitespace(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(64);
  const char *input = "(  a   b\t\tc  )";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->count, 3);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

int main(void) {
  test_parse_empty_paren_list();
  test_parse_empty_bracket_list();
  test_parse_empty_curly_list();
  test_parse_paren_list_with_symbols();
  test_parse_bracket_list_with_integers();
  test_parse_curly_list_with_mixed();
  test_parse_string_list();
  test_parse_empty_string();
  test_parse_nested_lists_simple();
  test_parse_nested_lists_complex();
  test_parse_deeply_nested_lists();
  test_parse_adjacent_lists();
  test_parse_list_with_whitespace();

  return 0;
}
