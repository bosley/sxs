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
                               .on_list_start = NULL,
                               .on_list_end = NULL,
                               .on_virtual_list_start = NULL,
                               .on_virtual_list_end = NULL,
                               .on_error = NULL,
                               .context = ctx};
  return callbacks;
}

static void test_empty_lists_create_no_objects(void) {
  const char *inputs[] = {"()", "[]", "{}", "\"\""};

  for (size_t i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++) {
    test_context_t *ctx = test_context_new();
    slp_callbacks_t callbacks = test_callbacks_create(ctx);

    slp_buffer_t *buffer = slp_buffer_new(16);
    slp_buffer_copy_to(buffer, (uint8_t *)inputs[i], strlen(inputs[i]));

    slp_process_buffer(buffer, &callbacks);

    ASSERT_EQ(ctx->count, 0);

    slp_buffer_free(buffer);
    test_context_free(ctx);
  }
}

static void test_string_buffer_contents(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(32);
  const char *input = "\"hello world\"";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(ctx->count, 2);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(ctx->objects[1]->type, SLP_TYPE_SYMBOL);

  ASSERT_EQ(ctx->objects[0]->value.buffer->count, 5);
  ASSERT_EQ(memcmp(ctx->objects[0]->value.buffer->data, "hello", 5), 0);

  ASSERT_EQ(ctx->objects[1]->value.buffer->count, 5);
  ASSERT_EQ(memcmp(ctx->objects[1]->value.buffer->data, "world", 5), 0);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_object_equality_simple(void) {
  slp_object_t *obj1 = malloc(sizeof(slp_object_t));
  slp_object_t *obj2 = malloc(sizeof(slp_object_t));

  obj1->type = SLP_TYPE_INTEGER;
  obj1->value.integer = 42;
  obj1->source_position = 0;

  obj2->type = SLP_TYPE_INTEGER;
  obj2->value.integer = 42;
  obj2->source_position = 0;

  ASSERT_TRUE(slp_objects_equal(obj1, obj2));

  obj2->value.integer = 43;
  ASSERT_FALSE(slp_objects_equal(obj1, obj2));

  free(obj1);
  free(obj2);
}

static void test_copy_independence_simple(void) {
  slp_object_t *original = malloc(sizeof(slp_object_t));
  original->type = SLP_TYPE_INTEGER;
  original->value.integer = 100;
  original->source_position = 0;

  slp_object_t *copy = slp_object_copy(original);

  ASSERT_EQ(copy->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(copy->value.integer, 100);
  ASSERT_TRUE(slp_objects_equal(original, copy));

  original->value.integer = 200;

  ASSERT_EQ(copy->value.integer, 100);
  ASSERT_FALSE(slp_objects_equal(original, copy));

  free(original);
  slp_object_free(copy);
}

static void test_list_copy_deep(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(32);
  const char *input = "[1 2 3]";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(ctx->count, 3);

  slp_object_t *copy1 = slp_object_copy(ctx->objects[0]);
  slp_object_t *copy2 = slp_object_copy(ctx->objects[1]);
  slp_object_t *copy3 = slp_object_copy(ctx->objects[2]);

  ASSERT_TRUE(slp_objects_equal(ctx->objects[0], copy1));
  ASSERT_TRUE(slp_objects_equal(ctx->objects[1], copy2));
  ASSERT_TRUE(slp_objects_equal(ctx->objects[2], copy3));

  ctx->objects[0]->value.integer = 999;

  ASSERT_EQ(copy1->value.integer, 1);
  ASSERT_FALSE(slp_objects_equal(ctx->objects[0], copy1));

  slp_object_free(copy1);
  slp_object_free(copy2);
  slp_object_free(copy3);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_list_equality_all_types(void) {
  test_context_t *ctx1 = test_context_new();
  test_context_t *ctx2 = test_context_new();
  slp_callbacks_t callbacks1 = test_callbacks_create(ctx1);
  slp_callbacks_t callbacks2 = test_callbacks_create(ctx2);

  const char *inputs[] = {"(a b)", "[1 2]", "{x y}"};

  for (size_t i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++) {
    slp_buffer_t *buffer1 = slp_buffer_new(32);
    slp_buffer_t *buffer2 = slp_buffer_new(32);

    slp_buffer_copy_to(buffer1, (uint8_t *)inputs[i], strlen(inputs[i]));
    slp_buffer_copy_to(buffer2, (uint8_t *)inputs[i], strlen(inputs[i]));

    slp_process_buffer(buffer1, &callbacks1);
    slp_process_buffer(buffer2, &callbacks2);

    ASSERT_EQ(ctx1->count, ctx2->count);
    for (size_t j = 0; j < ctx1->count; j++) {
      ASSERT_TRUE(slp_objects_equal(ctx1->objects[j], ctx2->objects[j]));
    }

    slp_buffer_free(buffer1);
    slp_buffer_free(buffer2);

    test_context_free(ctx1);
    test_context_free(ctx2);
    ctx1 = test_context_new();
    ctx2 = test_context_new();
    callbacks1 = test_callbacks_create(ctx1);
    callbacks2 = test_callbacks_create(ctx2);
  }

  test_context_free(ctx1);
  test_context_free(ctx2);
}

static void test_nested_list_structure(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(32);
  const char *input = "(a [b c])";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(ctx->count, 3);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(ctx->objects[1]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(ctx->objects[2]->type, SLP_TYPE_SYMBOL);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_symbol_buffer_contents(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(32);
  const char *input = "foo bar baz";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(ctx->count, 3);

  ASSERT_EQ(ctx->objects[0]->value.buffer->count, 3);
  ASSERT_EQ(memcmp(ctx->objects[0]->value.buffer->data, "foo", 3), 0);

  ASSERT_EQ(ctx->objects[1]->value.buffer->count, 3);
  ASSERT_EQ(memcmp(ctx->objects[1]->value.buffer->data, "bar", 3), 0);

  ASSERT_EQ(ctx->objects[2]->value.buffer->count, 3);
  ASSERT_EQ(memcmp(ctx->objects[2]->value.buffer->data, "baz", 3), 0);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_mixed_type_list(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(64);
  const char *input = "(a 42 3.14 \"str\")";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(ctx->count, 4);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_SYMBOL);
  ASSERT_EQ(ctx->objects[1]->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(ctx->objects[2]->type, SLP_TYPE_REAL);
  ASSERT_EQ(ctx->objects[3]->type, SLP_TYPE_SYMBOL);

  ASSERT_EQ(ctx->objects[1]->value.integer, 42);
  ASSERT_EQ((int)(ctx->objects[2]->value.real * 100), 314);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_quoted_buffer_contents(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(32);
  const char *input = "'hello";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(ctx->count, 1);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_QUOTED);
  ASSERT_EQ(ctx->objects[0]->value.buffer->count, 5);
  ASSERT_EQ(memcmp(ctx->objects[0]->value.buffer->data, "hello", 5), 0);

  slp_buffer_free(buffer);
  test_context_free(ctx);
}

static void test_deep_copy_list_s(void) {
  test_context_t *ctx = test_context_new();
  slp_callbacks_t callbacks = test_callbacks_create(ctx);

  slp_buffer_t *buffer = slp_buffer_new(32);
  const char *input = "\"test\"";
  slp_buffer_copy_to(buffer, (uint8_t *)input, strlen(input));

  slp_process_buffer(buffer, &callbacks);

  ASSERT_EQ(ctx->count, 1);
  ASSERT_EQ(ctx->objects[0]->type, SLP_TYPE_SYMBOL);

  slp_object_t *copy = slp_object_copy(ctx->objects[0]);
  ASSERT_TRUE(slp_objects_equal(ctx->objects[0], copy));

  ctx->objects[0]->value.buffer->data[0] = 'X';
  ASSERT_FALSE(slp_objects_equal(ctx->objects[0], copy));

  slp_object_free(copy);
  slp_buffer_free(buffer);
  test_context_free(ctx);
}

int main(void) {
  test_empty_lists_create_no_objects();
  test_string_buffer_contents();
  test_object_equality_simple();
  test_copy_independence_simple();
  test_list_copy_deep();
  test_list_equality_all_types();
  test_nested_list_structure();
  test_symbol_buffer_contents();
  test_mixed_type_list();
  test_quoted_buffer_contents();
  test_deep_copy_list_s();

  return 0;
}
