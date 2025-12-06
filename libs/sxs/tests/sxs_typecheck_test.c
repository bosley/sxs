#include "../forms.h"
#include "../impls/impls.h"
#include "../sxs.h"
#include "../typecheck.h"
#include "buffer/buffer.h"
#include "testlib/test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static slp_object_t *create_test_integer(int64_t value) {
  slp_object_t *obj = malloc(sizeof(slp_object_t));
  ASSERT_NOT_NULL(obj);
  obj->type = SLP_TYPE_INTEGER;
  obj->value.integer = value;
  obj->source_position = 0;
  return obj;
}

static slp_object_t *create_test_real(double value) {
  slp_object_t *obj = malloc(sizeof(slp_object_t));
  ASSERT_NOT_NULL(obj);
  obj->type = SLP_TYPE_REAL;
  obj->value.real = value;
  obj->source_position = 0;
  return obj;
}

static slp_object_t *create_test_symbol(const char *text) {
  slp_object_t *obj = malloc(sizeof(slp_object_t));
  ASSERT_NOT_NULL(obj);
  obj->type = SLP_TYPE_SYMBOL;
  size_t len = strlen(text);
  obj->value.buffer = slp_buffer_new(len);
  ASSERT_NOT_NULL(obj->value.buffer);
  slp_buffer_copy_to(obj->value.buffer, (uint8_t *)text, len);
  obj->source_position = 0;
  return obj;
}

static slp_object_t *create_test_list_s(void) {
  slp_object_t *obj = malloc(sizeof(slp_object_t));
  ASSERT_NOT_NULL(obj);
  obj->type = SLP_TYPE_LIST_S;
  obj->source_position = 0;
  obj->value.list.count = 0;
  obj->value.list.items = NULL;
  return obj;
}

static slp_object_t *create_test_list_b(slp_object_t **items, size_t count) {
  slp_object_t *obj = malloc(sizeof(slp_object_t));
  ASSERT_NOT_NULL(obj);
  obj->type = SLP_TYPE_LIST_B;
  obj->source_position = 0;
  obj->value.list.count = count;
  obj->value.list.items = malloc(sizeof(slp_object_t *) * count);
  ASSERT_NOT_NULL(obj->value.list.items);
  for (size_t i = 0; i < count; i++) {
    obj->value.list.items[i] = slp_object_copy(items[i]);
  }
  return obj;
}

static slp_object_t *create_test_list_p(slp_object_t **items, size_t count) {
  slp_object_t *obj = malloc(sizeof(slp_object_t));
  ASSERT_NOT_NULL(obj);
  obj->type = SLP_TYPE_LIST_P;
  obj->source_position = 0;
  obj->value.list.count = count;
  obj->value.list.items = malloc(sizeof(slp_object_t *) * count);
  ASSERT_NOT_NULL(obj->value.list.items);
  for (size_t i = 0; i < count; i++) {
    obj->value.list.items[i] = slp_object_copy(items[i]);
  }
  return obj;
}

static slp_object_t *create_test_list_c(slp_object_t **items, size_t count) {
  slp_object_t *obj = malloc(sizeof(slp_object_t));
  ASSERT_NOT_NULL(obj);
  obj->type = SLP_TYPE_LIST_C;
  obj->source_position = 0;
  obj->value.list.count = count;
  obj->value.list.items = malloc(sizeof(slp_object_t *) * count);
  ASSERT_NOT_NULL(obj->value.list.items);
  for (size_t i = 0; i < count; i++) {
    obj->value.list.items[i] = slp_object_copy(items[i]);
  }
  return obj;
}

static slp_object_t *create_test_quoted(const char *text) {
  slp_object_t *obj = malloc(sizeof(slp_object_t));
  ASSERT_NOT_NULL(obj);
  obj->type = SLP_TYPE_QUOTED;
  obj->source_position = 0;
  size_t len = strlen(text);
  obj->value.buffer = slp_buffer_new(len);
  ASSERT_NOT_NULL(obj->value.buffer);
  slp_buffer_copy_to(obj->value.buffer, (uint8_t *)text, len);
  return obj;
}

static slp_object_t *create_test_none(void) {
  slp_object_t *obj = malloc(sizeof(slp_object_t));
  ASSERT_NOT_NULL(obj);
  obj->type = SLP_TYPE_NONE;
  obj->source_position = 0;
  return obj;
}

static sxs_builtin_registry_t *create_test_registry(void) {
  sxs_builtins_init();
  sxs_builtin_registry_t *registry = sxs_builtin_registry_create(0);
  ASSERT_NOT_NULL(registry);
  sxs_builtin_registry_add(registry, sxs_impl_get_load_store());
  sxs_builtin_registry_add(registry, sxs_impl_get_debug());
  sxs_builtin_registry_add(registry, sxs_impl_get_rotl());
  sxs_builtin_registry_add(registry, sxs_impl_get_rotr());
  sxs_builtin_registry_add(registry, sxs_impl_get_insist());
  sxs_builtin_registry_add(registry, sxs_impl_get_catch());
  return registry;
}

static sxs_callable_t *get_callable_from_impl(sxs_command_impl_t impl) {
  return sxs_get_callable_for_handler(impl.handler);
}

static void cleanup_test_registry(sxs_builtin_registry_t *registry) {
  sxs_builtin_registry_free(registry);
  sxs_builtins_deinit();
}

static void free_form_def(form_definition_t *def) {
  if (!def) {
    return;
  }
  if (def->name) {
    free(def->name);
  }
  if (def->types) {
    free(def->types);
  }
  free(def);
}

static void test_typecheck_context_create_destroy(void) {
  sxs_builtin_registry_t *registry = create_test_registry();

  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);
  ASSERT_NOT_NULL(ctx);
  ASSERT_NOT_NULL(ctx->current_context);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_FALSE(ctx->has_error);
  ASSERT_EQ(ctx->next_context_id, 1);

  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_typecheck_object_integer(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  slp_object_t *obj = create_test_integer(42);
  form_definition_t *type = sxs_typecheck_object(ctx, obj);
  ASSERT_NOT_NULL(type);
  ASSERT_EQ(type->types[0], FORM_TYPE_INTEGER);
  ASSERT_EQ(type->type_count, 1);

  free_form_def(type);
  slp_object_free(obj);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_typecheck_object_real(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  slp_object_t *obj = create_test_real(3.14);
  form_definition_t *type = sxs_typecheck_object(ctx, obj);
  ASSERT_NOT_NULL(type);
  ASSERT_EQ(type->types[0], FORM_TYPE_REAL);

  free_form_def(type);
  slp_object_free(obj);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_typecheck_object_symbol(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  slp_object_t *obj = create_test_symbol("test");
  form_definition_t *type = sxs_typecheck_object(ctx, obj);
  ASSERT_NOT_NULL(type);
  ASSERT_EQ(type->types[0], FORM_TYPE_SYMBOL);

  free_form_def(type);
  slp_object_free(obj);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_typecheck_object_list_s(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  slp_object_t *obj = create_test_list_s();
  form_definition_t *type = sxs_typecheck_object(ctx, obj);
  ASSERT_NOT_NULL(type);
  ASSERT_EQ(type->types[0], FORM_TYPE_LIST_S);

  free_form_def(type);
  slp_object_free(obj);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_typecheck_object_list_b(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  slp_object_t *items[2];
  items[0] = create_test_integer(1);
  items[1] = create_test_integer(2);

  slp_object_t *obj = create_test_list_b(items, 2);
  form_definition_t *type = sxs_typecheck_object(ctx, obj);
  ASSERT_NOT_NULL(type);
  ASSERT_EQ(type->types[0], FORM_TYPE_LIST_B);

  free_form_def(type);
  slp_object_free(obj);
  slp_object_free(items[0]);
  slp_object_free(items[1]);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_typecheck_object_list_c(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  slp_object_t *items[1];
  items[0] = create_test_integer(42);

  slp_object_t *obj = create_test_list_c(items, 1);
  form_definition_t *type = sxs_typecheck_object(ctx, obj);
  ASSERT_NOT_NULL(type);
  ASSERT_EQ(type->types[0], FORM_TYPE_LIST_C);

  free_form_def(type);
  slp_object_free(obj);
  slp_object_free(items[0]);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_typecheck_object_quoted(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  slp_object_t *obj = create_test_quoted("test");

  form_definition_t *type = sxs_typecheck_object(ctx, obj);
  ASSERT_NOT_NULL(type);
  ASSERT_EQ(type->types[0], FORM_TYPE_SOME);

  free_form_def(type);
  slp_object_free(obj);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_typecheck_object_none(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  slp_object_t *obj = create_test_none();
  form_definition_t *type = sxs_typecheck_object(ctx, obj);
  ASSERT_NOT_NULL(type);
  ASSERT_EQ(type->types[0], FORM_TYPE_NONE);

  free_form_def(type);
  slp_object_free(obj);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_typecheck_object_null_handling(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  form_definition_t *null_obj_type = sxs_typecheck_object(ctx, NULL);
  ASSERT_NULL(null_obj_type);

  slp_object_t *obj = create_test_integer(42);
  form_definition_t *null_ctx_type = sxs_typecheck_object(NULL, obj);
  ASSERT_NULL(null_ctx_type);
  slp_object_free(obj);

  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_register_type_tracking_store(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_callable_t *load_store =
      get_callable_from_impl(sxs_impl_get_load_store());

  slp_object_t *args[2];
  args[0] = create_test_integer(10);
  args[1] = create_test_integer(42);

  int result = sxs_typecheck_load_store(ctx, load_store, args, 2);
  ASSERT_EQ(result, 0);
  ASSERT_NOT_NULL(ctx->register_types[10]);
  ASSERT_EQ(ctx->register_types[10]->types[0], FORM_TYPE_INTEGER);

  slp_object_free(args[0]);
  slp_object_free(args[1]);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_register_type_tracking_load_uninitialized(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  ASSERT_NULL(ctx->register_types[99]);

  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_register_type_tracking_store_then_load(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_callable_t *load_store =
      get_callable_from_impl(sxs_impl_get_load_store());

  slp_object_t *store_args[2];
  store_args[0] = create_test_integer(5);
  store_args[1] = create_test_real(3.14);

  int store_result = sxs_typecheck_load_store(ctx, load_store, store_args, 2);
  ASSERT_EQ(store_result, 0);
  ASSERT_NOT_NULL(ctx->register_types[5]);
  ASSERT_EQ(ctx->register_types[5]->types[0], FORM_TYPE_REAL);

  slp_object_free(store_args[0]);
  slp_object_free(store_args[1]);

  slp_object_t *load_args[1];
  load_args[0] = create_test_integer(5);

  int load_result = sxs_typecheck_load_store(ctx, load_store, load_args, 1);
  ASSERT_EQ(load_result, 0);

  slp_object_free(load_args[0]);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_register_type_tracking_overwrite(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_callable_t *load_store =
      get_callable_from_impl(sxs_impl_get_load_store());

  slp_object_t *args1[2];
  args1[0] = create_test_integer(7);
  args1[1] = create_test_integer(100);

  sxs_typecheck_load_store(ctx, load_store, args1, 2);
  ASSERT_EQ(ctx->register_types[7]->types[0], FORM_TYPE_INTEGER);

  slp_object_free(args1[0]);
  slp_object_free(args1[1]);

  slp_object_t *args2[2];
  args2[0] = create_test_integer(7);
  args2[1] = create_test_symbol("test");

  sxs_typecheck_load_store(ctx, load_store, args2, 2);
  ASSERT_EQ(ctx->register_types[7]->types[0], FORM_TYPE_SYMBOL);

  slp_object_free(args2[0]);
  slp_object_free(args2[1]);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_register_type_tracking_bounds(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_callable_t *load_store =
      get_callable_from_impl(sxs_impl_get_load_store());

  slp_object_t *args_valid[2];
  args_valid[0] = create_test_integer(8191);
  args_valid[1] = create_test_integer(42);

  int result_valid = sxs_typecheck_load_store(ctx, load_store, args_valid, 2);
  ASSERT_EQ(result_valid, 0);
  ASSERT_NOT_NULL(ctx->register_types[8191]);

  slp_object_free(args_valid[0]);
  slp_object_free(args_valid[1]);

  slp_object_t *args_invalid[2];
  args_invalid[0] = create_test_integer(8192);
  args_invalid[1] = create_test_integer(42);

  sxs_typecheck_load_store(ctx, load_store, args_invalid, 2);

  slp_object_free(args_invalid[0]);
  slp_object_free(args_invalid[1]);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_insist_valid_int(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_callable_t *insist = get_callable_from_impl(sxs_impl_get_insist());

  slp_object_t *args[2];
  args[0] = create_test_symbol(":int");
  args[1] = create_test_integer(42);

  int result = sxs_typecheck_insist(ctx, insist, args, 2);
  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);

  slp_object_free(args[0]);
  slp_object_free(args[1]);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_insist_valid_real(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_callable_t *insist = get_callable_from_impl(sxs_impl_get_insist());

  slp_object_t *args[2];
  args[0] = create_test_symbol(":real");
  args[1] = create_test_real(3.14);

  int result = sxs_typecheck_insist(ctx, insist, args, 2);
  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);

  slp_object_free(args[0]);
  slp_object_free(args[1]);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_insist_valid_symbol(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_callable_t *insist = get_callable_from_impl(sxs_impl_get_insist());

  slp_object_t *args[2];
  args[0] = create_test_symbol(":symbol");
  args[1] = create_test_symbol("test");

  int result = sxs_typecheck_insist(ctx, insist, args, 2);
  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);

  slp_object_free(args[0]);
  slp_object_free(args[1]);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_insist_valid_list_b(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_callable_t *insist = get_callable_from_impl(sxs_impl_get_insist());

  slp_object_t *items[1];
  items[0] = create_test_integer(1);

  slp_object_t *args[2];
  args[0] = create_test_symbol(":list-b");
  args[1] = create_test_list_b(items, 1);

  int result = sxs_typecheck_insist(ctx, insist, args, 2);
  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);

  slp_object_free(args[0]);
  slp_object_free(args[1]);
  slp_object_free(items[0]);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_insist_valid_some(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_callable_t *insist = get_callable_from_impl(sxs_impl_get_insist());

  slp_object_t *args[2];
  args[0] = create_test_symbol(":some");
  args[1] = create_test_quoted("test");

  int result = sxs_typecheck_insist(ctx, insist, args, 2);
  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);

  slp_object_free(args[0]);
  slp_object_free(args[1]);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_insist_valid_any(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_callable_t *insist = get_callable_from_impl(sxs_impl_get_insist());

  slp_object_t *args[2];
  args[0] = create_test_symbol(":any");
  args[1] = create_test_integer(42);

  int result = sxs_typecheck_insist(ctx, insist, args, 2);
  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);

  slp_object_free(args[0]);
  slp_object_free(args[1]);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_insist_invalid_type_mismatch(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_callable_t *insist = get_callable_from_impl(sxs_impl_get_insist());

  slp_object_t *args[2];
  args[0] = create_test_symbol(":int");
  args[1] = create_test_real(3.14);

  int result = sxs_typecheck_insist(ctx, insist, args, 2);
  ASSERT_EQ(result, 1);
  ASSERT_TRUE(ctx->error_count > 0);
  ASSERT_TRUE(ctx->has_error);

  slp_object_free(args[0]);
  slp_object_free(args[1]);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_insist_unknown_form_type(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_callable_t *insist = get_callable_from_impl(sxs_impl_get_insist());

  slp_object_t *args[2];
  args[0] = create_test_symbol(":bogus");
  args[1] = create_test_integer(42);

  int result = sxs_typecheck_insist(ctx, insist, args, 2);
  ASSERT_EQ(result, 1);
  ASSERT_TRUE(ctx->error_count > 0);

  slp_object_free(args[0]);
  slp_object_free(args[1]);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_insist_invalid_first_arg_not_symbol(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_callable_t *insist = get_callable_from_impl(sxs_impl_get_insist());

  slp_object_t *args[2];
  args[0] = create_test_integer(42);
  args[1] = create_test_integer(100);

  int result = sxs_typecheck_insist(ctx, insist, args, 2);
  ASSERT_EQ(result, 1);
  ASSERT_TRUE(ctx->error_count > 0);

  slp_object_free(args[0]);
  slp_object_free(args[1]);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_insist_invalid_arg_count(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_callable_t *insist = get_callable_from_impl(sxs_impl_get_insist());

  slp_object_t *args[1];
  args[0] = create_test_symbol(":int");

  int result = sxs_typecheck_insist(ctx, insist, args, 1);
  ASSERT_EQ(result, 1);
  ASSERT_TRUE(ctx->error_count > 0);

  slp_object_free(args[0]);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_load_store_variant_1_load(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_callable_t *load_store =
      get_callable_from_impl(sxs_impl_get_load_store());

  slp_object_t *args[1];
  args[0] = create_test_integer(5);

  int result = sxs_typecheck_load_store(ctx, load_store, args, 1);
  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);

  slp_object_free(args[0]);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_load_store_variant_2_store(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_callable_t *load_store =
      get_callable_from_impl(sxs_impl_get_load_store());

  slp_object_t *args[2];
  args[0] = create_test_integer(10);
  args[1] = create_test_symbol("test");

  int result = sxs_typecheck_load_store(ctx, load_store, args, 2);
  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_NOT_NULL(ctx->register_types[10]);
  ASSERT_EQ(ctx->register_types[10]->types[0], FORM_TYPE_SYMBOL);

  slp_object_free(args[0]);
  slp_object_free(args[1]);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_load_store_variant_3_conditional_store(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_callable_t *load_store =
      get_callable_from_impl(sxs_impl_get_load_store());

  slp_object_t *args[3];
  args[0] = create_test_integer(15);
  args[1] = create_test_integer(100);
  args[2] = create_test_integer(200);

  int result = sxs_typecheck_load_store(ctx, load_store, args, 3);
  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);

  slp_object_free(args[0]);
  slp_object_free(args[1]);
  slp_object_free(args[2]);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_generic_debug_variadic(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_callable_t *debug = get_callable_from_impl(sxs_impl_get_debug());

  slp_object_t *args[4];
  args[0] = create_test_integer(42);
  args[1] = create_test_real(3.14);
  args[2] = create_test_symbol("test");
  args[3] = create_test_list_s();

  int result = sxs_typecheck_generic(ctx, debug, args, 4);
  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);

  for (size_t i = 0; i < 4; i++) {
    slp_object_free(args[i]);
  }
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_generic_rotl_valid(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_callable_t *rotl = get_callable_from_impl(sxs_impl_get_rotl());

  slp_object_t *args[2];
  args[0] = create_test_integer(42);
  args[1] = create_test_integer(2);

  int result = sxs_typecheck_generic(ctx, rotl, args, 2);
  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);

  slp_object_free(args[0]);
  slp_object_free(args[1]);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_generic_rotl_invalid_second_arg(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_callable_t *rotl = get_callable_from_impl(sxs_impl_get_rotl());

  slp_object_t *args[2];
  args[0] = create_test_integer(42);
  args[1] = create_test_real(2.5);

  int result = sxs_typecheck_generic(ctx, rotl, args, 2);
  ASSERT_EQ(result, 1);
  ASSERT_TRUE(ctx->error_count > 0);

  slp_object_free(args[0]);
  slp_object_free(args[1]);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_generic_rotl_invalid_arg_count(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_callable_t *rotl = get_callable_from_impl(sxs_impl_get_rotl());

  slp_object_t *args[1];
  args[0] = create_test_integer(42);

  int result = sxs_typecheck_generic(ctx, rotl, args, 1);
  ASSERT_EQ(result, 1);
  ASSERT_TRUE(ctx->error_count > 0);

  slp_object_free(args[0]);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_generic_rotr_valid(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_callable_t *rotr = get_callable_from_impl(sxs_impl_get_rotr());

  slp_object_t *args[2];
  args[0] = create_test_symbol("test");
  args[1] = create_test_integer(1);

  int result = sxs_typecheck_generic(ctx, rotr, args, 2);
  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);

  slp_object_free(args[0]);
  slp_object_free(args[1]);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_generic_catch_variadic(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_callable_t *catch = get_callable_from_impl(sxs_impl_get_catch());

  slp_object_t *args[3];
  args[0] = create_test_integer(1);
  args[1] = create_test_integer(2);
  args[2] = create_test_integer(3);

  int result = sxs_typecheck_generic(ctx, catch, args, 3);
  ASSERT_EQ(result, 0);
  ASSERT_EQ(ctx->error_count, 0);

  for (size_t i = 0; i < 3; i++) {
    slp_object_free(args[i]);
  }
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_error_accumulation_multiple(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  ASSERT_EQ(ctx->error_count, 0);
  ASSERT_FALSE(ctx->has_error);

  sxs_typecheck_add_error(ctx, "First error", 10);
  ASSERT_EQ(ctx->error_count, 1);
  ASSERT_TRUE(ctx->has_error);

  sxs_typecheck_add_error(ctx, "Second error", 20);
  ASSERT_EQ(ctx->error_count, 2);

  sxs_typecheck_add_error(ctx, "Third error", 30);
  ASSERT_EQ(ctx->error_count, 3);

  ASSERT_STR_EQ(ctx->errors[0].message, "First error");
  ASSERT_EQ(ctx->errors[0].position, 10);
  ASSERT_STR_EQ(ctx->errors[1].message, "Second error");
  ASSERT_EQ(ctx->errors[1].position, 20);
  ASSERT_STR_EQ(ctx->errors[2].message, "Third error");
  ASSERT_EQ(ctx->errors[2].position, 30);

  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_error_detailed_information(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_typecheck_add_detailed_error(ctx, "Type mismatch", 100, "test_func",
                                   "(test_func int)", "(test_func real)");

  ASSERT_EQ(ctx->error_count, 1);
  ASSERT_STR_EQ(ctx->errors[0].message, "Type mismatch");
  ASSERT_EQ(ctx->errors[0].position, 100);
  ASSERT_STR_EQ(ctx->errors[0].function_name, "test_func");
  ASSERT_STR_EQ(ctx->errors[0].expected_signature, "(test_func int)");
  ASSERT_STR_EQ(ctx->errors[0].received_signature, "(test_func real)");

  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_edge_case_null_context(void) {
  sxs_builtin_registry_t *registry = create_test_registry();

  sxs_callable_t *debug = get_callable_from_impl(sxs_impl_get_debug());

  slp_object_t *args[1];
  args[0] = create_test_integer(42);

  int result = sxs_typecheck_generic(NULL, debug, args, 1);
  ASSERT_EQ(result, 1);

  slp_object_free(args[0]);
  cleanup_test_registry(registry);
}

static void test_edge_case_null_callable(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  slp_object_t *args[1];
  args[0] = create_test_integer(42);

  int result = sxs_typecheck_generic(ctx, NULL, args, 1);
  ASSERT_EQ(result, 1);

  slp_object_free(args[0]);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_edge_case_null_args(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_callable_t *debug = get_callable_from_impl(sxs_impl_get_debug());

  slp_object_t *args[2];
  args[0] = create_test_integer(42);
  args[1] = NULL;

  int result = sxs_typecheck_generic(ctx, debug, args, 2);
  ASSERT_EQ(result, 1);
  ASSERT_TRUE(ctx->error_count > 0);

  slp_object_free(args[0]);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_integration_store_insist_use(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_callable_t *load_store =
      get_callable_from_impl(sxs_impl_get_load_store());

  sxs_callable_t *insist = get_callable_from_impl(sxs_impl_get_insist());

  sxs_callable_t *rotl = get_callable_from_impl(sxs_impl_get_rotl());

  slp_object_t *store_args[2];
  store_args[0] = create_test_integer(10);
  store_args[1] = create_test_integer(42);

  int store_result = sxs_typecheck_load_store(ctx, load_store, store_args, 2);
  ASSERT_EQ(store_result, 0);
  ASSERT_NOT_NULL(ctx->register_types[10]);
  ASSERT_EQ(ctx->register_types[10]->types[0], FORM_TYPE_INTEGER);

  slp_object_free(store_args[0]);
  slp_object_free(store_args[1]);

  slp_object_t *load_arg = create_test_integer(10);
  slp_object_t *insist_args[2];
  insist_args[0] = create_test_symbol(":int");
  insist_args[1] = load_arg;

  int insist_result = sxs_typecheck_insist(ctx, insist, insist_args, 2);
  ASSERT_EQ(insist_result, 0);
  ASSERT_EQ(ctx->error_count, 0);

  slp_object_free(insist_args[0]);
  slp_object_free(insist_args[1]);

  slp_object_t *rotl_args[2];
  rotl_args[0] = create_test_integer(10);
  rotl_args[1] = create_test_integer(2);

  int rotl_result = sxs_typecheck_generic(ctx, rotl, rotl_args, 2);
  ASSERT_EQ(rotl_result, 0);
  ASSERT_EQ(ctx->error_count, 0);

  slp_object_free(rotl_args[0]);
  slp_object_free(rotl_args[1]);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_integration_type_mismatch_detection(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_callable_t *insist = get_callable_from_impl(sxs_impl_get_insist());

  slp_object_t *insist_args[2];
  insist_args[0] = create_test_symbol(":int");
  insist_args[1] = create_test_real(3.14);

  int insist_result = sxs_typecheck_insist(ctx, insist, insist_args, 2);
  ASSERT_EQ(insist_result, 1);
  ASSERT_TRUE(ctx->error_count > 0);
  ASSERT_TRUE(ctx->has_error);

  slp_object_free(insist_args[0]);
  slp_object_free(insist_args[1]);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_integration_insist_updates_register_type(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_callable_t *load_store =
      get_callable_from_impl(sxs_impl_get_load_store());
  sxs_callable_t *insist = get_callable_from_impl(sxs_impl_get_insist());

  slp_object_t *store_args[2];
  store_args[0] = create_test_integer(30);
  store_args[1] = create_test_integer(100);

  int store_result = sxs_typecheck_load_store(ctx, load_store, store_args, 2);
  ASSERT_EQ(store_result, 0);
  ASSERT_NOT_NULL(ctx->register_types[30]);
  ASSERT_EQ(ctx->register_types[30]->types[0], FORM_TYPE_INTEGER);

  slp_object_free(store_args[0]);
  slp_object_free(store_args[1]);

  slp_object_t *load_arg = create_test_integer(30);
  slp_object_t *insist_args[2];
  insist_args[0] = create_test_symbol(":int");
  insist_args[1] = load_arg;

  int insist_result = sxs_typecheck_insist(ctx, insist, insist_args, 2);
  ASSERT_EQ(insist_result, 0);
  ASSERT_EQ(ctx->error_count, 0);

  slp_object_free(insist_args[0]);
  slp_object_free(insist_args[1]);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_integration_complex_nested_expressions(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_callable_t *load_store =
      get_callable_from_impl(sxs_impl_get_load_store());

  sxs_callable_t *rotl = get_callable_from_impl(sxs_impl_get_rotl());

  sxs_callable_t *insist = get_callable_from_impl(sxs_impl_get_insist());

  slp_object_t *store_args[2];
  store_args[0] = create_test_integer(3);
  store_args[1] = create_test_integer(100);

  int store_result = sxs_typecheck_load_store(ctx, load_store, store_args, 2);
  ASSERT_EQ(store_result, 0);

  slp_object_free(store_args[0]);
  slp_object_free(store_args[1]);

  slp_object_t *rotl_args[2];
  rotl_args[0] = create_test_integer(100);
  rotl_args[1] = create_test_integer(2);

  int rotl_result = sxs_typecheck_generic(ctx, rotl, rotl_args, 2);
  ASSERT_EQ(rotl_result, 0);
  ASSERT_EQ(ctx->error_count, 0);

  slp_object_free(rotl_args[0]);
  slp_object_free(rotl_args[1]);
  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

static void test_integration_multiple_register_operations(void) {
  sxs_builtin_registry_t *registry = create_test_registry();
  sxs_typecheck_context_t *ctx = sxs_typecheck_context_create(registry);

  sxs_callable_t *load_store =
      get_callable_from_impl(sxs_impl_get_load_store());

  for (int i = 0; i < 10; i++) {
    slp_object_t *args[2];
    args[0] = create_test_integer(i);
    args[1] = create_test_integer(i * 10);

    int result = sxs_typecheck_load_store(ctx, load_store, args, 2);
    ASSERT_EQ(result, 0);
    ASSERT_NOT_NULL(ctx->register_types[i]);
    ASSERT_EQ(ctx->register_types[i]->types[0], FORM_TYPE_INTEGER);

    slp_object_free(args[0]);
    slp_object_free(args[1]);
  }

  ASSERT_EQ(ctx->error_count, 0);

  sxs_typecheck_context_free(ctx);
  cleanup_test_registry(registry);
}

int main(void) {
  test_typecheck_context_create_destroy();

  test_typecheck_object_integer();
  test_typecheck_object_real();
  test_typecheck_object_symbol();
  test_typecheck_object_list_s();
  test_typecheck_object_list_b();
  test_typecheck_object_list_c();
  test_typecheck_object_quoted();
  test_typecheck_object_none();
  test_typecheck_object_null_handling();

  test_register_type_tracking_store();
  test_register_type_tracking_load_uninitialized();
  test_register_type_tracking_store_then_load();
  test_register_type_tracking_overwrite();
  test_register_type_tracking_bounds();

  test_insist_valid_int();
  test_insist_valid_real();
  test_insist_valid_symbol();
  test_insist_valid_list_b();
  test_insist_valid_some();
  test_insist_valid_any();
  test_insist_invalid_type_mismatch();
  test_insist_unknown_form_type();
  test_insist_invalid_first_arg_not_symbol();
  test_insist_invalid_arg_count();

  test_load_store_variant_1_load();
  test_load_store_variant_2_store();
  test_load_store_variant_3_conditional_store();

  test_generic_debug_variadic();
  test_generic_rotl_valid();
  test_generic_rotl_invalid_second_arg();
  test_generic_rotl_invalid_arg_count();
  test_generic_rotr_valid();
  test_generic_catch_variadic();

  test_error_accumulation_multiple();
  test_error_detailed_information();

  test_edge_case_null_context();
  test_edge_case_null_callable();
  test_edge_case_null_args();

  test_integration_store_insist_use();
  test_integration_type_mismatch_detection();
  test_integration_insist_updates_register_type();
  test_integration_complex_nested_expressions();
  test_integration_multiple_register_operations();

  printf("All sxs_typecheck tests passed!\n");
  return 0;
}
