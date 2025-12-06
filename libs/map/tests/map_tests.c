#include "../map.h"
#include "test.h"
#include <string.h>

void test_map_init_deinit(void) {
  map_int_t map;
  map_init(&map);

  ASSERT_EQ(map.base.nbuckets, 0);
  ASSERT_EQ(map.base.nnodes, 0);
  ASSERT_NULL(map.base.buckets);

  map_deinit(&map);
}

void test_map_set_get_int(void) {
  map_int_t map;
  map_init(&map);

  int result = map_set(&map, "key1", 42);
  ASSERT_EQ(result, 0);
  ASSERT_EQ(map.base.nnodes, 1);

  int *value = map_get(&map, "key1");
  ASSERT_NOT_NULL(value);
  ASSERT_EQ(*value, 42);

  map_deinit(&map);
}

void test_map_set_get_multiple(void) {
  map_int_t map;
  map_init(&map);

  map_set(&map, "one", 1);
  map_set(&map, "two", 2);
  map_set(&map, "three", 3);
  map_set(&map, "four", 4);
  map_set(&map, "five", 5);

  ASSERT_EQ(map.base.nnodes, 5);

  ASSERT_EQ(*map_get(&map, "one"), 1);
  ASSERT_EQ(*map_get(&map, "two"), 2);
  ASSERT_EQ(*map_get(&map, "three"), 3);
  ASSERT_EQ(*map_get(&map, "four"), 4);
  ASSERT_EQ(*map_get(&map, "five"), 5);

  map_deinit(&map);
}

void test_map_update_existing(void) {
  map_int_t map;
  map_init(&map);

  map_set(&map, "key", 100);
  ASSERT_EQ(*map_get(&map, "key"), 100);

  map_set(&map, "key", 200);
  ASSERT_EQ(*map_get(&map, "key"), 200);
  ASSERT_EQ(map.base.nnodes, 1);

  map_deinit(&map);
}

void test_map_get_nonexistent(void) {
  map_int_t map;
  map_init(&map);

  int *value = map_get(&map, "nonexistent");
  ASSERT_NULL(value);

  map_deinit(&map);
}

void test_map_remove(void) {
  map_int_t map;
  map_init(&map);

  map_set(&map, "key1", 10);
  map_set(&map, "key2", 20);
  map_set(&map, "key3", 30);
  ASSERT_EQ(map.base.nnodes, 3);

  map_remove(&map, "key2");
  ASSERT_EQ(map.base.nnodes, 2);

  ASSERT_EQ(*map_get(&map, "key1"), 10);
  ASSERT_NULL(map_get(&map, "key2"));
  ASSERT_EQ(*map_get(&map, "key3"), 30);

  map_deinit(&map);
}

void test_map_remove_nonexistent(void) {
  map_int_t map;
  map_init(&map);

  map_set(&map, "key", 42);
  ASSERT_EQ(map.base.nnodes, 1);

  map_remove(&map, "nonexistent");
  ASSERT_EQ(map.base.nnodes, 1);

  map_deinit(&map);
}

void test_map_string_values(void) {
  map_str_t map;
  map_init(&map);

  char *str1 = "hello";
  char *str2 = "world";
  char *str3 = "test";

  map_set(&map, "greeting", str1);
  map_set(&map, "noun", str2);
  map_set(&map, "action", str3);

  char **value1 = map_get(&map, "greeting");
  char **value2 = map_get(&map, "noun");
  char **value3 = map_get(&map, "action");

  ASSERT_NOT_NULL(value1);
  ASSERT_NOT_NULL(value2);
  ASSERT_NOT_NULL(value3);

  ASSERT_STR_EQ(*value1, "hello");
  ASSERT_STR_EQ(*value2, "world");
  ASSERT_STR_EQ(*value3, "test");

  map_deinit(&map);
}

void test_map_pointer_values(void) {
  map_void_t map;
  map_init(&map);

  int data1 = 100;
  int data2 = 200;
  int data3 = 300;

  map_set(&map, "ptr1", &data1);
  map_set(&map, "ptr2", &data2);
  map_set(&map, "ptr3", &data3);

  void **ptr1 = map_get(&map, "ptr1");
  void **ptr2 = map_get(&map, "ptr2");
  void **ptr3 = map_get(&map, "ptr3");

  ASSERT_NOT_NULL(ptr1);
  ASSERT_NOT_NULL(ptr2);
  ASSERT_NOT_NULL(ptr3);

  ASSERT_EQ(*(int *)*ptr1, 100);
  ASSERT_EQ(*(int *)*ptr2, 200);
  ASSERT_EQ(*(int *)*ptr3, 300);

  map_deinit(&map);
}

void test_map_iterator_empty(void) {
  map_int_t map;
  map_init(&map);

  map_iter_t iter = map_iter(&map);
  const char *key = map_next(&map, &iter);

  ASSERT_NULL(key);

  map_deinit(&map);
}

void test_map_iterator_single(void) {
  map_int_t map;
  map_init(&map);

  map_set(&map, "only", 42);

  map_iter_t iter = map_iter(&map);
  const char *key = map_next(&map, &iter);

  ASSERT_NOT_NULL(key);
  ASSERT_STR_EQ(key, "only");
  ASSERT_EQ(*map_get(&map, key), 42);

  key = map_next(&map, &iter);
  ASSERT_NULL(key);

  map_deinit(&map);
}

void test_map_iterator_multiple(void) {
  map_int_t map;
  map_init(&map);

  map_set(&map, "alpha", 1);
  map_set(&map, "beta", 2);
  map_set(&map, "gamma", 3);
  map_set(&map, "delta", 4);

  int count = 0;
  int sum = 0;

  map_iter_t iter = map_iter(&map);
  const char *key;
  while ((key = map_next(&map, &iter))) {
    int *value = map_get(&map, key);
    ASSERT_NOT_NULL(value);
    sum += *value;
    count++;
  }

  ASSERT_EQ(count, 4);
  ASSERT_EQ(sum, 10);

  map_deinit(&map);
}

void test_map_resize_behavior(void) {
  map_int_t map;
  map_init(&map);

  for (int i = 0; i < 100; i++) {
    char key[32];
    snprintf(key, sizeof(key), "key%d", i);
    map_set(&map, key, i);
  }

  ASSERT_EQ(map.base.nnodes, 100);
  ASSERT_TRUE(map.base.nbuckets >= 100);

  for (int i = 0; i < 100; i++) {
    char key[32];
    snprintf(key, sizeof(key), "key%d", i);
    int *value = map_get(&map, key);
    ASSERT_NOT_NULL(value);
    ASSERT_EQ(*value, i);
  }

  map_deinit(&map);
}

void test_map_collision_handling(void) {
  map_int_t map;
  map_init(&map);

  map_set(&map, "a", 1);
  map_set(&map, "aa", 2);
  map_set(&map, "aaa", 3);
  map_set(&map, "aaaa", 4);
  map_set(&map, "aaaaa", 5);

  ASSERT_EQ(*map_get(&map, "a"), 1);
  ASSERT_EQ(*map_get(&map, "aa"), 2);
  ASSERT_EQ(*map_get(&map, "aaa"), 3);
  ASSERT_EQ(*map_get(&map, "aaaa"), 4);
  ASSERT_EQ(*map_get(&map, "aaaaa"), 5);

  map_deinit(&map);
}

void test_map_empty_key(void) {
  map_int_t map;
  map_init(&map);

  map_set(&map, "", 999);

  int *value = map_get(&map, "");
  ASSERT_NOT_NULL(value);
  ASSERT_EQ(*value, 999);

  map_deinit(&map);
}

void test_map_long_keys(void) {
  map_int_t map;
  map_init(&map);

  char long_key1[256];
  char long_key2[256];
  memset(long_key1, 'x', 255);
  memset(long_key2, 'y', 255);
  long_key1[255] = '\0';
  long_key2[255] = '\0';

  map_set(&map, long_key1, 111);
  map_set(&map, long_key2, 222);

  ASSERT_EQ(*map_get(&map, long_key1), 111);
  ASSERT_EQ(*map_get(&map, long_key2), 222);

  map_deinit(&map);
}

void test_map_remove_all(void) {
  map_int_t map;
  map_init(&map);

  map_set(&map, "k1", 1);
  map_set(&map, "k2", 2);
  map_set(&map, "k3", 3);

  map_remove(&map, "k1");
  map_remove(&map, "k2");
  map_remove(&map, "k3");

  ASSERT_EQ(map.base.nnodes, 0);
  ASSERT_NULL(map_get(&map, "k1"));
  ASSERT_NULL(map_get(&map, "k2"));
  ASSERT_NULL(map_get(&map, "k3"));

  map_deinit(&map);
}

void test_map_reuse_after_clear(void) {
  map_int_t map;
  map_init(&map);

  map_set(&map, "first", 1);
  map_set(&map, "second", 2);

  map_deinit(&map);

  map_init(&map);

  map_set(&map, "third", 3);
  map_set(&map, "fourth", 4);

  ASSERT_EQ(map.base.nnodes, 2);
  ASSERT_EQ(*map_get(&map, "third"), 3);
  ASSERT_EQ(*map_get(&map, "fourth"), 4);

  map_deinit(&map);
}

void test_map_float_values(void) {
  map_float_t map;
  map_init(&map);

  map_set(&map, "pi", 3.14159f);
  map_set(&map, "e", 2.71828f);
  map_set(&map, "phi", 1.61803f);

  float *pi = map_get(&map, "pi");
  float *e = map_get(&map, "e");
  float *phi = map_get(&map, "phi");

  ASSERT_NOT_NULL(pi);
  ASSERT_NOT_NULL(e);
  ASSERT_NOT_NULL(phi);

  ASSERT_TRUE(*pi > 3.14f && *pi < 3.15f);
  ASSERT_TRUE(*e > 2.71f && *e < 2.72f);
  ASSERT_TRUE(*phi > 1.61f && *phi < 1.62f);

  map_deinit(&map);
}

void test_map_double_values(void) {
  map_double_t map;
  map_init(&map);

  map_set(&map, "large", 1234567890.123456);
  map_set(&map, "small", 0.000000123456);

  double *large = map_get(&map, "large");
  double *small = map_get(&map, "small");

  ASSERT_NOT_NULL(large);
  ASSERT_NOT_NULL(small);

  ASSERT_TRUE(*large > 1234567890.0);
  ASSERT_TRUE(*small < 0.001);

  map_deinit(&map);
}

void test_map_char_values(void) {
  map_char_t map;
  map_init(&map);

  map_set(&map, "letter_a", 'A');
  map_set(&map, "letter_b", 'B');
  map_set(&map, "letter_z", 'Z');

  char *a = map_get(&map, "letter_a");
  char *b = map_get(&map, "letter_b");
  char *z = map_get(&map, "letter_z");

  ASSERT_NOT_NULL(a);
  ASSERT_NOT_NULL(b);
  ASSERT_NOT_NULL(z);

  ASSERT_EQ(*a, 'A');
  ASSERT_EQ(*b, 'B');
  ASSERT_EQ(*z, 'Z');

  map_deinit(&map);
}

void test_map_stress_test(void) {
  map_int_t map;
  map_init(&map);

  for (int i = 0; i < 1000; i++) {
    char key[32];
    snprintf(key, sizeof(key), "stress_key_%d", i);
    map_set(&map, key, i * 2);
  }

  ASSERT_EQ(map.base.nnodes, 1000);

  for (int i = 0; i < 1000; i++) {
    char key[32];
    snprintf(key, sizeof(key), "stress_key_%d", i);
    int *value = map_get(&map, key);
    ASSERT_NOT_NULL(value);
    ASSERT_EQ(*value, i * 2);
  }

  for (int i = 0; i < 500; i++) {
    char key[32];
    snprintf(key, sizeof(key), "stress_key_%d", i * 2);
    map_remove(&map, key);
  }

  ASSERT_EQ(map.base.nnodes, 500);

  map_deinit(&map);
}

void test_map_iterator_after_remove(void) {
  map_int_t map;
  map_init(&map);

  map_set(&map, "keep1", 1);
  map_set(&map, "remove", 2);
  map_set(&map, "keep2", 3);

  map_remove(&map, "remove");

  int count = 0;
  map_iter_t iter = map_iter(&map);
  const char *key;
  while ((key = map_next(&map, &iter))) {
    ASSERT_TRUE(strcmp(key, "remove") != 0);
    count++;
  }

  ASSERT_EQ(count, 2);

  map_deinit(&map);
}

void test_map_memory_leak_check(void) {
  for (int round = 0; round < 10; round++) {
    map_int_t map;
    map_init(&map);

    for (int i = 0; i < 100; i++) {
      char key[32];
      snprintf(key, sizeof(key), "key_%d", i);
      map_set(&map, key, i);
    }

    for (int i = 0; i < 50; i++) {
      char key[32];
      snprintf(key, sizeof(key), "key_%d", i);
      map_remove(&map, key);
    }

    map_deinit(&map);
  }
}

int main(void) {
  test_map_init_deinit();
  test_map_set_get_int();
  test_map_set_get_multiple();
  test_map_update_existing();
  test_map_get_nonexistent();
  test_map_remove();
  test_map_remove_nonexistent();
  test_map_string_values();
  test_map_pointer_values();
  test_map_iterator_empty();
  test_map_iterator_single();
  test_map_iterator_multiple();
  test_map_resize_behavior();
  test_map_collision_handling();
  test_map_empty_key();
  test_map_long_keys();
  test_map_remove_all();
  test_map_reuse_after_clear();
  test_map_float_values();
  test_map_double_values();
  test_map_char_values();
  test_map_stress_test();
  test_map_iterator_after_remove();
  test_map_memory_leak_check();

  return 0;
}
