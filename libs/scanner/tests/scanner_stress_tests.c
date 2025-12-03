#include "../scanner.h"
#include "test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_large_valid_buffer_mixed_types() {
  slp_buffer_t *buffer = slp_buffer_create(2048);
  ASSERT(buffer != NULL);

  uint8_t data[] = "alpha 42 beta -17 3.14 gamma +99 delta -2.5 epsilon 0 zeta "
                   "100 eta 0.001 theta -999 iota 42.42 kappa +1 lambda -1 "
                   "mu 3.14159 nu +0 xi -0 omicron 1.0 pi 2.0 rho 3.0 "
                   "sigma 4.0 tau 5.0 upsilon 6.0 phi 7.0 chi 8.0 psi 9.0 "
                   "omega 10.0 var1 11 var2 12 var3 13 var4 14 var5 15";

  slp_buffer_copy_to(buffer, data, strlen((char *)data));
  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  ASSERT(scanner != NULL);

  typedef struct {
    slp_static_base_e expected_type;
    const char *expected_value;
  } expected_token_t;

  expected_token_t expected[] = {
      {SLP_STATIC_BASE_SYMBOL, "alpha"}, {SLP_STATIC_BASE_INTEGER, "42"},
      {SLP_STATIC_BASE_SYMBOL, "beta"},  {SLP_STATIC_BASE_INTEGER, "-17"},
      {SLP_STATIC_BASE_REAL, "3.14"},    {SLP_STATIC_BASE_SYMBOL, "gamma"},
      {SLP_STATIC_BASE_INTEGER, "+99"},  {SLP_STATIC_BASE_SYMBOL, "delta"},
      {SLP_STATIC_BASE_REAL, "-2.5"},    {SLP_STATIC_BASE_SYMBOL, "epsilon"},
      {SLP_STATIC_BASE_INTEGER, "0"},    {SLP_STATIC_BASE_SYMBOL, "zeta"},
      {SLP_STATIC_BASE_INTEGER, "100"},  {SLP_STATIC_BASE_SYMBOL, "eta"},
      {SLP_STATIC_BASE_REAL, "0.001"},   {SLP_STATIC_BASE_SYMBOL, "theta"},
      {SLP_STATIC_BASE_INTEGER, "-999"}, {SLP_STATIC_BASE_SYMBOL, "iota"},
      {SLP_STATIC_BASE_REAL, "42.42"},   {SLP_STATIC_BASE_SYMBOL, "kappa"},
      {SLP_STATIC_BASE_INTEGER, "+1"},   {SLP_STATIC_BASE_SYMBOL, "lambda"},
      {SLP_STATIC_BASE_INTEGER, "-1"},   {SLP_STATIC_BASE_SYMBOL, "mu"},
      {SLP_STATIC_BASE_REAL, "3.14159"}, {SLP_STATIC_BASE_SYMBOL, "nu"},
      {SLP_STATIC_BASE_INTEGER, "+0"},   {SLP_STATIC_BASE_SYMBOL, "xi"},
      {SLP_STATIC_BASE_INTEGER, "-0"},   {SLP_STATIC_BASE_SYMBOL, "omicron"},
      {SLP_STATIC_BASE_REAL, "1.0"},     {SLP_STATIC_BASE_SYMBOL, "pi"},
      {SLP_STATIC_BASE_REAL, "2.0"},     {SLP_STATIC_BASE_SYMBOL, "rho"},
      {SLP_STATIC_BASE_REAL, "3.0"},     {SLP_STATIC_BASE_SYMBOL, "sigma"},
      {SLP_STATIC_BASE_REAL, "4.0"},     {SLP_STATIC_BASE_SYMBOL, "tau"},
      {SLP_STATIC_BASE_REAL, "5.0"},     {SLP_STATIC_BASE_SYMBOL, "upsilon"},
      {SLP_STATIC_BASE_REAL, "6.0"},     {SLP_STATIC_BASE_SYMBOL, "phi"},
      {SLP_STATIC_BASE_REAL, "7.0"},     {SLP_STATIC_BASE_SYMBOL, "chi"},
      {SLP_STATIC_BASE_REAL, "8.0"},     {SLP_STATIC_BASE_SYMBOL, "psi"},
      {SLP_STATIC_BASE_REAL, "9.0"},     {SLP_STATIC_BASE_SYMBOL, "omega"},
      {SLP_STATIC_BASE_REAL, "10.0"},    {SLP_STATIC_BASE_SYMBOL, "var1"},
      {SLP_STATIC_BASE_INTEGER, "11"},   {SLP_STATIC_BASE_SYMBOL, "var2"},
      {SLP_STATIC_BASE_INTEGER, "12"},   {SLP_STATIC_BASE_SYMBOL, "var3"},
      {SLP_STATIC_BASE_INTEGER, "13"},   {SLP_STATIC_BASE_SYMBOL, "var4"},
      {SLP_STATIC_BASE_INTEGER, "14"},   {SLP_STATIC_BASE_SYMBOL, "var5"},
      {SLP_STATIC_BASE_INTEGER, "15"}};

  size_t num_expected = sizeof(expected) / sizeof(expected[0]);

  for (size_t i = 0; i < num_expected; i++) {
    slp_scanner_static_type_result_t result =
        slp_scanner_read_static_base_type(scanner, NULL);

    ASSERT(result.success);
    ASSERT(result.data.base == expected[i].expected_type);

    size_t expected_len = strlen(expected[i].expected_value);
    ASSERT(result.data.byte_length == expected_len);
    ASSERT(memcmp(result.data.data, expected[i].expected_value, expected_len) ==
           0);
  }

  slp_scanner_free(scanner);
  slp_buffer_destroy(buffer);
}

static void test_large_buffer_with_whitespace_variations() {
  slp_buffer_t *buffer = slp_buffer_create(2048);
  ASSERT(buffer != NULL);

  uint8_t data[] = "  \t\n  a1   \t  42  \n\n  b2\t\t-17\n   3.14   \t\n"
                   "c3    +99     d4\t\t-2.5\n\ne5\t0\tf6\n100\tg7\r\n"
                   "0.001  \t  h8    -999\ni9\t\t42.42   j10\t+1\n\n"
                   "k11  -1  l12\t3.14159\tm13\n+0\tn14  -0  o15\t1.0";

  slp_buffer_copy_to(buffer, data, strlen((char *)data));
  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  ASSERT(scanner != NULL);

  typedef struct {
    slp_static_base_e expected_type;
    const char *expected_value;
  } expected_token_t;

  expected_token_t expected[] = {
      {SLP_STATIC_BASE_SYMBOL, "a1"},    {SLP_STATIC_BASE_INTEGER, "42"},
      {SLP_STATIC_BASE_SYMBOL, "b2"},    {SLP_STATIC_BASE_INTEGER, "-17"},
      {SLP_STATIC_BASE_REAL, "3.14"},    {SLP_STATIC_BASE_SYMBOL, "c3"},
      {SLP_STATIC_BASE_INTEGER, "+99"},  {SLP_STATIC_BASE_SYMBOL, "d4"},
      {SLP_STATIC_BASE_REAL, "-2.5"},    {SLP_STATIC_BASE_SYMBOL, "e5"},
      {SLP_STATIC_BASE_INTEGER, "0"},    {SLP_STATIC_BASE_SYMBOL, "f6"},
      {SLP_STATIC_BASE_INTEGER, "100"},  {SLP_STATIC_BASE_SYMBOL, "g7"},
      {SLP_STATIC_BASE_REAL, "0.001"},   {SLP_STATIC_BASE_SYMBOL, "h8"},
      {SLP_STATIC_BASE_INTEGER, "-999"}, {SLP_STATIC_BASE_SYMBOL, "i9"},
      {SLP_STATIC_BASE_REAL, "42.42"},   {SLP_STATIC_BASE_SYMBOL, "j10"},
      {SLP_STATIC_BASE_INTEGER, "+1"},   {SLP_STATIC_BASE_SYMBOL, "k11"},
      {SLP_STATIC_BASE_INTEGER, "-1"},   {SLP_STATIC_BASE_SYMBOL, "l12"},
      {SLP_STATIC_BASE_REAL, "3.14159"}, {SLP_STATIC_BASE_SYMBOL, "m13"},
      {SLP_STATIC_BASE_INTEGER, "+0"},   {SLP_STATIC_BASE_SYMBOL, "n14"},
      {SLP_STATIC_BASE_INTEGER, "-0"},   {SLP_STATIC_BASE_SYMBOL, "o15"},
      {SLP_STATIC_BASE_REAL, "1.0"}};

  size_t num_expected = sizeof(expected) / sizeof(expected[0]);

  for (size_t i = 0; i < num_expected; i++) {
    slp_scanner_static_type_result_t result =
        slp_scanner_read_static_base_type(scanner, NULL);

    ASSERT(result.success);
    ASSERT(result.data.base == expected[i].expected_type);

    size_t expected_len = strlen(expected[i].expected_value);
    ASSERT(result.data.byte_length == expected_len);
    ASSERT(memcmp(result.data.data, expected[i].expected_value, expected_len) ==
           0);
  }

  slp_scanner_free(scanner);
  slp_buffer_destroy(buffer);
}

static void test_large_buffer_error_at_start() {
  slp_buffer_t *buffer = slp_buffer_create(2048);
  ASSERT(buffer != NULL);

  uint8_t data[] = "123abc alpha 42 beta gamma";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  ASSERT(scanner != NULL);

  slp_scanner_static_type_result_t result =
      slp_scanner_read_static_base_type(scanner, NULL);

  ASSERT(!result.success);
  ASSERT(result.start_position == 0);
  ASSERT(result.error_position == 3);
  ASSERT(scanner->position == 0);

  slp_scanner_free(scanner);
  slp_buffer_destroy(buffer);
}

static void test_double_period_in_real() {
  slp_buffer_t *buffer = slp_buffer_create(64);
  ASSERT(buffer != NULL);

  uint8_t data[] = "1.2.3";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  ASSERT(scanner != NULL);

  slp_scanner_static_type_result_t result =
      slp_scanner_read_static_base_type(scanner, NULL);

  ASSERT(!result.success);
  ASSERT(result.start_position == 0);
  ASSERT(result.error_position == 3);
  ASSERT(scanner->position == 0);

  slp_scanner_free(scanner);
  slp_buffer_destroy(buffer);
}

static void test_large_buffer_error_in_middle() {
  slp_buffer_t *buffer = slp_buffer_create(2048);
  ASSERT(buffer != NULL);

  uint8_t data[] = "alpha 42 beta -17 3.14 gamma +99 delta -2.5 epsilon 0 zeta "
                   "100 eta 0.001 theta 123x iota 42.42 kappa +1 lambda";

  slp_buffer_copy_to(buffer, data, strlen((char *)data));
  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  ASSERT(scanner != NULL);

  for (int i = 0; i < 16; i++) {
    slp_scanner_static_type_result_t result =
        slp_scanner_read_static_base_type(scanner, NULL);
    ASSERT(result.success);
  }

  size_t pos_before_error = scanner->position;

  slp_scanner_static_type_result_t error_result =
      slp_scanner_read_static_base_type(scanner, NULL);

  ASSERT(!error_result.success);
  ASSERT(error_result.start_position == pos_before_error);
  ASSERT(error_result.error_position > pos_before_error);
  ASSERT(scanner->position == pos_before_error);

  slp_scanner_free(scanner);
  slp_buffer_destroy(buffer);
}

static void test_large_buffer_error_at_end() {
  slp_buffer_t *buffer = slp_buffer_create(2048);
  ASSERT(buffer != NULL);

  uint8_t data[] = "alpha 42 beta -17 3.14 gamma +99 delta -2.5 epsilon 0 zeta "
                   "100 eta 0.001 theta -999 iota 42.42 kappa 5.5x";

  slp_buffer_copy_to(buffer, data, strlen((char *)data));
  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  ASSERT(scanner != NULL);

  for (int i = 0; i < 20; i++) {
    slp_scanner_static_type_result_t result =
        slp_scanner_read_static_base_type(scanner, NULL);
    ASSERT(result.success);
  }

  size_t pos_before_error = scanner->position;

  slp_scanner_static_type_result_t error_result =
      slp_scanner_read_static_base_type(scanner, NULL);

  ASSERT(!error_result.success);
  ASSERT(error_result.start_position == pos_before_error);
  ASSERT(error_result.error_position > pos_before_error);
  ASSERT(scanner->position == pos_before_error);

  slp_scanner_free(scanner);
  slp_buffer_destroy(buffer);
}

static void test_large_buffer_invalid_integer_in_sequence() {
  slp_buffer_t *buffer = slp_buffer_create(2048);
  ASSERT(buffer != NULL);

  uint8_t data[] = "1 2 3 4 5 6 7 8 9 10 11 12 13 14 15x 16 17 18 19 20";

  slp_buffer_copy_to(buffer, data, strlen((char *)data));
  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  ASSERT(scanner != NULL);

  for (int i = 0; i < 14; i++) {
    slp_scanner_static_type_result_t result =
        slp_scanner_read_static_base_type(scanner, NULL);
    ASSERT(result.success);
    ASSERT(result.data.base == SLP_STATIC_BASE_INTEGER);
  }

  size_t pos_before_error = scanner->position;

  slp_scanner_static_type_result_t error_result =
      slp_scanner_read_static_base_type(scanner, NULL);

  ASSERT(!error_result.success);
  ASSERT(error_result.start_position == pos_before_error);

  size_t error_offset =
      error_result.error_position - error_result.start_position;
  ASSERT(error_offset == 3);

  ASSERT(scanner->position == pos_before_error);

  slp_scanner_free(scanner);
  slp_buffer_destroy(buffer);
}

static void test_large_buffer_invalid_real_in_sequence() {
  slp_buffer_t *buffer = slp_buffer_create(2048);
  ASSERT(buffer != NULL);

  uint8_t data[] = "1.1 2.2 3.3 4.4 5.5 6.6 7.7 8.8 9.9 10.10 11.1.1 12.12";

  slp_buffer_copy_to(buffer, data, strlen((char *)data));
  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  ASSERT(scanner != NULL);

  for (int i = 0; i < 10; i++) {
    slp_scanner_static_type_result_t result =
        slp_scanner_read_static_base_type(scanner, NULL);
    ASSERT(result.success);
    ASSERT(result.data.base == SLP_STATIC_BASE_REAL);
  }

  size_t pos_before_error = scanner->position;

  slp_scanner_static_type_result_t error_result =
      slp_scanner_read_static_base_type(scanner, NULL);

  ASSERT(!error_result.success);
  ASSERT(error_result.start_position == pos_before_error);
  ASSERT(error_result.error_position > pos_before_error);
  ASSERT(scanner->position == pos_before_error);

  slp_scanner_free(scanner);
  slp_buffer_destroy(buffer);
}

static void test_large_buffer_complex_symbols() {
  slp_buffer_t *buffer = slp_buffer_create(2048);
  ASSERT(buffer != NULL);

  uint8_t data[] = "foo-bar baz_qux test123 abc-def-ghi jkl_mno_pqr "
                   "var1 var2 var3 var4 var5 var6 var7 var8 var9 var10 "
                   "alpha-1 beta-2 gamma-3 delta-4 epsilon-5 "
                   "test_a test_b test_c test_d test_e "
                   "sym1! sym2@ sym3# sym4$ sym5% "
                   "a-b-c d-e-f g-h-i j-k-l m-n-o";

  slp_buffer_copy_to(buffer, data, strlen((char *)data));
  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  ASSERT(scanner != NULL);

  int token_count = 0;
  while (scanner->position < buffer->count) {
    slp_scanner_static_type_result_t result =
        slp_scanner_read_static_base_type(scanner, NULL);

    if (!result.success) {
      break;
    }

    ASSERT(result.data.base == SLP_STATIC_BASE_SYMBOL);
    ASSERT(result.data.byte_length > 0);
    token_count++;
  }

  ASSERT(token_count == 35);

  slp_scanner_free(scanner);
  slp_buffer_destroy(buffer);
}

static void test_large_buffer_all_integers() {
  slp_buffer_t *buffer = slp_buffer_create(2048);
  ASSERT(buffer != NULL);

  uint8_t data[] =
      "0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 "
      "20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 "
      "40 41 42 43 44 45 46 47 48 49";

  slp_buffer_copy_to(buffer, data, strlen((char *)data));
  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  ASSERT(scanner != NULL);

  for (int i = 0; i < 50; i++) {
    slp_scanner_static_type_result_t result =
        slp_scanner_read_static_base_type(scanner, NULL);

    ASSERT(result.success);
    ASSERT(result.data.base == SLP_STATIC_BASE_INTEGER);
  }

  slp_scanner_free(scanner);
  slp_buffer_destroy(buffer);
}

static void test_large_buffer_all_reals() {
  slp_buffer_t *buffer = slp_buffer_create(2048);
  ASSERT(buffer != NULL);

  uint8_t data[] = "0.0 1.1 2.2 3.3 4.4 5.5 6.6 7.7 8.8 9.9 "
                   "10.0 11.1 12.2 13.3 14.4 15.5 16.6 17.7 18.8 19.9 "
                   "20.0 21.1 22.2 23.3 24.4 25.5 26.6 27.7 28.8 29.9 "
                   "30.0 31.1 32.2 33.3 34.4 35.5 36.6 37.7 38.8 39.9";

  slp_buffer_copy_to(buffer, data, strlen((char *)data));
  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  ASSERT(scanner != NULL);

  for (int i = 0; i < 40; i++) {
    slp_scanner_static_type_result_t result =
        slp_scanner_read_static_base_type(scanner, NULL);

    ASSERT(result.success);
    ASSERT(result.data.base == SLP_STATIC_BASE_REAL);
  }

  slp_scanner_free(scanner);
  slp_buffer_destroy(buffer);
}

int main(void) {
  test_large_valid_buffer_mixed_types();
  test_large_buffer_with_whitespace_variations();
  test_large_buffer_error_at_start();
  test_double_period_in_real();
  test_large_buffer_error_in_middle();
  test_large_buffer_error_at_end();
  test_large_buffer_invalid_integer_in_sequence();
  test_large_buffer_invalid_real_in_sequence();
  test_large_buffer_complex_symbols();
  test_large_buffer_all_integers();
  test_large_buffer_all_reals();

  return 0;
}
