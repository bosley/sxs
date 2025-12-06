#include "../ctx.h"
#include "test.h"
#include <string.h>

void test_ctx_create_free(void) {
  ctx_t *ctx = ctx_create(NULL);
  ASSERT_NOT_NULL(ctx);
  ASSERT_NULL(ctx->parent);
  ASSERT_EQ(ctx->data.base.nnodes, 0);
  ctx_free(ctx);
}

void test_ctx_create_with_parent(void) {
  ctx_t *parent = ctx_create(NULL);
  ASSERT_NOT_NULL(parent);

  ctx_t *child = ctx_create(parent);
  ASSERT_NOT_NULL(child);
  ASSERT_EQ(child->parent, parent);

  ctx_free(child);
  ctx_free(parent);
}

void test_ctx_set_get_integer(void) {
  ctx_t *ctx = ctx_create(NULL);
  ASSERT_NOT_NULL(ctx);

  slp_object_t obj;
  obj.type = SLP_TYPE_INTEGER;
  obj.value.integer = 42;
  obj.source_position = 0;

  int result = ctx_set(ctx, "test_key", &obj);
  ASSERT_EQ(result, 0);

  slp_object_t *retrieved = ctx_get(ctx, "test_key");
  ASSERT_NOT_NULL(retrieved);
  ASSERT_EQ(retrieved->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(retrieved->value.integer, 42);

  ctx_free(ctx);
}

void test_ctx_set_get_real(void) {
  ctx_t *ctx = ctx_create(NULL);
  ASSERT_NOT_NULL(ctx);

  slp_object_t obj;
  obj.type = SLP_TYPE_REAL;
  obj.value.real = 3.14159;
  obj.source_position = 0;

  ctx_set(ctx, "pi", &obj);

  slp_object_t *retrieved = ctx_get(ctx, "pi");
  ASSERT_NOT_NULL(retrieved);
  ASSERT_EQ(retrieved->type, SLP_TYPE_REAL);
  ASSERT_TRUE(retrieved->value.real > 3.14 && retrieved->value.real < 3.15);

  ctx_free(ctx);
}

void test_ctx_set_get_symbol(void) {
  ctx_t *ctx = ctx_create(NULL);
  ASSERT_NOT_NULL(ctx);

  slp_buffer_t *buffer = slp_buffer_new(11);
  slp_buffer_copy_to(buffer, (uint8_t *)"test_symbol", 11);

  slp_object_t obj;
  obj.type = SLP_TYPE_SYMBOL;
  obj.value.buffer = buffer;
  obj.source_position = 0;

  ctx_set(ctx, "symbol_key", &obj);

  slp_object_t *retrieved = ctx_get(ctx, "symbol_key");
  ASSERT_NOT_NULL(retrieved);
  ASSERT_EQ(retrieved->type, SLP_TYPE_SYMBOL);
  ASSERT_NOT_NULL(retrieved->value.buffer);
  ASSERT_EQ(retrieved->value.buffer->count, 11);
  ASSERT_EQ(memcmp(retrieved->value.buffer->data, "test_symbol", 11), 0);

  slp_buffer_free(buffer);
  ctx_free(ctx);
}

void test_ctx_overwrite_frees_old(void) {
  ctx_t *ctx = ctx_create(NULL);
  ASSERT_NOT_NULL(ctx);

  slp_object_t obj1;
  obj1.type = SLP_TYPE_INTEGER;
  obj1.value.integer = 100;
  obj1.source_position = 0;

  ctx_set(ctx, "key", &obj1);
  ASSERT_EQ(ctx->data.base.nnodes, 1);

  slp_object_t *retrieved1 = ctx_get(ctx, "key");
  ASSERT_NOT_NULL(retrieved1);
  ASSERT_EQ(retrieved1->value.integer, 100);

  slp_object_t obj2;
  obj2.type = SLP_TYPE_INTEGER;
  obj2.value.integer = 200;
  obj2.source_position = 0;

  ctx_set(ctx, "key", &obj2);
  ASSERT_EQ(ctx->data.base.nnodes, 1);

  slp_object_t *retrieved2 = ctx_get(ctx, "key");
  ASSERT_NOT_NULL(retrieved2);
  ASSERT_EQ(retrieved2->value.integer, 200);

  ctx_free(ctx);
}

void test_ctx_get_nonexistent(void) {
  ctx_t *ctx = ctx_create(NULL);
  ASSERT_NOT_NULL(ctx);

  slp_object_t *retrieved = ctx_get(ctx, "nonexistent");
  ASSERT_NULL(retrieved);

  ctx_free(ctx);
}

void test_ctx_get_context_if_exists_current(void) {
  ctx_t *ctx = ctx_create(NULL);
  ASSERT_NOT_NULL(ctx);

  slp_object_t obj;
  obj.type = SLP_TYPE_INTEGER;
  obj.value.integer = 42;
  obj.source_position = 0;

  ctx_set(ctx, "key", &obj);

  ctx_t *found = ctx_get_context_if_exists(ctx, "key", false);
  ASSERT_NOT_NULL(found);
  ASSERT_EQ(found, ctx);

  ctx_free(ctx);
}

void test_ctx_get_context_if_exists_not_found(void) {
  ctx_t *ctx = ctx_create(NULL);
  ASSERT_NOT_NULL(ctx);

  ctx_t *found = ctx_get_context_if_exists(ctx, "nonexistent", false);
  ASSERT_NULL(found);

  ctx_free(ctx);
}

void test_ctx_get_context_if_exists_parent_search(void) {
  ctx_t *parent = ctx_create(NULL);
  ASSERT_NOT_NULL(parent);

  slp_object_t obj;
  obj.type = SLP_TYPE_INTEGER;
  obj.value.integer = 100;
  obj.source_position = 0;

  ctx_set(parent, "parent_key", &obj);

  ctx_t *child = ctx_create(parent);
  ASSERT_NOT_NULL(child);

  ctx_t *found = ctx_get_context_if_exists(child, "parent_key", true);
  ASSERT_NOT_NULL(found);
  ASSERT_EQ(found, parent);

  slp_object_t *retrieved = ctx_get(found, "parent_key");
  ASSERT_NOT_NULL(retrieved);
  ASSERT_EQ(retrieved->value.integer, 100);

  ctx_free(child);
  ctx_free(parent);
}

void test_ctx_get_context_if_exists_no_parent_search(void) {
  ctx_t *parent = ctx_create(NULL);
  ASSERT_NOT_NULL(parent);

  slp_object_t obj;
  obj.type = SLP_TYPE_INTEGER;
  obj.value.integer = 100;
  obj.source_position = 0;

  ctx_set(parent, "parent_key", &obj);

  ctx_t *child = ctx_create(parent);
  ASSERT_NOT_NULL(child);

  ctx_t *found = ctx_get_context_if_exists(child, "parent_key", false);
  ASSERT_NULL(found);

  ctx_free(child);
  ctx_free(parent);
}

void test_ctx_nested_contexts_three_levels(void) {
  ctx_t *root = ctx_create(NULL);
  ctx_t *level1 = ctx_create(root);
  ctx_t *level2 = ctx_create(level1);

  slp_object_t obj_root, obj_level1, obj_level2;
  obj_root.type = SLP_TYPE_INTEGER;
  obj_root.value.integer = 1;
  obj_root.source_position = 0;

  obj_level1.type = SLP_TYPE_INTEGER;
  obj_level1.value.integer = 2;
  obj_level1.source_position = 0;

  obj_level2.type = SLP_TYPE_INTEGER;
  obj_level2.value.integer = 3;
  obj_level2.source_position = 0;

  ctx_set(root, "root_key", &obj_root);
  ctx_set(level1, "level1_key", &obj_level1);
  ctx_set(level2, "level2_key", &obj_level2);

  ctx_t *found_root = ctx_get_context_if_exists(level2, "root_key", true);
  ASSERT_NOT_NULL(found_root);
  ASSERT_EQ(found_root, root);

  ctx_t *found_level1 = ctx_get_context_if_exists(level2, "level1_key", true);
  ASSERT_NOT_NULL(found_level1);
  ASSERT_EQ(found_level1, level1);

  ctx_t *found_level2 = ctx_get_context_if_exists(level2, "level2_key", true);
  ASSERT_NOT_NULL(found_level2);
  ASSERT_EQ(found_level2, level2);

  ctx_free(level2);
  ctx_free(level1);
  ctx_free(root);
}

void test_ctx_remove(void) {
  ctx_t *ctx = ctx_create(NULL);
  ASSERT_NOT_NULL(ctx);

  slp_object_t obj;
  obj.type = SLP_TYPE_INTEGER;
  obj.value.integer = 42;
  obj.source_position = 0;

  ctx_set(ctx, "key", &obj);
  ASSERT_EQ(ctx->data.base.nnodes, 1);

  slp_object_t *retrieved = ctx_get(ctx, "key");
  ASSERT_NOT_NULL(retrieved);

  ctx_remove(ctx, "key");
  ASSERT_EQ(ctx->data.base.nnodes, 0);

  retrieved = ctx_get(ctx, "key");
  ASSERT_NULL(retrieved);

  ctx_free(ctx);
}

void test_ctx_remove_nonexistent(void) {
  ctx_t *ctx = ctx_create(NULL);
  ASSERT_NOT_NULL(ctx);

  ctx_remove(ctx, "nonexistent");
  ASSERT_EQ(ctx->data.base.nnodes, 0);

  ctx_free(ctx);
}

void test_ctx_multiple_keys(void) {
  ctx_t *ctx = ctx_create(NULL);
  ASSERT_NOT_NULL(ctx);

  slp_object_t obj1, obj2, obj3;
  obj1.type = SLP_TYPE_INTEGER;
  obj1.value.integer = 1;
  obj1.source_position = 0;

  obj2.type = SLP_TYPE_INTEGER;
  obj2.value.integer = 2;
  obj2.source_position = 0;

  obj3.type = SLP_TYPE_INTEGER;
  obj3.value.integer = 3;
  obj3.source_position = 0;

  ctx_set(ctx, "key1", &obj1);
  ctx_set(ctx, "key2", &obj2);
  ctx_set(ctx, "key3", &obj3);

  ASSERT_EQ(ctx->data.base.nnodes, 3);

  ASSERT_EQ(ctx_get(ctx, "key1")->value.integer, 1);
  ASSERT_EQ(ctx_get(ctx, "key2")->value.integer, 2);
  ASSERT_EQ(ctx_get(ctx, "key3")->value.integer, 3);

  ctx_free(ctx);
}

void test_ctx_list_object(void) {
  ctx_t *ctx = ctx_create(NULL);
  ASSERT_NOT_NULL(ctx);

  slp_object_t *item1 = malloc(sizeof(slp_object_t));
  item1->type = SLP_TYPE_INTEGER;
  item1->value.integer = 10;
  item1->source_position = 0;

  slp_object_t *item2 = malloc(sizeof(slp_object_t));
  item2->type = SLP_TYPE_INTEGER;
  item2->value.integer = 20;
  item2->source_position = 0;

  slp_object_t **items = malloc(sizeof(slp_object_t *) * 2);
  items[0] = item1;
  items[1] = item2;

  slp_object_t list_obj;
  list_obj.type = SLP_TYPE_LIST_P;
  list_obj.value.list.items = items;
  list_obj.value.list.count = 2;
  list_obj.source_position = 0;

  ctx_set(ctx, "list", &list_obj);

  slp_object_t *retrieved = ctx_get(ctx, "list");
  ASSERT_NOT_NULL(retrieved);
  ASSERT_EQ(retrieved->type, SLP_TYPE_LIST_P);
  ASSERT_EQ(retrieved->value.list.count, 2);
  ASSERT_EQ(retrieved->value.list.items[0]->value.integer, 10);
  ASSERT_EQ(retrieved->value.list.items[1]->value.integer, 20);

  free(items);
  free(item2);
  free(item1);
  ctx_free(ctx);
}

void test_ctx_memory_leak_check(void) {
  for (int round = 0; round < 10; round++) {
    ctx_t *ctx = ctx_create(NULL);
    ASSERT_NOT_NULL(ctx);

    for (int i = 0; i < 50; i++) {
      char key[32];
      snprintf(key, sizeof(key), "key_%d", i);

      slp_object_t obj;
      obj.type = SLP_TYPE_INTEGER;
      obj.value.integer = i;
      obj.source_position = 0;

      ctx_set(ctx, key, &obj);
    }

    for (int i = 0; i < 25; i++) {
      char key[32];
      snprintf(key, sizeof(key), "key_%d", i);
      ctx_remove(ctx, key);
    }

    ctx_free(ctx);
  }
}

void test_ctx_shadowing(void) {
  ctx_t *parent = ctx_create(NULL);
  ctx_t *child = ctx_create(parent);

  slp_object_t obj_parent, obj_child;
  obj_parent.type = SLP_TYPE_INTEGER;
  obj_parent.value.integer = 100;
  obj_parent.source_position = 0;

  obj_child.type = SLP_TYPE_INTEGER;
  obj_child.value.integer = 200;
  obj_child.source_position = 0;

  ctx_set(parent, "key", &obj_parent);
  ctx_set(child, "key", &obj_child);

  ctx_t *found_in_child = ctx_get_context_if_exists(child, "key", true);
  ASSERT_NOT_NULL(found_in_child);
  ASSERT_EQ(found_in_child, child);

  slp_object_t *retrieved = ctx_get(found_in_child, "key");
  ASSERT_EQ(retrieved->value.integer, 200);

  ctx_free(child);
  ctx_free(parent);
}

void test_ctx_null_key_handling(void) {
  ctx_t *ctx = ctx_create(NULL);
  ASSERT_NOT_NULL(ctx);

  slp_object_t obj;
  obj.type = SLP_TYPE_INTEGER;
  obj.value.integer = 42;
  obj.source_position = 0;

  int result = ctx_set(ctx, NULL, &obj);
  ASSERT_EQ(result, -1);

  slp_object_t *retrieved = ctx_get(ctx, NULL);
  ASSERT_NULL(retrieved);

  ctx_remove(ctx, NULL);

  ctx_free(ctx);
}

void test_ctx_null_object_handling(void) {
  ctx_t *ctx = ctx_create(NULL);
  ASSERT_NOT_NULL(ctx);

  int result = ctx_set(ctx, "key", NULL);
  ASSERT_EQ(result, -1);

  ctx_free(ctx);
}

void test_ctx_empty_key(void) {
  ctx_t *ctx = ctx_create(NULL);
  ASSERT_NOT_NULL(ctx);

  slp_object_t obj;
  obj.type = SLP_TYPE_INTEGER;
  obj.value.integer = 999;
  obj.source_position = 0;

  ctx_set(ctx, "", &obj);

  slp_object_t *retrieved = ctx_get(ctx, "");
  ASSERT_NOT_NULL(retrieved);
  ASSERT_EQ(retrieved->value.integer, 999);

  ctx_free(ctx);
}

int main(void) {
  test_ctx_create_free();
  test_ctx_create_with_parent();
  test_ctx_set_get_integer();
  test_ctx_set_get_real();
  test_ctx_set_get_symbol();
  test_ctx_overwrite_frees_old();
  test_ctx_get_nonexistent();
  test_ctx_get_context_if_exists_current();
  test_ctx_get_context_if_exists_not_found();
  test_ctx_get_context_if_exists_parent_search();
  test_ctx_get_context_if_exists_no_parent_search();
  test_ctx_nested_contexts_three_levels();
  test_ctx_remove();
  test_ctx_remove_nonexistent();
  test_ctx_multiple_keys();
  test_ctx_list_object();
  test_ctx_memory_leak_check();
  test_ctx_shadowing();
  test_ctx_null_key_handling();
  test_ctx_null_object_handling();
  test_ctx_empty_key();

  return 0;
}
