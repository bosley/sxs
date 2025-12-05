#include "../buffer.h"
#include "test.h"
#include <string.h>

void test_split_out_of_bounds_index(void) {
  slp_buffer_t *buffer = slp_buffer_new(32);
  uint8_t data[] = {1, 2, 3, 4, 5};
  slp_buffer_copy_to(buffer, data, 5);

  split_buffer_t split = slp_buffer_split(buffer, 10, 32, 32);

  ASSERT_NULL(split.left);
  ASSERT_NULL(split.right);

  slp_buffer_free(buffer);
}

void test_split_null_buffer(void) {
  split_buffer_t split = slp_buffer_split(NULL, 2, 32, 32);

  ASSERT_NULL(split.left);
  ASSERT_NULL(split.right);
}

void test_split_empty_buffer(void) {
  slp_buffer_t *buffer = slp_buffer_new(32);

  split_buffer_t split = slp_buffer_split(buffer, 0, 32, 32);

  ASSERT_NOT_NULL(split.left);
  ASSERT_NOT_NULL(split.right);
  ASSERT_EQ(slp_buffer_count(split.left), 0);
  ASSERT_EQ(slp_buffer_count(split.right), 0);

  slp_split_buffer_free(&split);
  slp_buffer_free(buffer);
}

void test_split_l_and_r_size_validation(void) {
  slp_buffer_t *buffer = slp_buffer_new(100);
  uint8_t data[50];
  for (int i = 0; i < 50; i++) {
    data[i] = i;
  }
  slp_buffer_copy_to(buffer, data, 50);

  split_buffer_t split = slp_buffer_split(buffer, 25, 30, 40);

  ASSERT_NOT_NULL(split.left);
  ASSERT_NOT_NULL(split.right);

  ASSERT_EQ(slp_buffer_count(split.left), 25);
  ASSERT_EQ(slp_buffer_count(split.right), 25);

  ASSERT_EQ(split.left->capacity, 25);
  ASSERT_EQ(split.right->capacity, 25);

  slp_split_buffer_free(&split);
  slp_buffer_free(buffer);
}

void test_split_value_validation_basic(void) {
  slp_buffer_t *buffer = slp_buffer_new(32);
  uint8_t data[] = {10, 20, 30, 40, 50, 60, 70, 80};
  slp_buffer_copy_to(buffer, data, 8);

  split_buffer_t split = slp_buffer_split(buffer, 5, 32, 32);

  ASSERT_NOT_NULL(split.left);
  ASSERT_NOT_NULL(split.right);

  ASSERT_EQ(slp_buffer_count(split.left), 5);
  ASSERT_EQ(slp_buffer_count(split.right), 3);

  uint8_t *left_data = slp_buffer_data(split.left);
  ASSERT_EQ(left_data[0], 10);
  ASSERT_EQ(left_data[1], 20);
  ASSERT_EQ(left_data[2], 30);
  ASSERT_EQ(left_data[3], 40);
  ASSERT_EQ(left_data[4], 50);

  uint8_t *right_data = slp_buffer_data(split.right);
  ASSERT_EQ(right_data[0], 60);
  ASSERT_EQ(right_data[1], 70);
  ASSERT_EQ(right_data[2], 80);

  slp_split_buffer_free(&split);
  slp_buffer_free(buffer);
}

void test_split_index_exclusive(void) {
  slp_buffer_t *buffer = slp_buffer_new(32);
  uint8_t data[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  slp_buffer_copy_to(buffer, data, 10);

  split_buffer_t split = slp_buffer_split(buffer, 3, 32, 32);

  ASSERT_NOT_NULL(split.left);
  ASSERT_NOT_NULL(split.right);

  ASSERT_EQ(slp_buffer_count(split.left), 3);
  ASSERT_EQ(slp_buffer_count(split.right), 7);

  uint8_t *left_data = slp_buffer_data(split.left);
  ASSERT_EQ(left_data[0], 0);
  ASSERT_EQ(left_data[1], 1);
  ASSERT_EQ(left_data[2], 2);

  uint8_t *right_data = slp_buffer_data(split.right);
  ASSERT_EQ(right_data[0], 3);
  ASSERT_EQ(right_data[1], 4);
  ASSERT_EQ(right_data[2], 5);

  slp_split_buffer_free(&split);
  slp_buffer_free(buffer);
}

void test_split_left_under_min_buffer_size(void) {
  slp_buffer_t *buffer = slp_buffer_new(32);
  uint8_t data[] = {1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                    11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
  slp_buffer_copy_to(buffer, data, 20);

  split_buffer_t split = slp_buffer_split(buffer, 5, 100, 100);

  ASSERT_NOT_NULL(split.left);
  ASSERT_NOT_NULL(split.right);

  ASSERT_EQ(slp_buffer_count(split.left), 5);
  ASSERT_EQ(split.left->capacity, 16);

  uint8_t *left_data = slp_buffer_data(split.left);
  for (int i = 0; i < 5; i++) {
    ASSERT_EQ(left_data[i], i + 1);
  }

  slp_split_buffer_free(&split);
  slp_buffer_free(buffer);
}

void test_split_right_under_min_buffer_size(void) {
  slp_buffer_t *buffer = slp_buffer_new(32);
  uint8_t data[] = {1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                    11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
  slp_buffer_copy_to(buffer, data, 20);

  split_buffer_t split = slp_buffer_split(buffer, 15, 100, 100);

  ASSERT_NOT_NULL(split.left);
  ASSERT_NOT_NULL(split.right);

  ASSERT_EQ(slp_buffer_count(split.right), 5);
  ASSERT_EQ(split.right->capacity, 16);

  uint8_t *right_data = slp_buffer_data(split.right);
  for (int i = 0; i < 5; i++) {
    ASSERT_EQ(right_data[i], 16 + i);
  }

  slp_split_buffer_free(&split);
  slp_buffer_free(buffer);
}

void test_split_both_under_min_buffer_size(void) {
  slp_buffer_t *buffer = slp_buffer_new(32);
  uint8_t data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  slp_buffer_copy_to(buffer, data, 10);

  split_buffer_t split = slp_buffer_split(buffer, 5, 100, 100);

  ASSERT_NOT_NULL(split.left);
  ASSERT_NOT_NULL(split.right);

  ASSERT_EQ(slp_buffer_count(split.left), 5);
  ASSERT_EQ(split.left->capacity, 16);

  ASSERT_EQ(slp_buffer_count(split.right), 5);
  ASSERT_EQ(split.right->capacity, 16);

  uint8_t *left_data = slp_buffer_data(split.left);
  for (int i = 0; i < 5; i++) {
    ASSERT_EQ(left_data[i], i + 1);
  }

  uint8_t *right_data = slp_buffer_data(split.right);
  for (int i = 0; i < 5; i++) {
    ASSERT_EQ(right_data[i], 6 + i);
  }

  slp_split_buffer_free(&split);
  slp_buffer_free(buffer);
}

void test_split_at_start(void) {
  slp_buffer_t *buffer = slp_buffer_new(32);
  uint8_t data[] = {1, 2, 3, 4, 5};
  slp_buffer_copy_to(buffer, data, 5);

  split_buffer_t split = slp_buffer_split(buffer, 0, 32, 32);

  ASSERT_NOT_NULL(split.left);
  ASSERT_NOT_NULL(split.right);

  ASSERT_EQ(slp_buffer_count(split.left), 0);
  ASSERT_EQ(slp_buffer_count(split.right), 5);

  uint8_t *right_data = slp_buffer_data(split.right);
  for (int i = 0; i < 5; i++) {
    ASSERT_EQ(right_data[i], i + 1);
  }

  slp_split_buffer_free(&split);
  slp_buffer_free(buffer);
}

void test_split_at_end(void) {
  slp_buffer_t *buffer = slp_buffer_new(32);
  uint8_t data[] = {1, 2, 3, 4, 5};
  slp_buffer_copy_to(buffer, data, 5);

  split_buffer_t split = slp_buffer_split(buffer, 5, 32, 32);

  ASSERT_NOT_NULL(split.left);
  ASSERT_NOT_NULL(split.right);

  ASSERT_EQ(slp_buffer_count(split.left), 5);
  ASSERT_EQ(slp_buffer_count(split.right), 0);

  uint8_t *left_data = slp_buffer_data(split.left);
  for (int i = 0; i < 5; i++) {
    ASSERT_EQ(left_data[i], i + 1);
  }

  slp_split_buffer_free(&split);
  slp_buffer_free(buffer);
}

void test_split_middle_large_buffer(void) {
  slp_buffer_t *buffer = slp_buffer_new(100);
  uint8_t data[100];
  for (int i = 0; i < 100; i++) {
    data[i] = i % 256;
  }
  slp_buffer_copy_to(buffer, data, 100);

  split_buffer_t split = slp_buffer_split(buffer, 50, 60, 60);

  ASSERT_NOT_NULL(split.left);
  ASSERT_NOT_NULL(split.right);

  ASSERT_EQ(slp_buffer_count(split.left), 50);
  ASSERT_EQ(slp_buffer_count(split.right), 50);

  uint8_t *left_data = slp_buffer_data(split.left);
  for (int i = 0; i < 50; i++) {
    ASSERT_EQ(left_data[i], i % 256);
  }

  uint8_t *right_data = slp_buffer_data(split.right);
  for (int i = 0; i < 50; i++) {
    ASSERT_EQ(right_data[i], (50 + i) % 256);
  }

  slp_split_buffer_free(&split);
  slp_buffer_free(buffer);
}

void test_split_with_small_l_capacity(void) {
  slp_buffer_t *buffer = slp_buffer_new(32);
  uint8_t data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  slp_buffer_copy_to(buffer, data, 10);

  split_buffer_t split = slp_buffer_split(buffer, 5, 3, 20);

  ASSERT_NOT_NULL(split.left);
  ASSERT_NOT_NULL(split.right);

  ASSERT_EQ(slp_buffer_count(split.left), 5);
  ASSERT_EQ(split.left->capacity, 16);

  slp_split_buffer_free(&split);
  slp_buffer_free(buffer);
}

void test_split_with_small_r_capacity(void) {
  slp_buffer_t *buffer = slp_buffer_new(32);
  uint8_t data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  slp_buffer_copy_to(buffer, data, 10);

  split_buffer_t split = slp_buffer_split(buffer, 5, 20, 3);

  ASSERT_NOT_NULL(split.left);
  ASSERT_NOT_NULL(split.right);

  ASSERT_EQ(slp_buffer_count(split.right), 5);
  ASSERT_EQ(split.right->capacity, 16);

  slp_split_buffer_free(&split);
  slp_buffer_free(buffer);
}

void test_split_single_element_buffer(void) {
  slp_buffer_t *buffer = slp_buffer_new(32);
  uint8_t data[] = {42};
  slp_buffer_copy_to(buffer, data, 1);

  split_buffer_t split = slp_buffer_split(buffer, 1, 32, 32);

  ASSERT_NOT_NULL(split.left);
  ASSERT_NOT_NULL(split.right);

  ASSERT_EQ(slp_buffer_count(split.left), 1);
  ASSERT_EQ(slp_buffer_count(split.right), 0);

  uint8_t *left_data = slp_buffer_data(split.left);
  ASSERT_EQ(left_data[0], 42);

  slp_split_buffer_free(&split);
  slp_buffer_free(buffer);
}

void test_split_destroy_null(void) { slp_split_buffer_free(NULL); }

void test_split_destroy_partial(void) {
  split_buffer_t split;
  split.left = slp_buffer_new(32);
  split.right = NULL;

  slp_split_buffer_free(&split);

  ASSERT_NULL(split.left);
  ASSERT_NULL(split.right);
}

void test_split_sequential_operations(void) {
  slp_buffer_t *buffer = slp_buffer_new(50);
  uint8_t data[30];
  for (int i = 0; i < 30; i++) {
    data[i] = i + 100;
  }
  slp_buffer_copy_to(buffer, data, 30);

  split_buffer_t split1 = slp_buffer_split(buffer, 10, 50, 50);
  ASSERT_NOT_NULL(split1.left);
  ASSERT_NOT_NULL(split1.right);

  split_buffer_t split2 = slp_buffer_split(split1.right, 10, 50, 50);
  ASSERT_NOT_NULL(split2.left);
  ASSERT_NOT_NULL(split2.right);

  ASSERT_EQ(slp_buffer_count(split1.left), 10);
  ASSERT_EQ(slp_buffer_count(split2.left), 10);
  ASSERT_EQ(slp_buffer_count(split2.right), 10);

  uint8_t *data1 = slp_buffer_data(split1.left);
  for (int i = 0; i < 10; i++) {
    ASSERT_EQ(data1[i], 100 + i);
  }

  uint8_t *data2 = slp_buffer_data(split2.left);
  for (int i = 0; i < 10; i++) {
    ASSERT_EQ(data2[i], 110 + i);
  }

  uint8_t *data3 = slp_buffer_data(split2.right);
  for (int i = 0; i < 10; i++) {
    ASSERT_EQ(data3[i], 120 + i);
  }

  slp_split_buffer_free(&split2);
  slp_split_buffer_free(&split1);
  slp_buffer_free(buffer);
}

int main(void) {
  test_split_out_of_bounds_index();
  test_split_null_buffer();
  test_split_empty_buffer();
  test_split_l_and_r_size_validation();
  test_split_value_validation_basic();
  test_split_index_exclusive();
  test_split_left_under_min_buffer_size();
  test_split_right_under_min_buffer_size();
  test_split_both_under_min_buffer_size();
  test_split_at_start();
  test_split_at_end();
  test_split_middle_large_buffer();
  test_split_with_small_l_capacity();
  test_split_with_small_r_capacity();
  test_split_single_element_buffer();
  test_split_destroy_null();
  test_split_destroy_partial();
  test_split_sequential_operations();

  return 0;
}
