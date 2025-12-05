#include "../slp.h"
#include "test.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
  size_t error_count;
  slp_error_type_e last_error_type;
  size_t last_error_position;
  char last_error_message[256];
} test_context_t;

static void test_on_object(slp_object_t *object, void *context) {}

static void test_on_list_start(slp_type_e list_type, void *context) {}

static void test_on_list_end(slp_type_e list_type, void *context) {}

static void test_on_error(slp_error_type_e error_type, const char *message,
                          size_t position, slp_buffer_t *buffer,
                          void *context) {
  test_context_t *ctx = (test_context_t *)context;
  ctx->error_count++;
  ctx->last_error_type = error_type;
  ctx->last_error_position = position;
  if (message) {
    strncpy(ctx->last_error_message, message,
            sizeof(ctx->last_error_message) - 1);
    ctx->last_error_message[sizeof(ctx->last_error_message) - 1] = '\0';
  }
}

static test_context_t *test_context_new(void) {
  test_context_t *ctx = malloc(sizeof(test_context_t));
  ctx->error_count = 0;
  ctx->last_error_type = SLP_ERROR_UNCLOSED_GROUP;
  ctx->last_error_position = 0;
  ctx->last_error_message[0] = '\0';
  return ctx;
}

static void test_context_free(test_context_t *ctx) { free(ctx); }

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

static void test_unclosed_paren_list(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(32);
  const char *input = "(a b c";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_NEQ(result, 0);
  ASSERT_EQ(ctx->error_count, 1);
  ASSERT_EQ(ctx->last_error_type, SLP_ERROR_UNCLOSED_GROUP);
  ASSERT_EQ(ctx->last_error_position, 0);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_unclosed_bracket_list(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(32);
  const char *input = "[x y";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_NEQ(result, 0);
  ASSERT_EQ(ctx->error_count, 1);
  ASSERT_EQ(ctx->last_error_type, SLP_ERROR_UNCLOSED_GROUP);
  ASSERT_EQ(ctx->last_error_position, 0);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_unclosed_curly_list(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(32);
  const char *input = "{foo";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_NEQ(result, 0);
  ASSERT_EQ(ctx->error_count, 1);
  ASSERT_EQ(ctx->last_error_type, SLP_ERROR_UNCLOSED_GROUP);
  ASSERT_EQ(ctx->last_error_position, 0);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_unclosed_string(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(32);
  const char *input = "\"hello";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_NEQ(result, 0);
  ASSERT_EQ(ctx->error_count, 1);
  ASSERT_EQ(ctx->last_error_type, SLP_ERROR_UNCLOSED_GROUP);
  ASSERT_EQ(ctx->last_error_position, 0);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_unclosed_quoted_paren_list(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(32);
  const char *input = "'(a b c";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_NEQ(result, 0);
  ASSERT_EQ(ctx->error_count, 1);
  ASSERT_EQ(ctx->last_error_type, SLP_ERROR_UNCLOSED_QUOTED_GROUP);
  ASSERT_EQ(ctx->last_error_position, 1);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_unclosed_quoted_bracket_list(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(32);
  const char *input = "'[x y";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_NEQ(result, 0);
  ASSERT_EQ(ctx->error_count, 1);
  ASSERT_EQ(ctx->last_error_type, SLP_ERROR_UNCLOSED_QUOTED_GROUP);
  ASSERT_EQ(ctx->last_error_position, 1);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_error_position_in_middle_of_buffer(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(64);
  const char *input = "foo bar (baz";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_NEQ(result, 0);
  ASSERT_EQ(ctx->error_count, 1);
  ASSERT_EQ(ctx->last_error_type, SLP_ERROR_UNCLOSED_GROUP);
  ASSERT_EQ(ctx->last_error_position, 8);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_nested_unclosed_list(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(64);
  const char *input = "(a [b c)";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  int result = slp_process_buffer(buffer, &callbacks);

  ASSERT_NEQ(result, 0);
  ASSERT_EQ(ctx->error_count, 1);
  ASSERT_EQ(ctx->last_error_type, SLP_ERROR_UNCLOSED_GROUP);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

int main(void) {
  test_unclosed_paren_list();
  test_unclosed_bracket_list();
  test_unclosed_curly_list();
  test_unclosed_string();
  test_unclosed_quoted_paren_list();
  test_unclosed_quoted_bracket_list();
  test_error_position_in_middle_of_buffer();
  test_nested_unclosed_list();

  return 0;
}
