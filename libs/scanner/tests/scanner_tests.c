#include "../scanner.h"
#include "test.h"
#include <stdlib.h>
#include <string.h>

static void test_scanner_new_valid_position() {
  slp_buffer_t *buffer = slp_buffer_create(64);
  ASSERT(buffer != NULL);

  uint8_t data[] = "hello world";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  ASSERT(scanner != NULL);
  ASSERT(scanner->buffer == buffer);
  ASSERT(scanner->position == 0);

  slp_scanner_free(scanner);
  slp_buffer_destroy(buffer);
}

static void test_scanner_new_mid_position() {
  slp_buffer_t *buffer = slp_buffer_create(64);
  ASSERT(buffer != NULL);

  uint8_t data[] = "hello world";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 6);
  ASSERT(scanner != NULL);
  ASSERT(scanner->buffer == buffer);
  ASSERT(scanner->position == 6);

  slp_scanner_free(scanner);
  slp_buffer_destroy(buffer);
}

static void test_scanner_new_end_position() {
  slp_buffer_t *buffer = slp_buffer_create(64);
  ASSERT(buffer != NULL);

  uint8_t data[] = "hello world";
  size_t len = strlen((char *)data);
  slp_buffer_copy_to(buffer, data, len);

  slp_scanner_t *scanner = slp_scanner_new(buffer, len);
  ASSERT(scanner != NULL);
  ASSERT(scanner->buffer == buffer);
  ASSERT(scanner->position == len);

  slp_scanner_free(scanner);
  slp_buffer_destroy(buffer);
}

static void test_scanner_new_invalid_position() {
  slp_buffer_t *buffer = slp_buffer_create(64);
  ASSERT(buffer != NULL);

  uint8_t data[] = "hello world";
  size_t len = strlen((char *)data);
  slp_buffer_copy_to(buffer, data, len);

  slp_scanner_t *scanner = slp_scanner_new(buffer, len + 1);
  ASSERT(scanner == NULL);

  slp_buffer_destroy(buffer);
}

static void test_scanner_new_null_buffer() {
  slp_scanner_t *scanner = slp_scanner_new(NULL, 0);
  ASSERT(scanner == NULL);
}

static void test_scanner_new_empty_buffer() {
  slp_buffer_t *buffer = slp_buffer_create(64);
  ASSERT(buffer != NULL);

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  ASSERT(scanner != NULL);
  ASSERT(scanner->buffer == buffer);
  ASSERT(scanner->position == 0);

  slp_scanner_free(scanner);
  slp_buffer_destroy(buffer);
}

static void test_scanner_free_null() { slp_scanner_free(NULL); }

static void test_scanner_does_not_own_buffer() {
  slp_buffer_t *buffer = slp_buffer_create(64);
  ASSERT(buffer != NULL);

  uint8_t data[] = "test data";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  ASSERT(scanner != NULL);

  slp_scanner_free(scanner);

  ASSERT(buffer->data != NULL);
  ASSERT(buffer->count == strlen((char *)data));

  slp_buffer_destroy(buffer);
}

int main(void) {
  test_scanner_new_valid_position();
  test_scanner_new_mid_position();
  test_scanner_new_end_position();
  test_scanner_new_invalid_position();
  test_scanner_new_null_buffer();
  test_scanner_new_empty_buffer();
  test_scanner_free_null();
  test_scanner_does_not_own_buffer();

  return 0;
}
