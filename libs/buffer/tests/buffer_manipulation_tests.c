#include "test.h"
#include "../buffer.h"
#include <string.h>

void test_rotate_left_basic(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    uint8_t data[] = {1, 2, 3, 4, 5};
    slp_buffer_copy_to(buffer, data, 5);
    
    slp_buffer_rotate_left(buffer, 2);
    
    uint8_t *buf_data = slp_buffer_data(buffer);
    ASSERT_EQ(buf_data[0], 3);
    ASSERT_EQ(buf_data[1], 4);
    ASSERT_EQ(buf_data[2], 5);
    ASSERT_EQ(buf_data[3], 1);
    ASSERT_EQ(buf_data[4], 2);
    
    slp_buffer_destroy(buffer);
}

void test_rotate_left_zero(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    uint8_t data[] = {10, 20, 30, 40};
    slp_buffer_copy_to(buffer, data, 4);
    
    slp_buffer_rotate_left(buffer, 0);
    
    uint8_t *buf_data = slp_buffer_data(buffer);
    ASSERT_EQ(buf_data[0], 10);
    ASSERT_EQ(buf_data[1], 20);
    ASSERT_EQ(buf_data[2], 30);
    ASSERT_EQ(buf_data[3], 40);
    
    slp_buffer_destroy(buffer);
}

void test_rotate_left_exact_count(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    uint8_t data[] = {1, 2, 3, 4, 5};
    slp_buffer_copy_to(buffer, data, 5);
    
    slp_buffer_rotate_left(buffer, 5);
    
    uint8_t *buf_data = slp_buffer_data(buffer);
    ASSERT_EQ(buf_data[0], 1);
    ASSERT_EQ(buf_data[1], 2);
    ASSERT_EQ(buf_data[2], 3);
    ASSERT_EQ(buf_data[3], 4);
    ASSERT_EQ(buf_data[4], 5);
    
    slp_buffer_destroy(buffer);
}

void test_rotate_left_wrapping(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    uint8_t data[] = {1, 2, 3, 4, 5};
    slp_buffer_copy_to(buffer, data, 5);
    
    slp_buffer_rotate_left(buffer, 7);
    
    uint8_t *buf_data = slp_buffer_data(buffer);
    ASSERT_EQ(buf_data[0], 3);
    ASSERT_EQ(buf_data[1], 4);
    ASSERT_EQ(buf_data[2], 5);
    ASSERT_EQ(buf_data[3], 1);
    ASSERT_EQ(buf_data[4], 2);
    
    slp_buffer_destroy(buffer);
}

void test_rotate_left_single_element(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    uint8_t data[] = {42};
    slp_buffer_copy_to(buffer, data, 1);
    
    slp_buffer_rotate_left(buffer, 10);
    
    uint8_t *buf_data = slp_buffer_data(buffer);
    ASSERT_EQ(buf_data[0], 42);
    
    slp_buffer_destroy(buffer);
}

void test_rotate_left_empty(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    
    slp_buffer_rotate_left(buffer, 5);
    
    ASSERT_EQ(slp_buffer_count(buffer), 0);
    
    slp_buffer_destroy(buffer);
}

void test_rotate_left_null(void) {
    slp_buffer_rotate_left(NULL, 5);
}

void test_rotate_right_basic(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    uint8_t data[] = {1, 2, 3, 4, 5};
    slp_buffer_copy_to(buffer, data, 5);
    
    slp_buffer_rotate_right(buffer, 2);
    
    uint8_t *buf_data = slp_buffer_data(buffer);
    ASSERT_EQ(buf_data[0], 4);
    ASSERT_EQ(buf_data[1], 5);
    ASSERT_EQ(buf_data[2], 1);
    ASSERT_EQ(buf_data[3], 2);
    ASSERT_EQ(buf_data[4], 3);
    
    slp_buffer_destroy(buffer);
}

void test_rotate_right_zero(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    uint8_t data[] = {10, 20, 30, 40};
    slp_buffer_copy_to(buffer, data, 4);
    
    slp_buffer_rotate_right(buffer, 0);
    
    uint8_t *buf_data = slp_buffer_data(buffer);
    ASSERT_EQ(buf_data[0], 10);
    ASSERT_EQ(buf_data[1], 20);
    ASSERT_EQ(buf_data[2], 30);
    ASSERT_EQ(buf_data[3], 40);
    
    slp_buffer_destroy(buffer);
}

void test_rotate_right_exact_count(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    uint8_t data[] = {1, 2, 3, 4, 5};
    slp_buffer_copy_to(buffer, data, 5);
    
    slp_buffer_rotate_right(buffer, 5);
    
    uint8_t *buf_data = slp_buffer_data(buffer);
    ASSERT_EQ(buf_data[0], 1);
    ASSERT_EQ(buf_data[1], 2);
    ASSERT_EQ(buf_data[2], 3);
    ASSERT_EQ(buf_data[3], 4);
    ASSERT_EQ(buf_data[4], 5);
    
    slp_buffer_destroy(buffer);
}

void test_rotate_right_wrapping(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    uint8_t data[] = {1, 2, 3, 4, 5};
    slp_buffer_copy_to(buffer, data, 5);
    
    slp_buffer_rotate_right(buffer, 7);
    
    uint8_t *buf_data = slp_buffer_data(buffer);
    ASSERT_EQ(buf_data[0], 4);
    ASSERT_EQ(buf_data[1], 5);
    ASSERT_EQ(buf_data[2], 1);
    ASSERT_EQ(buf_data[3], 2);
    ASSERT_EQ(buf_data[4], 3);
    
    slp_buffer_destroy(buffer);
}

void test_rotate_right_single_element(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    uint8_t data[] = {42};
    slp_buffer_copy_to(buffer, data, 1);
    
    slp_buffer_rotate_right(buffer, 10);
    
    uint8_t *buf_data = slp_buffer_data(buffer);
    ASSERT_EQ(buf_data[0], 42);
    
    slp_buffer_destroy(buffer);
}

void test_rotate_right_empty(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    
    slp_buffer_rotate_right(buffer, 5);
    
    ASSERT_EQ(slp_buffer_count(buffer), 0);
    
    slp_buffer_destroy(buffer);
}

void test_rotate_right_null(void) {
    slp_buffer_rotate_right(NULL, 5);
}

void test_trim_left_basic(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    uint8_t data[] = {0, 0, 0, 5, 6, 0};
    slp_buffer_copy_to(buffer, data, 6);
    
    int result = slp_buffer_trim_left(buffer, 0);
    
    ASSERT_EQ(result, 0);
    ASSERT_EQ(slp_buffer_count(buffer), 3);
    
    uint8_t *buf_data = slp_buffer_data(buffer);
    ASSERT_EQ(buf_data[0], 5);
    ASSERT_EQ(buf_data[1], 6);
    ASSERT_EQ(buf_data[2], 0);
    
    slp_buffer_destroy(buffer);
}

void test_trim_left_no_match(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    uint8_t data[] = {1, 2, 3, 4, 5};
    slp_buffer_copy_to(buffer, data, 5);
    
    int result = slp_buffer_trim_left(buffer, 0);
    
    ASSERT_EQ(result, 0);
    ASSERT_EQ(slp_buffer_count(buffer), 5);
    
    uint8_t *buf_data = slp_buffer_data(buffer);
    ASSERT_EQ(buf_data[0], 1);
    ASSERT_EQ(buf_data[1], 2);
    ASSERT_EQ(buf_data[2], 3);
    ASSERT_EQ(buf_data[3], 4);
    ASSERT_EQ(buf_data[4], 5);
    
    slp_buffer_destroy(buffer);
}

void test_trim_left_all_match(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    uint8_t data[] = {7, 7, 7, 7, 7};
    slp_buffer_copy_to(buffer, data, 5);
    
    int result = slp_buffer_trim_left(buffer, 7);
    
    ASSERT_EQ(result, 0);
    ASSERT_EQ(slp_buffer_count(buffer), 0);
    
    slp_buffer_destroy(buffer);
}

void test_trim_left_partial_match(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    uint8_t data[] = {255, 255, 100, 255, 255};
    slp_buffer_copy_to(buffer, data, 5);
    
    int result = slp_buffer_trim_left(buffer, 255);
    
    ASSERT_EQ(result, 0);
    ASSERT_EQ(slp_buffer_count(buffer), 3);
    
    uint8_t *buf_data = slp_buffer_data(buffer);
    ASSERT_EQ(buf_data[0], 100);
    ASSERT_EQ(buf_data[1], 255);
    ASSERT_EQ(buf_data[2], 255);
    
    slp_buffer_destroy(buffer);
}

void test_trim_left_empty(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    
    int result = slp_buffer_trim_left(buffer, 0);
    
    ASSERT_EQ(result, 0);
    ASSERT_EQ(slp_buffer_count(buffer), 0);
    
    slp_buffer_destroy(buffer);
}

void test_trim_left_null(void) {
    int result = slp_buffer_trim_left(NULL, 0);
    ASSERT_EQ(result, -1);
}

void test_trim_left_single_byte_match(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    uint8_t data[] = {9};
    slp_buffer_copy_to(buffer, data, 1);
    
    int result = slp_buffer_trim_left(buffer, 9);
    
    ASSERT_EQ(result, 0);
    ASSERT_EQ(slp_buffer_count(buffer), 0);
    
    slp_buffer_destroy(buffer);
}

void test_trim_left_single_byte_no_match(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    uint8_t data[] = {9};
    slp_buffer_copy_to(buffer, data, 1);
    
    int result = slp_buffer_trim_left(buffer, 8);
    
    ASSERT_EQ(result, 0);
    ASSERT_EQ(slp_buffer_count(buffer), 1);
    
    uint8_t *buf_data = slp_buffer_data(buffer);
    ASSERT_EQ(buf_data[0], 9);
    
    slp_buffer_destroy(buffer);
}

void test_trim_right_basic(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    uint8_t data[] = {0, 5, 6, 0, 0, 0};
    slp_buffer_copy_to(buffer, data, 6);
    
    int result = slp_buffer_trim_right(buffer, 0);
    
    ASSERT_EQ(result, 0);
    ASSERT_EQ(slp_buffer_count(buffer), 3);
    
    uint8_t *buf_data = slp_buffer_data(buffer);
    ASSERT_EQ(buf_data[0], 0);
    ASSERT_EQ(buf_data[1], 5);
    ASSERT_EQ(buf_data[2], 6);
    
    slp_buffer_destroy(buffer);
}

void test_trim_right_no_match(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    uint8_t data[] = {1, 2, 3, 4, 5};
    slp_buffer_copy_to(buffer, data, 5);
    
    int result = slp_buffer_trim_right(buffer, 0);
    
    ASSERT_EQ(result, 0);
    ASSERT_EQ(slp_buffer_count(buffer), 5);
    
    uint8_t *buf_data = slp_buffer_data(buffer);
    ASSERT_EQ(buf_data[0], 1);
    ASSERT_EQ(buf_data[1], 2);
    ASSERT_EQ(buf_data[2], 3);
    ASSERT_EQ(buf_data[3], 4);
    ASSERT_EQ(buf_data[4], 5);
    
    slp_buffer_destroy(buffer);
}

void test_trim_right_all_match(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    uint8_t data[] = {7, 7, 7, 7, 7};
    slp_buffer_copy_to(buffer, data, 5);
    
    int result = slp_buffer_trim_right(buffer, 7);
    
    ASSERT_EQ(result, 0);
    ASSERT_EQ(slp_buffer_count(buffer), 0);
    
    slp_buffer_destroy(buffer);
}

void test_trim_right_partial_match(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    uint8_t data[] = {255, 255, 100, 255, 255};
    slp_buffer_copy_to(buffer, data, 5);
    
    int result = slp_buffer_trim_right(buffer, 255);
    
    ASSERT_EQ(result, 0);
    ASSERT_EQ(slp_buffer_count(buffer), 3);
    
    uint8_t *buf_data = slp_buffer_data(buffer);
    ASSERT_EQ(buf_data[0], 255);
    ASSERT_EQ(buf_data[1], 255);
    ASSERT_EQ(buf_data[2], 100);
    
    slp_buffer_destroy(buffer);
}

void test_trim_right_empty(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    
    int result = slp_buffer_trim_right(buffer, 0);
    
    ASSERT_EQ(result, 0);
    ASSERT_EQ(slp_buffer_count(buffer), 0);
    
    slp_buffer_destroy(buffer);
}

void test_trim_right_null(void) {
    int result = slp_buffer_trim_right(NULL, 0);
    ASSERT_EQ(result, -1);
}

void test_trim_right_single_byte_match(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    uint8_t data[] = {9};
    slp_buffer_copy_to(buffer, data, 1);
    
    int result = slp_buffer_trim_right(buffer, 9);
    
    ASSERT_EQ(result, 0);
    ASSERT_EQ(slp_buffer_count(buffer), 0);
    
    slp_buffer_destroy(buffer);
}

void test_trim_right_single_byte_no_match(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    uint8_t data[] = {9};
    slp_buffer_copy_to(buffer, data, 1);
    
    int result = slp_buffer_trim_right(buffer, 8);
    
    ASSERT_EQ(result, 0);
    ASSERT_EQ(slp_buffer_count(buffer), 1);
    
    uint8_t *buf_data = slp_buffer_data(buffer);
    ASSERT_EQ(buf_data[0], 9);
    
    slp_buffer_destroy(buffer);
}

void test_copy_buffer_basic(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    uint8_t data[] = {1, 2, 3, 4, 5};
    slp_buffer_copy_to(buffer, data, 5);
    
    slp_buffer_t *copy = slp_buffer_copy(buffer);
    
    ASSERT_NOT_NULL(copy);
    ASSERT_EQ(slp_buffer_count(copy), 5);
    ASSERT_EQ(copy->capacity, buffer->capacity);
    
    uint8_t *copy_data = slp_buffer_data(copy);
    ASSERT_EQ(copy_data[0], 1);
    ASSERT_EQ(copy_data[1], 2);
    ASSERT_EQ(copy_data[2], 3);
    ASSERT_EQ(copy_data[3], 4);
    ASSERT_EQ(copy_data[4], 5);
    
    slp_buffer_destroy(copy);
    slp_buffer_destroy(buffer);
}

void test_copy_buffer_independence(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    uint8_t data[] = {10, 20, 30, 40, 50};
    slp_buffer_copy_to(buffer, data, 5);
    
    slp_buffer_t *copy = slp_buffer_copy(buffer);
    
    uint8_t *buf_data = slp_buffer_data(buffer);
    buf_data[0] = 99;
    buf_data[1] = 88;
    
    uint8_t *copy_data = slp_buffer_data(copy);
    ASSERT_EQ(copy_data[0], 10);
    ASSERT_EQ(copy_data[1], 20);
    ASSERT_EQ(copy_data[2], 30);
    ASSERT_EQ(copy_data[3], 40);
    ASSERT_EQ(copy_data[4], 50);
    
    slp_buffer_destroy(copy);
    slp_buffer_destroy(buffer);
}

void test_copy_buffer_empty(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    
    slp_buffer_t *copy = slp_buffer_copy(buffer);
    
    ASSERT_NOT_NULL(copy);
    ASSERT_EQ(slp_buffer_count(copy), 0);
    
    slp_buffer_destroy(copy);
    slp_buffer_destroy(buffer);
}

void test_copy_buffer_null(void) {
    slp_buffer_t *copy = slp_buffer_copy(NULL);
    ASSERT_NULL(copy);
}

void test_copy_buffer_large(void) {
    slp_buffer_t *buffer = slp_buffer_create(1000);
    uint8_t data[1000];
    for (int i = 0; i < 1000; i++) {
        data[i] = i % 256;
    }
    slp_buffer_copy_to(buffer, data, 1000);
    
    slp_buffer_t *copy = slp_buffer_copy(buffer);
    
    ASSERT_NOT_NULL(copy);
    ASSERT_EQ(slp_buffer_count(copy), 1000);
    
    uint8_t *copy_data = slp_buffer_data(copy);
    for (int i = 0; i < 1000; i++) {
        ASSERT_EQ(copy_data[i], i % 256);
    }
    
    slp_buffer_destroy(copy);
    slp_buffer_destroy(buffer);
}

void test_rotate_left_then_right(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    uint8_t data[] = {1, 2, 3, 4, 5, 6, 7, 8};
    slp_buffer_copy_to(buffer, data, 8);
    
    slp_buffer_rotate_left(buffer, 3);
    slp_buffer_rotate_right(buffer, 3);
    
    uint8_t *buf_data = slp_buffer_data(buffer);
    for (int i = 0; i < 8; i++) {
        ASSERT_EQ(buf_data[i], i + 1);
    }
    
    slp_buffer_destroy(buffer);
}

void test_trim_both_sides(void) {
    slp_buffer_t *buffer = slp_buffer_create(32);
    uint8_t data[] = {0, 0, 5, 6, 7, 0, 0};
    slp_buffer_copy_to(buffer, data, 7);
    
    slp_buffer_trim_left(buffer, 0);
    slp_buffer_trim_right(buffer, 0);
    
    ASSERT_EQ(slp_buffer_count(buffer), 3);
    
    uint8_t *buf_data = slp_buffer_data(buffer);
    ASSERT_EQ(buf_data[0], 5);
    ASSERT_EQ(buf_data[1], 6);
    ASSERT_EQ(buf_data[2], 7);
    
    slp_buffer_destroy(buffer);
}

int main(void) {
    test_rotate_left_basic();
    test_rotate_left_zero();
    test_rotate_left_exact_count();
    test_rotate_left_wrapping();
    test_rotate_left_single_element();
    test_rotate_left_empty();
    test_rotate_left_null();
    
    test_rotate_right_basic();
    test_rotate_right_zero();
    test_rotate_right_exact_count();
    test_rotate_right_wrapping();
    test_rotate_right_single_element();
    test_rotate_right_empty();
    test_rotate_right_null();
    
    test_trim_left_basic();
    test_trim_left_no_match();
    test_trim_left_all_match();
    test_trim_left_partial_match();
    test_trim_left_empty();
    test_trim_left_null();
    test_trim_left_single_byte_match();
    test_trim_left_single_byte_no_match();
    
    test_trim_right_basic();
    test_trim_right_no_match();
    test_trim_right_all_match();
    test_trim_right_partial_match();
    test_trim_right_empty();
    test_trim_right_null();
    test_trim_right_single_byte_match();
    test_trim_right_single_byte_no_match();
    
    test_copy_buffer_basic();
    test_copy_buffer_independence();
    test_copy_buffer_empty();
    test_copy_buffer_null();
    test_copy_buffer_large();
    
    test_rotate_left_then_right();
    test_trim_both_sides();
    
    return 0;
}


