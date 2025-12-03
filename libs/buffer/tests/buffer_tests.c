#include "../buffer.h"
#include "test.h"
#include <string.h>

static int increment_byte(uint8_t *byte, size_t idx) {
  (*byte)++;
  return 1;
}

static int stop_at_5(uint8_t *byte, size_t idx) {
  if (idx >= 5) {
    return 0;
  }
  (*byte) *= 2;
  return 1;
}

static int skip_every_other(uint8_t *byte, size_t idx) {
  (*byte) += 10;
  return 2;
}

static int count_callback_calls = 0;
static int counting_callback(uint8_t *byte, size_t idx) {
  count_callback_calls++;
  return 1;
}

void test_buffer_create_destroy(void) {
  slp_buffer_t *buffer = slp_buffer_create(100);
  ASSERT_NOT_NULL(buffer);
  ASSERT_NOT_NULL(buffer->data);
  ASSERT_EQ(buffer->capacity, 100);
  ASSERT_EQ(buffer->count, 0);
  slp_buffer_destroy(buffer);
}

void test_buffer_create_min_size(void) {
  slp_buffer_t *buffer = slp_buffer_create(1);
  ASSERT_NOT_NULL(buffer);
  ASSERT_EQ(buffer->capacity, 16);
  slp_buffer_destroy(buffer);
}

void test_buffer_copy_to_basic(void) {
  slp_buffer_t *buffer = slp_buffer_create(32);
  uint8_t data[] = {1, 2, 3, 4, 5};

  int result = slp_buffer_copy_to(buffer, data, 5);
  ASSERT_EQ(result, 0);
  ASSERT_EQ(slp_buffer_count(buffer), 5);

  uint8_t *buf_data = slp_buffer_data(buffer);
  ASSERT_NOT_NULL(buf_data);
  ASSERT_EQ(buf_data[0], 1);
  ASSERT_EQ(buf_data[1], 2);
  ASSERT_EQ(buf_data[2], 3);
  ASSERT_EQ(buf_data[3], 4);
  ASSERT_EQ(buf_data[4], 5);

  slp_buffer_destroy(buffer);
}

void test_buffer_copy_to_multiple(void) {
  slp_buffer_t *buffer = slp_buffer_create(32);
  uint8_t data1[] = {1, 2, 3};
  uint8_t data2[] = {4, 5, 6};
  uint8_t data3[] = {7, 8, 9};

  slp_buffer_copy_to(buffer, data1, 3);
  slp_buffer_copy_to(buffer, data2, 3);
  slp_buffer_copy_to(buffer, data3, 3);

  ASSERT_EQ(slp_buffer_count(buffer), 9);

  uint8_t *buf_data = slp_buffer_data(buffer);
  for (int i = 0; i < 9; i++) {
    ASSERT_EQ(buf_data[i], i + 1);
  }

  slp_buffer_destroy(buffer);
}

void test_buffer_reallocation(void) {
  slp_buffer_t *buffer = slp_buffer_create(8);
  ASSERT_EQ(buffer->capacity, 16);

  uint8_t data[40];
  for (int i = 0; i < 40; i++) {
    data[i] = i;
  }

  int result = slp_buffer_copy_to(buffer, data, 40);
  ASSERT_EQ(result, 0);
  ASSERT_EQ(slp_buffer_count(buffer), 40);
  ASSERT_TRUE(buffer->capacity >= 40);

  uint8_t *buf_data = slp_buffer_data(buffer);
  for (int i = 0; i < 40; i++) {
    ASSERT_EQ(buf_data[i], i);
  }

  slp_buffer_destroy(buffer);
}

void test_buffer_clear(void) {
  slp_buffer_t *buffer = slp_buffer_create(32);
  uint8_t data[] = {1, 2, 3, 4, 5};

  slp_buffer_copy_to(buffer, data, 5);
  ASSERT_EQ(slp_buffer_count(buffer), 5);

  slp_buffer_clear(buffer);
  ASSERT_EQ(slp_buffer_count(buffer), 0);

  slp_buffer_copy_to(buffer, data, 3);
  ASSERT_EQ(slp_buffer_count(buffer), 3);

  slp_buffer_destroy(buffer);
}

void test_buffer_for_each_increment(void) {
  slp_buffer_t *buffer = slp_buffer_create(32);
  uint8_t data[] = {0, 1, 2, 3, 4};

  slp_buffer_copy_to(buffer, data, 5);
  slp_buffer_for_each(buffer, increment_byte);

  uint8_t *buf_data = slp_buffer_data(buffer);
  ASSERT_EQ(buf_data[0], 1);
  ASSERT_EQ(buf_data[1], 2);
  ASSERT_EQ(buf_data[2], 3);
  ASSERT_EQ(buf_data[3], 4);
  ASSERT_EQ(buf_data[4], 5);

  slp_buffer_destroy(buffer);
}

void test_buffer_for_each_stop(void) {
  slp_buffer_t *buffer = slp_buffer_create(32);
  uint8_t data[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

  slp_buffer_copy_to(buffer, data, 10);
  slp_buffer_for_each(buffer, stop_at_5);

  uint8_t *buf_data = slp_buffer_data(buffer);
  ASSERT_EQ(buf_data[0], 2);
  ASSERT_EQ(buf_data[1], 2);
  ASSERT_EQ(buf_data[2], 2);
  ASSERT_EQ(buf_data[3], 2);
  ASSERT_EQ(buf_data[4], 2);
  ASSERT_EQ(buf_data[5], 1);
  ASSERT_EQ(buf_data[6], 1);
  ASSERT_EQ(buf_data[7], 1);
  ASSERT_EQ(buf_data[8], 1);
  ASSERT_EQ(buf_data[9], 1);

  slp_buffer_destroy(buffer);
}

void test_buffer_for_each_skip(void) {
  slp_buffer_t *buffer = slp_buffer_create(32);
  uint8_t data[] = {0, 0, 0, 0, 0, 0, 0, 0};

  slp_buffer_copy_to(buffer, data, 8);
  slp_buffer_for_each(buffer, skip_every_other);

  uint8_t *buf_data = slp_buffer_data(buffer);
  ASSERT_EQ(buf_data[0], 10);
  ASSERT_EQ(buf_data[1], 0);
  ASSERT_EQ(buf_data[2], 10);
  ASSERT_EQ(buf_data[3], 0);
  ASSERT_EQ(buf_data[4], 10);
  ASSERT_EQ(buf_data[5], 0);
  ASSERT_EQ(buf_data[6], 10);
  ASSERT_EQ(buf_data[7], 0);

  slp_buffer_destroy(buffer);
}

void test_buffer_empty(void) {
  slp_buffer_t *buffer = slp_buffer_create(32);

  ASSERT_EQ(slp_buffer_count(buffer), 0);

  count_callback_calls = 0;
  slp_buffer_for_each(buffer, counting_callback);
  ASSERT_EQ(count_callback_calls, 0);

  slp_buffer_destroy(buffer);
}

void test_buffer_null_checks(void) {
  slp_buffer_destroy(NULL);

  ASSERT_EQ(slp_buffer_count(NULL), 0);
  ASSERT_NULL(slp_buffer_data(NULL));

  slp_buffer_clear(NULL);

  slp_buffer_t *buffer = slp_buffer_create(32);
  ASSERT_EQ(slp_buffer_copy_to(buffer, NULL, 10), -1);
  ASSERT_EQ(slp_buffer_copy_to(NULL, (uint8_t *)"test", 4), -1);

  slp_buffer_for_each(NULL, increment_byte);
  slp_buffer_for_each(buffer, NULL);

  slp_buffer_destroy(buffer);
}

void test_buffer_large_data(void) {
  slp_buffer_t *buffer = slp_buffer_create(16);

  uint8_t large_data[1000];
  for (int i = 0; i < 1000; i++) {
    large_data[i] = i % 256;
  }

  int result = slp_buffer_copy_to(buffer, large_data, 1000);
  ASSERT_EQ(result, 0);
  ASSERT_EQ(slp_buffer_count(buffer), 1000);

  uint8_t *buf_data = slp_buffer_data(buffer);
  for (int i = 0; i < 1000; i++) {
    ASSERT_EQ(buf_data[i], i % 256);
  }

  slp_buffer_destroy(buffer);
}

void test_buffer_zero_length_copy(void) {
  slp_buffer_t *buffer = slp_buffer_create(32);
  uint8_t data[] = {1, 2, 3};

  int result = slp_buffer_copy_to(buffer, data, 0);
  ASSERT_EQ(result, 0);
  ASSERT_EQ(slp_buffer_count(buffer), 0);

  slp_buffer_destroy(buffer);
}

void test_buffer_shrink_to_fit(void) {
  slp_buffer_t *buffer = slp_buffer_create(100);
  ASSERT_EQ(buffer->capacity, 100);

  uint8_t data[] = {1, 2, 3, 4, 5};
  slp_buffer_copy_to(buffer, data, 5);
  ASSERT_EQ(slp_buffer_count(buffer), 5);
  ASSERT_EQ(buffer->capacity, 100);

  int result = slp_buffer_shrink_to_fit(buffer);
  ASSERT_EQ(result, 0);
  ASSERT_EQ(buffer->capacity, 5);
  ASSERT_EQ(slp_buffer_count(buffer), 5);

  uint8_t *buf_data = slp_buffer_data(buffer);
  for (int i = 0; i < 5; i++) {
    ASSERT_EQ(buf_data[i], i + 1);
  }

  slp_buffer_destroy(buffer);
}

void test_buffer_shrink_to_fit_empty(void) {
  slp_buffer_t *buffer = slp_buffer_create(100);

  int result = slp_buffer_shrink_to_fit(buffer);
  ASSERT_EQ(result, 0);
  ASSERT_EQ(buffer->capacity, 100);

  slp_buffer_destroy(buffer);
}

void test_buffer_shrink_to_fit_already_fit(void) {
  slp_buffer_t *buffer = slp_buffer_create(32);
  uint8_t data[32];
  for (int i = 0; i < 32; i++) {
    data[i] = i;
  }

  slp_buffer_copy_to(buffer, data, 32);
  ASSERT_EQ(buffer->capacity, 32);
  ASSERT_EQ(slp_buffer_count(buffer), 32);

  int result = slp_buffer_shrink_to_fit(buffer);
  ASSERT_EQ(result, 0);
  ASSERT_EQ(buffer->capacity, 32);

  slp_buffer_destroy(buffer);
}

void test_buffer_shrink_to_fit_null(void) {
  int result = slp_buffer_shrink_to_fit(NULL);
  ASSERT_EQ(result, -1);
}

void test_buffer_sub_buffer_validation(void) {
  slp_buffer_t *buffer = slp_buffer_create(100);
  uint8_t data[100];
  for (int i = 0; i < 100; i++) {
    data[i] = i;
  }
  slp_buffer_copy_to(buffer, data, 100);

  int bytes_copied = 0;
  slp_buffer_t *sub = slp_buffer_sub_buffer(buffer, 20, 30, &bytes_copied);

  ASSERT_NOT_NULL(sub);
  ASSERT_EQ(bytes_copied, 30);
  ASSERT_EQ(slp_buffer_count(sub), 30);

  uint8_t *sub_data = slp_buffer_data(sub);
  for (int i = 0; i < 30; i++) {
    ASSERT_EQ(sub_data[i], 20 + i);
  }

  slp_buffer_destroy(sub);
  slp_buffer_destroy(buffer);
}

int main(void) {
  test_buffer_create_destroy();
  test_buffer_create_min_size();
  test_buffer_copy_to_basic();
  test_buffer_copy_to_multiple();
  test_buffer_reallocation();
  test_buffer_clear();
  test_buffer_for_each_increment();
  test_buffer_for_each_stop();
  test_buffer_for_each_skip();
  test_buffer_empty();
  test_buffer_null_checks();
  test_buffer_large_data();
  test_buffer_zero_length_copy();
  test_buffer_shrink_to_fit();
  test_buffer_shrink_to_fit_empty();
  test_buffer_shrink_to_fit_already_fit();
  test_buffer_shrink_to_fit_null();
  test_buffer_sub_buffer_validation();

  return 0;
}
