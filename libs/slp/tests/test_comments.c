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

static void test_single_line_comment_with_code_after(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(64);
  const char *input = "; this is a comment\n42";
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

static void test_comment_at_end_of_line_after_code(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(64);
  const char *input = "42 ; this is a comment\n";
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

static void test_multiple_consecutive_comment_lines(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(128);
  const char *input = "; comment 1\n; comment 2\n; comment 3\n123";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 1);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(ctx->objects[0]->value.integer, 123);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_comment_in_virtual_list(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(128);
  const char *input = "1 ; comment\n2 ; another comment\n3";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 3);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(ctx->objects[1]->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(ctx->objects[2]->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(ctx->objects[0]->value.integer, 1);
  ASSERT_EQ(ctx->objects[1]->value.integer, 2);
  ASSERT_EQ(ctx->objects[2]->value.integer, 3);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_comment_inside_parenthesized_list(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(128);
  const char *input = "(1 ; comment\n2 3)";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 3);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(ctx->objects[1]->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(ctx->objects[2]->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(ctx->objects[0]->value.integer, 1);
  ASSERT_EQ(ctx->objects[1]->value.integer, 2);
  ASSERT_EQ(ctx->objects[2]->value.integer, 3);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_comment_inside_bracketed_list(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(128);
  const char *input = "[foo ; comment\nbar]";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 2);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(memcmp(ctx->objects[0]->value.buffer->data, "foo", 3), 0);
  ASSERT_EQ(ctx->objects[1]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(memcmp(ctx->objects[1]->value.buffer->data, "bar", 3), 0);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_comment_inside_curly_braced_list(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(128);
  const char *input = "{1.5 ; comment\n2.5}";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 2);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_REAL);
  ASSERT_TRUE(ctx->objects[0]->value.real > 1.4 &&
              ctx->objects[0]->value.real < 1.6);
  ASSERT_EQ(ctx->objects[1]->type, SLP_TYPE_REAL);
  ASSERT_TRUE(ctx->objects[1]->value.real > 2.4 &&
              ctx->objects[1]->value.real < 2.6);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_empty_comment(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(64);
  const char *input = ";\n42";
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

static void test_comment_with_special_characters(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(128);
  const char *input = "; ()[]{}\"'+-*/ special chars!\n99";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 1);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(ctx->objects[0]->value.integer, 99);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_mixed_whitespace_and_comments(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(128);
  const char *input = "  ; comment\n\t; another\n  \n  hello";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 1);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(memcmp(ctx->objects[0]->value.buffer->data, "hello", 5), 0);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_comment_before_quoted_expression(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(128);
  const char *input = "; comment\n'foo";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 1);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_QUOTED);
  ASSERT_EQ(memcmp(ctx->objects[0]->value.buffer->data, "foo", 3), 0);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_comment_after_quote_before_expression(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(128);
  const char *input = "' ; comment\nbar";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 1);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_QUOTED);
  ASSERT_EQ(memcmp(ctx->objects[0]->value.buffer->data, "bar", 3), 0);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_nested_lists_with_comments(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(256);
  const char *input =
      "(; outer comment\n(; inner comment\n1 2) ; end inner\n3)";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 3);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(ctx->objects[1]->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(ctx->objects[2]->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(ctx->objects[0]->value.integer, 1);
  ASSERT_EQ(ctx->objects[1]->value.integer, 2);
  ASSERT_EQ(ctx->objects[2]->value.integer, 3);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_comment_only_buffer(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(128);
  const char *input = "; just a comment\n; another comment\n";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 0);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_comment_without_newline_at_eof(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(128);
  const char *input = "42 ; comment at end";
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

static void test_multiple_values_with_inline_comments(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(256);
  const char *input = "1 ; first\n2 ; second\n3 ; third\n";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 3);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(ctx->objects[1]->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(ctx->objects[2]->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(ctx->objects[0]->value.integer, 1);
  ASSERT_EQ(ctx->objects[1]->value.integer, 2);
  ASSERT_EQ(ctx->objects[2]->value.integer, 3);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_comment_in_quoted_list(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(128);
  const char *input = "'(; comment\n1 2)";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_EQ(ctx->count, 1);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_QUOTED);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

int main(void) {
  test_single_line_comment_with_code_after();
  test_comment_at_end_of_line_after_code();
  test_multiple_consecutive_comment_lines();
  test_comment_in_virtual_list();
  test_comment_inside_parenthesized_list();
  test_comment_inside_bracketed_list();
  test_comment_inside_curly_braced_list();
  test_empty_comment();
  test_comment_with_special_characters();
  test_mixed_whitespace_and_comments();
  test_comment_before_quoted_expression();
  test_comment_after_quote_before_expression();
  test_nested_lists_with_comments();
  test_comment_only_buffer();
  test_comment_without_newline_at_eof();
  test_multiple_values_with_inline_comments();
  test_comment_in_quoted_list();

  return 0;
}
