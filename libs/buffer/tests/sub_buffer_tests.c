#include "test.h"
#include "../buffer.h"
#include <string.h>

void test_sub_buffer_basic_extraction(void) {
    slp_buffer_t *buffer = slp_buffer_create(50);
    uint8_t data[50];
    for (int i = 0; i < 50; i++) {
        data[i] = i;
    }
    slp_buffer_copy_to(buffer, data, 50);
    
    int bytes_copied = 0;
    slp_buffer_t *sub = slp_buffer_sub_buffer(buffer, 10, 20, &bytes_copied);
    
    ASSERT_NOT_NULL(sub);
    ASSERT_EQ(bytes_copied, 20);
    ASSERT_EQ(slp_buffer_count(sub), 20);
    
    uint8_t *sub_data = slp_buffer_data(sub);
    for (int i = 0; i < 20; i++) {
        ASSERT_EQ(sub_data[i], 10 + i);
    }
    
    slp_buffer_destroy(sub);
    slp_buffer_destroy(buffer);
}

void test_sub_buffer_multiple_ranges(void) {
    slp_buffer_t *buffer = slp_buffer_create(100);
    uint8_t data[100];
    for (int i = 0; i < 100; i++) {
        data[i] = i % 256;
    }
    slp_buffer_copy_to(buffer, data, 100);
    
    int bytes1 = 0, bytes2 = 0, bytes3 = 0, bytes4 = 0;
    slp_buffer_t *sub1 = slp_buffer_sub_buffer(buffer, 0, 10, &bytes1);
    slp_buffer_t *sub2 = slp_buffer_sub_buffer(buffer, 25, 15, &bytes2);
    slp_buffer_t *sub3 = slp_buffer_sub_buffer(buffer, 50, 20, &bytes3);
    slp_buffer_t *sub4 = slp_buffer_sub_buffer(buffer, 90, 10, &bytes4);
    
    ASSERT_NOT_NULL(sub1);
    ASSERT_NOT_NULL(sub2);
    ASSERT_NOT_NULL(sub3);
    ASSERT_NOT_NULL(sub4);
    
    ASSERT_EQ(bytes1, 10);
    ASSERT_EQ(bytes2, 15);
    ASSERT_EQ(bytes3, 20);
    ASSERT_EQ(bytes4, 10);
    
    uint8_t *data1 = slp_buffer_data(sub1);
    for (int i = 0; i < 10; i++) {
        ASSERT_EQ(data1[i], i);
    }
    
    uint8_t *data2 = slp_buffer_data(sub2);
    for (int i = 0; i < 15; i++) {
        ASSERT_EQ(data2[i], 25 + i);
    }
    
    uint8_t *data3 = slp_buffer_data(sub3);
    for (int i = 0; i < 20; i++) {
        ASSERT_EQ(data3[i], 50 + i);
    }
    
    uint8_t *data4 = slp_buffer_data(sub4);
    for (int i = 0; i < 10; i++) {
        ASSERT_EQ(data4[i], 90 + i);
    }
    
    slp_buffer_destroy(sub1);
    slp_buffer_destroy(sub2);
    slp_buffer_destroy(sub3);
    slp_buffer_destroy(sub4);
    slp_buffer_destroy(buffer);
}

void test_sub_buffer_offset_at_start(void) {
    slp_buffer_t *buffer = slp_buffer_create(30);
    uint8_t data[30];
    for (int i = 0; i < 30; i++) {
        data[i] = i + 100;
    }
    slp_buffer_copy_to(buffer, data, 30);
    
    int bytes_copied = 0;
    slp_buffer_t *sub = slp_buffer_sub_buffer(buffer, 0, 15, &bytes_copied);
    
    ASSERT_NOT_NULL(sub);
    ASSERT_EQ(bytes_copied, 15);
    ASSERT_EQ(slp_buffer_count(sub), 15);
    
    uint8_t *sub_data = slp_buffer_data(sub);
    for (int i = 0; i < 15; i++) {
        ASSERT_EQ(sub_data[i], i + 100);
    }
    
    slp_buffer_destroy(sub);
    slp_buffer_destroy(buffer);
}

void test_sub_buffer_offset_at_end(void) {
    slp_buffer_t *buffer = slp_buffer_create(50);
    uint8_t data[50];
    for (int i = 0; i < 50; i++) {
        data[i] = i * 2;
    }
    slp_buffer_copy_to(buffer, data, 50);
    
    int bytes_copied = 0;
    slp_buffer_t *sub = slp_buffer_sub_buffer(buffer, 45, 5, &bytes_copied);
    
    ASSERT_NOT_NULL(sub);
    ASSERT_EQ(bytes_copied, 5);
    ASSERT_EQ(slp_buffer_count(sub), 5);
    
    uint8_t *sub_data = slp_buffer_data(sub);
    for (int i = 0; i < 5; i++) {
        ASSERT_EQ(sub_data[i], (45 + i) * 2);
    }
    
    slp_buffer_destroy(sub);
    slp_buffer_destroy(buffer);
}

void test_sub_buffer_length_exceeds_available(void) {
    slp_buffer_t *buffer = slp_buffer_create(40);
    uint8_t data[40];
    for (int i = 0; i < 40; i++) {
        data[i] = 255 - i;
    }
    slp_buffer_copy_to(buffer, data, 40);
    
    int bytes_copied = 0;
    slp_buffer_t *sub = slp_buffer_sub_buffer(buffer, 30, 100, &bytes_copied);
    
    ASSERT_NOT_NULL(sub);
    ASSERT_EQ(bytes_copied, 10);
    ASSERT_EQ(slp_buffer_count(sub), 10);
    
    uint8_t *sub_data = slp_buffer_data(sub);
    for (int i = 0; i < 10; i++) {
        ASSERT_EQ(sub_data[i], 255 - (30 + i));
    }
    
    slp_buffer_destroy(sub);
    slp_buffer_destroy(buffer);
}

void test_sub_buffer_exact_boundary(void) {
    slp_buffer_t *buffer = slp_buffer_create(60);
    uint8_t data[60];
    for (int i = 0; i < 60; i++) {
        data[i] = i + 50;
    }
    slp_buffer_copy_to(buffer, data, 60);
    
    int bytes_copied = 0;
    slp_buffer_t *sub = slp_buffer_sub_buffer(buffer, 20, 40, &bytes_copied);
    
    ASSERT_NOT_NULL(sub);
    ASSERT_EQ(bytes_copied, 40);
    ASSERT_EQ(slp_buffer_count(sub), 40);
    
    uint8_t *sub_data = slp_buffer_data(sub);
    for (int i = 0; i < 40; i++) {
        ASSERT_EQ(sub_data[i], 20 + i + 50);
    }
    
    slp_buffer_destroy(sub);
    slp_buffer_destroy(buffer);
}

void test_sub_buffer_zero_length(void) {
    slp_buffer_t *buffer = slp_buffer_create(30);
    uint8_t data[30];
    for (int i = 0; i < 30; i++) {
        data[i] = i;
    }
    slp_buffer_copy_to(buffer, data, 30);
    
    int bytes_copied = 0;
    slp_buffer_t *sub = slp_buffer_sub_buffer(buffer, 10, 0, &bytes_copied);
    
    ASSERT_NOT_NULL(sub);
    ASSERT_EQ(bytes_copied, 0);
    ASSERT_EQ(slp_buffer_count(sub), 0);
    
    slp_buffer_destroy(sub);
    slp_buffer_destroy(buffer);
}

void test_sub_buffer_invalid_offset(void) {
    slp_buffer_t *buffer = slp_buffer_create(30);
    uint8_t data[30];
    for (int i = 0; i < 30; i++) {
        data[i] = i;
    }
    slp_buffer_copy_to(buffer, data, 30);
    
    int bytes_copied = 0;
    slp_buffer_t *sub = slp_buffer_sub_buffer(buffer, 50, 10, &bytes_copied);
    
    ASSERT_NULL(sub);
    ASSERT_EQ(bytes_copied, 0);
    
    slp_buffer_destroy(buffer);
}

void test_sub_buffer_bytes_copied_null(void) {
    slp_buffer_t *buffer = slp_buffer_create(30);
    uint8_t data[30];
    for (int i = 0; i < 30; i++) {
        data[i] = i + 10;
    }
    slp_buffer_copy_to(buffer, data, 30);
    
    slp_buffer_t *sub = slp_buffer_sub_buffer(buffer, 5, 10, NULL);
    
    ASSERT_NOT_NULL(sub);
    ASSERT_EQ(slp_buffer_count(sub), 10);
    
    uint8_t *sub_data = slp_buffer_data(sub);
    for (int i = 0; i < 10; i++) {
        ASSERT_EQ(sub_data[i], 5 + i + 10);
    }
    
    slp_buffer_destroy(sub);
    slp_buffer_destroy(buffer);
}

void test_sub_buffer_null_buffer(void) {
    int bytes_copied = 99;
    slp_buffer_t *sub = slp_buffer_sub_buffer(NULL, 0, 10, &bytes_copied);
    
    ASSERT_NULL(sub);
    ASSERT_EQ(bytes_copied, 0);
}

void test_sub_buffer_sequential_extractions(void) {
    slp_buffer_t *buffer = slp_buffer_create(80);
    uint8_t data[80];
    for (int i = 0; i < 80; i++) {
        data[i] = i % 256;
    }
    slp_buffer_copy_to(buffer, data, 80);
    
    int b1 = 0, b2 = 0, b3 = 0;
    slp_buffer_t *sub1 = slp_buffer_sub_buffer(buffer, 10, 20, &b1);
    slp_buffer_t *sub2 = slp_buffer_sub_buffer(buffer, 15, 25, &b2);
    slp_buffer_t *sub3 = slp_buffer_sub_buffer(buffer, 30, 10, &b3);
    
    ASSERT_NOT_NULL(sub1);
    ASSERT_NOT_NULL(sub2);
    ASSERT_NOT_NULL(sub3);
    
    ASSERT_EQ(b1, 20);
    ASSERT_EQ(b2, 25);
    ASSERT_EQ(b3, 10);
    
    uint8_t *data1 = slp_buffer_data(sub1);
    for (int i = 0; i < 20; i++) {
        ASSERT_EQ(data1[i], (10 + i) % 256);
    }
    
    uint8_t *data2 = slp_buffer_data(sub2);
    for (int i = 0; i < 25; i++) {
        ASSERT_EQ(data2[i], (15 + i) % 256);
    }
    
    uint8_t *data3 = slp_buffer_data(sub3);
    for (int i = 0; i < 10; i++) {
        ASSERT_EQ(data3[i], (30 + i) % 256);
    }
    
    slp_buffer_destroy(sub1);
    slp_buffer_destroy(sub2);
    slp_buffer_destroy(sub3);
    slp_buffer_destroy(buffer);
}

void test_sub_buffer_large_buffer_chunks(void) {
    slp_buffer_t *buffer = slp_buffer_create(1000);
    uint8_t data[1000];
    for (int i = 0; i < 1000; i++) {
        data[i] = i % 256;
    }
    slp_buffer_copy_to(buffer, data, 1000);
    
    slp_buffer_t *subs[10];
    int bytes[10];
    
    for (int chunk = 0; chunk < 10; chunk++) {
        subs[chunk] = slp_buffer_sub_buffer(buffer, chunk * 100, 100, &bytes[chunk]);
        ASSERT_NOT_NULL(subs[chunk]);
        ASSERT_EQ(bytes[chunk], 100);
        ASSERT_EQ(slp_buffer_count(subs[chunk]), 100);
    }
    
    for (int chunk = 0; chunk < 10; chunk++) {
        uint8_t *chunk_data = slp_buffer_data(subs[chunk]);
        for (int i = 0; i < 100; i++) {
            int expected_value = (chunk * 100 + i) % 256;
            ASSERT_EQ(chunk_data[i], expected_value);
        }
    }
    
    for (int chunk = 0; chunk < 10; chunk++) {
        slp_buffer_destroy(subs[chunk]);
    }
    slp_buffer_destroy(buffer);
}

void test_sub_buffer_full_copy(void) {
    slp_buffer_t *buffer = slp_buffer_create(25);
    uint8_t data[25];
    for (int i = 0; i < 25; i++) {
        data[i] = i * 3;
    }
    slp_buffer_copy_to(buffer, data, 25);
    
    int bytes_copied = 0;
    slp_buffer_t *sub = slp_buffer_sub_buffer(buffer, 0, 25, &bytes_copied);
    
    ASSERT_NOT_NULL(sub);
    ASSERT_EQ(bytes_copied, 25);
    ASSERT_EQ(slp_buffer_count(sub), 25);
    
    uint8_t *sub_data = slp_buffer_data(sub);
    for (int i = 0; i < 25; i++) {
        ASSERT_EQ(sub_data[i], i * 3);
    }
    
    slp_buffer_destroy(sub);
    slp_buffer_destroy(buffer);
}

void test_sub_buffer_single_byte(void) {
    slp_buffer_t *buffer = slp_buffer_create(20);
    uint8_t data[20];
    for (int i = 0; i < 20; i++) {
        data[i] = i + 200;
    }
    slp_buffer_copy_to(buffer, data, 20);
    
    int bytes_copied = 0;
    slp_buffer_t *sub = slp_buffer_sub_buffer(buffer, 10, 1, &bytes_copied);
    
    ASSERT_NOT_NULL(sub);
    ASSERT_EQ(bytes_copied, 1);
    ASSERT_EQ(slp_buffer_count(sub), 1);
    
    uint8_t *sub_data = slp_buffer_data(sub);
    ASSERT_EQ(sub_data[0], 210);
    
    slp_buffer_destroy(sub);
    slp_buffer_destroy(buffer);
}

int main(void) {
    test_sub_buffer_basic_extraction();
    test_sub_buffer_multiple_ranges();
    test_sub_buffer_offset_at_start();
    test_sub_buffer_offset_at_end();
    test_sub_buffer_length_exceeds_available();
    test_sub_buffer_exact_boundary();
    test_sub_buffer_zero_length();
    test_sub_buffer_invalid_offset();
    test_sub_buffer_bytes_copied_null();
    test_sub_buffer_null_buffer();
    test_sub_buffer_sequential_extractions();
    test_sub_buffer_large_buffer_chunks();
    test_sub_buffer_full_copy();
    test_sub_buffer_single_byte();
    
    return 0;
}

