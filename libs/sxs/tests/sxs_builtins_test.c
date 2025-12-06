#include "../impls/impls.h"
#include "../sxs.h"
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

static sxs_runtime_t *create_test_runtime(void) {
  sxs_builtin_registry_t *registry = sxs_builtin_registry_create(0);
  ASSERT_NOT_NULL(registry);
  sxs_builtin_registry_add(registry, sxs_impl_get_load_store());
  sxs_builtin_registry_add(registry, sxs_impl_get_debug());
  sxs_builtin_registry_add(registry, sxs_impl_get_rotl());
  sxs_builtin_registry_add(registry, sxs_impl_get_rotr());
  sxs_runtime_t *runtime = sxs_runtime_new(registry);
  ASSERT_NOT_NULL(runtime);
  return runtime;
}

static void cleanup_test_runtime(sxs_runtime_t *runtime) {
  if (!runtime) {
    return;
  }
  sxs_runtime_free(runtime);
}

static void assert_list_equals(slp_object_t *list1, slp_object_t *list2) {
  ASSERT_NOT_NULL(list1);
  ASSERT_NOT_NULL(list2);
  ASSERT_EQ(list1->type, list2->type);
  ASSERT_EQ(list1->value.list.count, list2->value.list.count);
  for (size_t i = 0; i < list1->value.list.count; i++) {
    ASSERT_TRUE(slp_objects_equal(list1->value.list.items[i],
                                  list2->value.list.items[i]));
  }
}

static void assert_is_error(slp_object_t *object) {
  ASSERT_NOT_NULL(object);
  ASSERT_EQ(object->type, SLP_TYPE_ERROR);
}

static void test_at_get_valid_index(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *value = create_test_integer(42);
  slp_object_t *index = create_test_integer(5);
  slp_object_t *set_args[] = {index, value};

  sxs_callable_t *callable =
      sxs_get_callable_for_handler(sxs_builtin_load_store);
  ASSERT_NOT_NULL(callable);
  slp_object_t *set_result =
      sxs_builtin_load_store(runtime, callable, set_args, 2);
  ASSERT_NOT_NULL(set_result);
  ASSERT_EQ(set_result->type, SLP_TYPE_INTEGER);
  slp_object_free(set_result);

  slp_object_t *get_index = create_test_integer(5);
  slp_object_t *get_args[] = {get_index};
  slp_object_t *get_result =
      sxs_builtin_load_store(runtime, callable, get_args, 1);
  ASSERT_NOT_NULL(get_result);
  ASSERT_EQ(get_result->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(get_result->value.integer, 42);

  slp_object_free(value);
  slp_object_free(index);
  slp_object_free(get_index);
  slp_object_free(get_result);
  cleanup_test_runtime(runtime);
}

static void test_at_get_empty_slot(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *index = create_test_integer(10);
  slp_object_t *args[] = {index};

  sxs_callable_t *callable =
      sxs_get_callable_for_handler(sxs_builtin_load_store);
  slp_object_t *result = sxs_builtin_load_store(runtime, callable, args, 1);
  ASSERT_NOT_NULL(result);
  ASSERT_EQ(result->type, SLP_TYPE_NONE);

  slp_object_free(index);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_at_get_out_of_bounds_negative(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *index = create_test_integer(-1);
  slp_object_t *args[] = {index};

  sxs_callable_t *callable =
      sxs_get_callable_for_handler(sxs_builtin_load_store);
  slp_object_t *result = sxs_builtin_load_store(runtime, callable, args, 1);
  assert_is_error(result);

  slp_object_free(index);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_at_get_out_of_bounds_large(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *index = create_test_integer(8192);
  slp_object_t *args[] = {index};

  sxs_callable_t *callable =
      sxs_get_callable_for_handler(sxs_builtin_load_store);
  slp_object_t *result = sxs_builtin_load_store(runtime, callable, args, 1);
  assert_is_error(result);

  slp_object_free(index);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_at_set_integer(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *index = create_test_integer(0);
  slp_object_t *value = create_test_integer(99);
  slp_object_t *args[] = {index, value};

  sxs_callable_t *callable =
      sxs_get_callable_for_handler(sxs_builtin_load_store);
  slp_object_t *result = sxs_builtin_load_store(runtime, callable, args, 2);
  ASSERT_NOT_NULL(result);
  ASSERT_EQ(result->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(result->value.integer, 99);

  slp_object_free(index);
  slp_object_free(value);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_at_set_list(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *items[] = {create_test_integer(1), create_test_integer(2),
                           create_test_integer(3)};
  slp_object_t *list = create_test_list_b(items, 3);
  slp_object_t *index = create_test_integer(1);
  slp_object_t *args[] = {index, list};

  sxs_callable_t *callable =
      sxs_get_callable_for_handler(sxs_builtin_load_store);
  slp_object_t *result = sxs_builtin_load_store(runtime, callable, args, 2);
  ASSERT_NOT_NULL(result);
  ASSERT_EQ(result->type, SLP_TYPE_LIST_B);
  ASSERT_EQ(result->value.list.count, 3);

  for (size_t i = 0; i < 3; i++) {
    slp_object_free(items[i]);
  }
  slp_object_free(list);
  slp_object_free(index);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_at_set_overwrites(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *index1 = create_test_integer(2);
  slp_object_t *value1 = create_test_integer(100);
  slp_object_t *args1[] = {index1, value1};

  sxs_callable_t *callable =
      sxs_get_callable_for_handler(sxs_builtin_load_store);
  slp_object_t *result1 = sxs_builtin_load_store(runtime, callable, args1, 2);
  slp_object_free(result1);

  slp_object_t *index2 = create_test_integer(2);
  slp_object_t *value2 = create_test_integer(200);
  slp_object_t *args2[] = {index2, value2};
  slp_object_t *result2 = sxs_builtin_load_store(runtime, callable, args2, 2);
  ASSERT_NOT_NULL(result2);
  ASSERT_EQ(result2->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(result2->value.integer, 200);

  slp_object_free(index1);
  slp_object_free(value1);
  slp_object_free(index2);
  slp_object_free(value2);
  slp_object_free(result2);
  cleanup_test_runtime(runtime);
}

static void test_at_set_out_of_bounds(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *index = create_test_integer(9000);
  slp_object_t *value = create_test_integer(42);
  slp_object_t *args[] = {index, value};

  sxs_callable_t *callable =
      sxs_get_callable_for_handler(sxs_builtin_load_store);
  slp_object_t *result = sxs_builtin_load_store(runtime, callable, args, 2);
  assert_is_error(result);

  slp_object_free(index);
  slp_object_free(value);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_at_set_multiple_slots(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  sxs_callable_t *callable =
      sxs_get_callable_for_handler(sxs_builtin_load_store);

  for (int i = 0; i < 5; i++) {
    slp_object_t *index = create_test_integer(i);
    slp_object_t *value = create_test_integer(i * 10);
    slp_object_t *args[] = {index, value};
    slp_object_t *result = sxs_builtin_load_store(runtime, callable, args, 2);
    slp_object_free(result);
    slp_object_free(index);
    slp_object_free(value);
  }

  for (int i = 0; i < 5; i++) {
    slp_object_t *index = create_test_integer(i);
    slp_object_t *args[] = {index};
    slp_object_t *result = sxs_builtin_load_store(runtime, callable, args, 1);
    ASSERT_EQ(result->type, SLP_TYPE_INTEGER);
    ASSERT_EQ(result->value.integer, i * 10);
    slp_object_free(index);
    slp_object_free(result);
  }

  cleanup_test_runtime(runtime);
}

static void test_at_cas_success(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *index1 = create_test_integer(3);
  slp_object_t *value1 = create_test_integer(50);
  slp_object_t *set_args[] = {index1, value1};

  sxs_callable_t *callable =
      sxs_get_callable_for_handler(sxs_builtin_load_store);
  slp_object_t *set_result =
      sxs_builtin_load_store(runtime, callable, set_args, 2);
  slp_object_free(set_result);

  slp_object_t *index2 = create_test_integer(3);
  slp_object_t *compare = create_test_integer(50);
  slp_object_t *new_value = create_test_integer(75);
  slp_object_t *cas_args[] = {index2, compare, new_value};
  slp_object_t *cas_result =
      sxs_builtin_load_store(runtime, callable, cas_args, 3);
  ASSERT_NOT_NULL(cas_result);
  ASSERT_EQ(cas_result->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(cas_result->value.integer, 1);

  slp_object_free(index1);
  slp_object_free(value1);
  slp_object_free(index2);
  slp_object_free(compare);
  slp_object_free(new_value);
  slp_object_free(cas_result);
  cleanup_test_runtime(runtime);
}

static void test_at_cas_failure(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *index1 = create_test_integer(4);
  slp_object_t *value1 = create_test_integer(100);
  slp_object_t *set_args[] = {index1, value1};

  sxs_callable_t *callable =
      sxs_get_callable_for_handler(sxs_builtin_load_store);
  slp_object_t *set_result =
      sxs_builtin_load_store(runtime, callable, set_args, 2);
  slp_object_free(set_result);

  slp_object_t *index2 = create_test_integer(4);
  slp_object_t *compare = create_test_integer(99);
  slp_object_t *new_value = create_test_integer(200);
  slp_object_t *cas_args[] = {index2, compare, new_value};
  slp_object_t *cas_result =
      sxs_builtin_load_store(runtime, callable, cas_args, 3);
  ASSERT_NOT_NULL(cas_result);
  ASSERT_EQ(cas_result->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(cas_result->value.integer, 0);

  slp_object_free(index1);
  slp_object_free(value1);
  slp_object_free(index2);
  slp_object_free(compare);
  slp_object_free(new_value);
  slp_object_free(cas_result);
  cleanup_test_runtime(runtime);
}

static void test_at_cas_empty_slot(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *index = create_test_integer(7);
  slp_object_t *compare = create_test_integer(0);
  slp_object_t *new_value = create_test_integer(42);
  slp_object_t *args[] = {index, compare, new_value};

  sxs_callable_t *callable =
      sxs_get_callable_for_handler(sxs_builtin_load_store);
  slp_object_t *result = sxs_builtin_load_store(runtime, callable, args, 3);
  ASSERT_NOT_NULL(result);
  ASSERT_EQ(result->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(result->value.integer, 0);

  slp_object_free(index);
  slp_object_free(compare);
  slp_object_free(new_value);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_at_cas_out_of_bounds(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *index = create_test_integer(-5);
  slp_object_t *compare = create_test_integer(0);
  slp_object_t *new_value = create_test_integer(42);
  slp_object_t *args[] = {index, compare, new_value};

  sxs_callable_t *callable =
      sxs_get_callable_for_handler(sxs_builtin_load_store);
  slp_object_t *result = sxs_builtin_load_store(runtime, callable, args, 3);
  assert_is_error(result);

  slp_object_free(index);
  slp_object_free(compare);
  slp_object_free(new_value);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_at_cas_type_mismatch(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *index1 = create_test_integer(8);
  slp_object_t *value1 = create_test_integer(123);
  slp_object_t *set_args[] = {index1, value1};

  sxs_callable_t *callable =
      sxs_get_callable_for_handler(sxs_builtin_load_store);
  slp_object_t *set_result =
      sxs_builtin_load_store(runtime, callable, set_args, 2);
  slp_object_free(set_result);

  slp_object_t *items[] = {create_test_integer(1)};
  slp_object_t *list = create_test_list_b(items, 1);
  slp_object_t *index2 = create_test_integer(8);
  slp_object_t *new_value = create_test_integer(456);
  slp_object_t *cas_args[] = {index2, list, new_value};
  slp_object_t *cas_result =
      sxs_builtin_load_store(runtime, callable, cas_args, 3);
  ASSERT_NOT_NULL(cas_result);
  ASSERT_EQ(cas_result->type, SLP_TYPE_INTEGER);
  ASSERT_EQ(cas_result->value.integer, 0);

  slp_object_free(index1);
  slp_object_free(value1);
  slp_object_free(items[0]);
  slp_object_free(list);
  slp_object_free(index2);
  slp_object_free(new_value);
  slp_object_free(cas_result);
  cleanup_test_runtime(runtime);
}

static void test_rotl_basic(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *items[] = {create_test_integer(1), create_test_integer(2),
                           create_test_integer(3), create_test_integer(4),
                           create_test_integer(5)};
  slp_object_t *list = create_test_list_b(items, 5);
  slp_object_t *rotation = create_test_integer(2);
  slp_object_t *args[] = {list, rotation};

  sxs_callable_t *callable = sxs_get_callable_for_handler(sxs_builtin_rotl);
  slp_object_t *result = sxs_builtin_rotl(runtime, callable, args, 2);
  ASSERT_NOT_NULL(result);
  ASSERT_EQ(result->type, SLP_TYPE_LIST_B);
  ASSERT_EQ(result->value.list.count, 5);
  ASSERT_EQ(result->value.list.items[0]->value.integer, 3);
  ASSERT_EQ(result->value.list.items[1]->value.integer, 4);
  ASSERT_EQ(result->value.list.items[2]->value.integer, 5);
  ASSERT_EQ(result->value.list.items[3]->value.integer, 1);
  ASSERT_EQ(result->value.list.items[4]->value.integer, 2);

  for (size_t i = 0; i < 5; i++) {
    slp_object_free(items[i]);
  }
  slp_object_free(list);
  slp_object_free(rotation);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_rotl_zero(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *items[] = {create_test_integer(1), create_test_integer(2),
                           create_test_integer(3)};
  slp_object_t *list = create_test_list_b(items, 3);
  slp_object_t *rotation = create_test_integer(0);
  slp_object_t *args[] = {list, rotation};

  sxs_callable_t *callable = sxs_get_callable_for_handler(sxs_builtin_rotl);
  slp_object_t *result = sxs_builtin_rotl(runtime, callable, args, 2);
  ASSERT_NOT_NULL(result);
  assert_list_equals(list, result);

  for (size_t i = 0; i < 3; i++) {
    slp_object_free(items[i]);
  }
  slp_object_free(list);
  slp_object_free(rotation);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_rotl_one(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *items[] = {create_test_integer(10), create_test_integer(20),
                           create_test_integer(30)};
  slp_object_t *list = create_test_list_b(items, 3);
  slp_object_t *rotation = create_test_integer(1);
  slp_object_t *args[] = {list, rotation};

  sxs_callable_t *callable = sxs_get_callable_for_handler(sxs_builtin_rotl);
  slp_object_t *result = sxs_builtin_rotl(runtime, callable, args, 2);
  ASSERT_NOT_NULL(result);
  ASSERT_EQ(result->type, SLP_TYPE_LIST_B);
  ASSERT_EQ(result->value.list.count, 3);
  ASSERT_EQ(result->value.list.items[0]->value.integer, 20);
  ASSERT_EQ(result->value.list.items[1]->value.integer, 30);
  ASSERT_EQ(result->value.list.items[2]->value.integer, 10);

  for (size_t i = 0; i < 3; i++) {
    slp_object_free(items[i]);
  }
  slp_object_free(list);
  slp_object_free(rotation);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_rotl_full_rotation(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *items[] = {create_test_integer(1), create_test_integer(2),
                           create_test_integer(3)};
  slp_object_t *list = create_test_list_b(items, 3);
  slp_object_t *rotation = create_test_integer(3);
  slp_object_t *args[] = {list, rotation};

  sxs_callable_t *callable = sxs_get_callable_for_handler(sxs_builtin_rotl);
  slp_object_t *result = sxs_builtin_rotl(runtime, callable, args, 2);
  ASSERT_NOT_NULL(result);
  assert_list_equals(list, result);

  for (size_t i = 0; i < 3; i++) {
    slp_object_free(items[i]);
  }
  slp_object_free(list);
  slp_object_free(rotation);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_rotl_empty_list(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *list = create_test_list_b(NULL, 0);
  slp_object_t *rotation = create_test_integer(5);
  slp_object_t *args[] = {list, rotation};

  sxs_callable_t *callable = sxs_get_callable_for_handler(sxs_builtin_rotl);
  slp_object_t *result = sxs_builtin_rotl(runtime, callable, args, 2);
  ASSERT_NOT_NULL(result);
  ASSERT_EQ(result->type, SLP_TYPE_LIST_B);
  ASSERT_EQ(result->value.list.count, 0);

  slp_object_free(list);
  slp_object_free(rotation);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_rotl_larger_than_size(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *items[] = {create_test_integer(1), create_test_integer(2),
                           create_test_integer(3)};
  slp_object_t *list = create_test_list_b(items, 3);
  slp_object_t *rotation = create_test_integer(7);
  slp_object_t *args[] = {list, rotation};

  sxs_callable_t *callable = sxs_get_callable_for_handler(sxs_builtin_rotl);
  slp_object_t *result = sxs_builtin_rotl(runtime, callable, args, 2);
  ASSERT_NOT_NULL(result);
  ASSERT_EQ(result->type, SLP_TYPE_LIST_B);
  ASSERT_EQ(result->value.list.count, 3);
  ASSERT_EQ(result->value.list.items[0]->value.integer, 2);
  ASSERT_EQ(result->value.list.items[1]->value.integer, 3);
  ASSERT_EQ(result->value.list.items[2]->value.integer, 1);

  for (size_t i = 0; i < 3; i++) {
    slp_object_free(items[i]);
  }
  slp_object_free(list);
  slp_object_free(rotation);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_rotl_negative(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *items[] = {create_test_integer(1), create_test_integer(2),
                           create_test_integer(3), create_test_integer(4)};
  slp_object_t *list = create_test_list_b(items, 4);
  slp_object_t *rotation = create_test_integer(-1);
  slp_object_t *args[] = {list, rotation};

  sxs_callable_t *callable = sxs_get_callable_for_handler(sxs_builtin_rotl);
  slp_object_t *result = sxs_builtin_rotl(runtime, callable, args, 2);
  ASSERT_NOT_NULL(result);
  ASSERT_EQ(result->type, SLP_TYPE_LIST_B);
  ASSERT_EQ(result->value.list.count, 4);
  ASSERT_EQ(result->value.list.items[0]->value.integer, 4);
  ASSERT_EQ(result->value.list.items[1]->value.integer, 1);
  ASSERT_EQ(result->value.list.items[2]->value.integer, 2);
  ASSERT_EQ(result->value.list.items[3]->value.integer, 3);

  for (size_t i = 0; i < 4; i++) {
    slp_object_free(items[i]);
  }
  slp_object_free(list);
  slp_object_free(rotation);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_rotl_single_element(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *items[] = {create_test_integer(42)};
  slp_object_t *list = create_test_list_b(items, 1);
  slp_object_t *rotation = create_test_integer(5);
  slp_object_t *args[] = {list, rotation};

  sxs_callable_t *callable = sxs_get_callable_for_handler(sxs_builtin_rotl);
  slp_object_t *result = sxs_builtin_rotl(runtime, callable, args, 2);
  ASSERT_NOT_NULL(result);
  assert_list_equals(list, result);

  slp_object_free(items[0]);
  slp_object_free(list);
  slp_object_free(rotation);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_rotl_bracket_list(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *items[] = {create_test_integer(1), create_test_integer(2)};
  slp_object_t *list = create_test_list_b(items, 2);
  slp_object_t *rotation = create_test_integer(1);
  slp_object_t *args[] = {list, rotation};

  sxs_callable_t *callable = sxs_get_callable_for_handler(sxs_builtin_rotl);
  slp_object_t *result = sxs_builtin_rotl(runtime, callable, args, 2);
  ASSERT_NOT_NULL(result);
  ASSERT_EQ(result->type, SLP_TYPE_LIST_B);

  for (size_t i = 0; i < 2; i++) {
    slp_object_free(items[i]);
  }
  slp_object_free(list);
  slp_object_free(rotation);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_rotl_curly_list(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *items[] = {create_test_integer(1), create_test_integer(2)};
  slp_object_t *list = create_test_list_c(items, 2);
  slp_object_t *rotation = create_test_integer(1);
  slp_object_t *args[] = {list, rotation};

  sxs_callable_t *callable = sxs_get_callable_for_handler(sxs_builtin_rotl);
  slp_object_t *result = sxs_builtin_rotl(runtime, callable, args, 2);
  ASSERT_NOT_NULL(result);
  ASSERT_EQ(result->type, SLP_TYPE_LIST_C);

  for (size_t i = 0; i < 2; i++) {
    slp_object_free(items[i]);
  }
  slp_object_free(list);
  slp_object_free(rotation);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_rotl_non_list_arg(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *not_a_list = create_test_integer(42);
  slp_object_t *rotation = create_test_integer(1);
  slp_object_t *args[] = {not_a_list, rotation};

  sxs_callable_t *callable = sxs_get_callable_for_handler(sxs_builtin_rotl);
  slp_object_t *result = sxs_builtin_rotl(runtime, callable, args, 2);
  assert_is_error(result);

  slp_object_free(not_a_list);
  slp_object_free(rotation);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_rotl_non_integer_rotation(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *items[] = {create_test_integer(1)};
  slp_object_t *list = create_test_list_b(items, 1);
  slp_object_t *not_an_int = create_test_list_b(NULL, 0);
  slp_object_t *args[] = {list, not_an_int};

  sxs_callable_t *callable = sxs_get_callable_for_handler(sxs_builtin_rotl);
  slp_object_t *result = sxs_builtin_rotl(runtime, callable, args, 2);
  assert_is_error(result);

  slp_object_free(items[0]);
  slp_object_free(list);
  slp_object_free(not_an_int);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_rotr_basic(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *items[] = {create_test_integer(1), create_test_integer(2),
                           create_test_integer(3), create_test_integer(4),
                           create_test_integer(5)};
  slp_object_t *list = create_test_list_b(items, 5);
  slp_object_t *rotation = create_test_integer(2);
  slp_object_t *args[] = {list, rotation};

  sxs_callable_t *callable = sxs_get_callable_for_handler(sxs_builtin_rotr);
  slp_object_t *result = sxs_builtin_rotr(runtime, callable, args, 2);
  ASSERT_NOT_NULL(result);
  ASSERT_EQ(result->type, SLP_TYPE_LIST_B);
  ASSERT_EQ(result->value.list.count, 5);
  ASSERT_EQ(result->value.list.items[0]->value.integer, 4);
  ASSERT_EQ(result->value.list.items[1]->value.integer, 5);
  ASSERT_EQ(result->value.list.items[2]->value.integer, 1);
  ASSERT_EQ(result->value.list.items[3]->value.integer, 2);
  ASSERT_EQ(result->value.list.items[4]->value.integer, 3);

  for (size_t i = 0; i < 5; i++) {
    slp_object_free(items[i]);
  }
  slp_object_free(list);
  slp_object_free(rotation);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_rotr_zero(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *items[] = {create_test_integer(1), create_test_integer(2),
                           create_test_integer(3)};
  slp_object_t *list = create_test_list_b(items, 3);
  slp_object_t *rotation = create_test_integer(0);
  slp_object_t *args[] = {list, rotation};

  sxs_callable_t *callable = sxs_get_callable_for_handler(sxs_builtin_rotr);
  slp_object_t *result = sxs_builtin_rotr(runtime, callable, args, 2);
  ASSERT_NOT_NULL(result);
  assert_list_equals(list, result);

  for (size_t i = 0; i < 3; i++) {
    slp_object_free(items[i]);
  }
  slp_object_free(list);
  slp_object_free(rotation);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_rotr_one(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *items[] = {create_test_integer(10), create_test_integer(20),
                           create_test_integer(30)};
  slp_object_t *list = create_test_list_b(items, 3);
  slp_object_t *rotation = create_test_integer(1);
  slp_object_t *args[] = {list, rotation};

  sxs_callable_t *callable = sxs_get_callable_for_handler(sxs_builtin_rotr);
  slp_object_t *result = sxs_builtin_rotr(runtime, callable, args, 2);
  ASSERT_NOT_NULL(result);
  ASSERT_EQ(result->type, SLP_TYPE_LIST_B);
  ASSERT_EQ(result->value.list.count, 3);
  ASSERT_EQ(result->value.list.items[0]->value.integer, 30);
  ASSERT_EQ(result->value.list.items[1]->value.integer, 10);
  ASSERT_EQ(result->value.list.items[2]->value.integer, 20);

  for (size_t i = 0; i < 3; i++) {
    slp_object_free(items[i]);
  }
  slp_object_free(list);
  slp_object_free(rotation);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_rotr_full_rotation(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *items[] = {create_test_integer(1), create_test_integer(2),
                           create_test_integer(3)};
  slp_object_t *list = create_test_list_b(items, 3);
  slp_object_t *rotation = create_test_integer(3);
  slp_object_t *args[] = {list, rotation};

  sxs_callable_t *callable = sxs_get_callable_for_handler(sxs_builtin_rotr);
  slp_object_t *result = sxs_builtin_rotr(runtime, callable, args, 2);
  ASSERT_NOT_NULL(result);
  assert_list_equals(list, result);

  for (size_t i = 0; i < 3; i++) {
    slp_object_free(items[i]);
  }
  slp_object_free(list);
  slp_object_free(rotation);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_rotr_empty_list(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *list = create_test_list_b(NULL, 0);
  slp_object_t *rotation = create_test_integer(5);
  slp_object_t *args[] = {list, rotation};

  sxs_callable_t *callable = sxs_get_callable_for_handler(sxs_builtin_rotr);
  slp_object_t *result = sxs_builtin_rotr(runtime, callable, args, 2);
  ASSERT_NOT_NULL(result);
  ASSERT_EQ(result->type, SLP_TYPE_LIST_B);
  ASSERT_EQ(result->value.list.count, 0);

  slp_object_free(list);
  slp_object_free(rotation);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_rotr_larger_than_size(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *items[] = {create_test_integer(1), create_test_integer(2),
                           create_test_integer(3)};
  slp_object_t *list = create_test_list_b(items, 3);
  slp_object_t *rotation = create_test_integer(7);
  slp_object_t *args[] = {list, rotation};

  sxs_callable_t *callable = sxs_get_callable_for_handler(sxs_builtin_rotr);
  slp_object_t *result = sxs_builtin_rotr(runtime, callable, args, 2);
  ASSERT_NOT_NULL(result);
  ASSERT_EQ(result->type, SLP_TYPE_LIST_B);
  ASSERT_EQ(result->value.list.count, 3);
  ASSERT_EQ(result->value.list.items[0]->value.integer, 3);
  ASSERT_EQ(result->value.list.items[1]->value.integer, 1);
  ASSERT_EQ(result->value.list.items[2]->value.integer, 2);

  for (size_t i = 0; i < 3; i++) {
    slp_object_free(items[i]);
  }
  slp_object_free(list);
  slp_object_free(rotation);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_rotr_negative(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *items[] = {create_test_integer(1), create_test_integer(2),
                           create_test_integer(3), create_test_integer(4)};
  slp_object_t *list = create_test_list_b(items, 4);
  slp_object_t *rotation = create_test_integer(-1);
  slp_object_t *args[] = {list, rotation};

  sxs_callable_t *callable = sxs_get_callable_for_handler(sxs_builtin_rotr);
  slp_object_t *result = sxs_builtin_rotr(runtime, callable, args, 2);
  ASSERT_NOT_NULL(result);
  ASSERT_EQ(result->type, SLP_TYPE_LIST_B);
  ASSERT_EQ(result->value.list.count, 4);
  ASSERT_EQ(result->value.list.items[0]->value.integer, 2);
  ASSERT_EQ(result->value.list.items[1]->value.integer, 3);
  ASSERT_EQ(result->value.list.items[2]->value.integer, 4);
  ASSERT_EQ(result->value.list.items[3]->value.integer, 1);

  for (size_t i = 0; i < 4; i++) {
    slp_object_free(items[i]);
  }
  slp_object_free(list);
  slp_object_free(rotation);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_rotr_single_element(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *items[] = {create_test_integer(42)};
  slp_object_t *list = create_test_list_b(items, 1);
  slp_object_t *rotation = create_test_integer(5);
  slp_object_t *args[] = {list, rotation};

  sxs_callable_t *callable = sxs_get_callable_for_handler(sxs_builtin_rotr);
  slp_object_t *result = sxs_builtin_rotr(runtime, callable, args, 2);
  ASSERT_NOT_NULL(result);
  assert_list_equals(list, result);

  slp_object_free(items[0]);
  slp_object_free(list);
  slp_object_free(rotation);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_rotr_bracket_list(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *items[] = {create_test_integer(1), create_test_integer(2)};
  slp_object_t *list = create_test_list_b(items, 2);
  slp_object_t *rotation = create_test_integer(1);
  slp_object_t *args[] = {list, rotation};

  sxs_callable_t *callable = sxs_get_callable_for_handler(sxs_builtin_rotr);
  slp_object_t *result = sxs_builtin_rotr(runtime, callable, args, 2);
  ASSERT_NOT_NULL(result);
  ASSERT_EQ(result->type, SLP_TYPE_LIST_B);

  for (size_t i = 0; i < 2; i++) {
    slp_object_free(items[i]);
  }
  slp_object_free(list);
  slp_object_free(rotation);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_rotr_curly_list(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *items[] = {create_test_integer(1), create_test_integer(2)};
  slp_object_t *list = create_test_list_c(items, 2);
  slp_object_t *rotation = create_test_integer(1);
  slp_object_t *args[] = {list, rotation};

  sxs_callable_t *callable = sxs_get_callable_for_handler(sxs_builtin_rotr);
  slp_object_t *result = sxs_builtin_rotr(runtime, callable, args, 2);
  ASSERT_NOT_NULL(result);
  ASSERT_EQ(result->type, SLP_TYPE_LIST_C);

  for (size_t i = 0; i < 2; i++) {
    slp_object_free(items[i]);
  }
  slp_object_free(list);
  slp_object_free(rotation);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_rotr_non_list_arg(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *not_a_list = create_test_integer(42);
  slp_object_t *rotation = create_test_integer(1);
  slp_object_t *args[] = {not_a_list, rotation};

  sxs_callable_t *callable = sxs_get_callable_for_handler(sxs_builtin_rotr);
  slp_object_t *result = sxs_builtin_rotr(runtime, callable, args, 2);
  assert_is_error(result);

  slp_object_free(not_a_list);
  slp_object_free(rotation);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

static void test_rotr_non_integer_rotation(void) {
  sxs_runtime_t *runtime = create_test_runtime();

  slp_object_t *items[] = {create_test_integer(1)};
  slp_object_t *list = create_test_list_b(items, 1);
  slp_object_t *not_an_int = create_test_list_b(NULL, 0);
  slp_object_t *args[] = {list, not_an_int};

  sxs_callable_t *callable = sxs_get_callable_for_handler(sxs_builtin_rotr);
  slp_object_t *result = sxs_builtin_rotr(runtime, callable, args, 2);
  assert_is_error(result);

  slp_object_free(items[0]);
  slp_object_free(list);
  slp_object_free(not_an_int);
  slp_object_free(result);
  cleanup_test_runtime(runtime);
}

int main(void) {
  sxs_builtins_init();

  test_at_get_valid_index();
  test_at_get_empty_slot();
  test_at_get_out_of_bounds_negative();
  test_at_get_out_of_bounds_large();
  test_at_set_integer();
  test_at_set_list();
  test_at_set_overwrites();
  test_at_set_out_of_bounds();
  test_at_set_multiple_slots();
  test_at_cas_success();
  test_at_cas_failure();
  test_at_cas_empty_slot();
  test_at_cas_out_of_bounds();
  test_at_cas_type_mismatch();

  test_rotl_basic();
  test_rotl_zero();
  test_rotl_one();
  test_rotl_full_rotation();
  test_rotl_empty_list();
  test_rotl_larger_than_size();
  test_rotl_negative();
  test_rotl_single_element();
  test_rotl_bracket_list();
  test_rotl_curly_list();
  test_rotl_non_list_arg();
  test_rotl_non_integer_rotation();

  test_rotr_basic();
  test_rotr_zero();
  test_rotr_one();
  test_rotr_full_rotation();
  test_rotr_empty_list();
  test_rotr_larger_than_size();
  test_rotr_negative();
  test_rotr_single_element();
  test_rotr_bracket_list();
  test_rotr_curly_list();
  test_rotr_non_list_arg();
  test_rotr_non_integer_rotation();

  sxs_builtins_deinit();

  printf("All sxs_builtins tests passed!\n");
  return 0;
}
