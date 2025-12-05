#include "../scanner.h"
#include "test.h"
#include <stdlib.h>
#include <string.h>

static void test_find_group_simple_parens() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = "(hello)";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '(', ')', NULL, false);

  ASSERT(result.success);
  ASSERT(result.index_of_start_symbol == 0);
  ASSERT(result.index_of_closing_symbol == 6);
  ASSERT(scanner->position == 6);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_simple_brackets() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = "[data]";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '[', ']', NULL, false);

  ASSERT(result.success);
  ASSERT(result.index_of_start_symbol == 0);
  ASSERT(result.index_of_closing_symbol == 5);
  ASSERT(scanner->position == 5);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_simple_braces() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = "{content}";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '{', '}', NULL, false);

  ASSERT(result.success);
  ASSERT(result.index_of_start_symbol == 0);
  ASSERT(result.index_of_closing_symbol == 8);
  ASSERT(scanner->position == 8);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_custom_delimiters() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = "!a b +1 2$";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '!', '$', NULL, false);

  ASSERT(result.success);
  ASSERT(result.index_of_start_symbol == 0);
  ASSERT(result.index_of_closing_symbol == 9);
  ASSERT(scanner->position == 9);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_sequential_groups() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = "(first)(second)(third)";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);

  slp_scanner_find_group_result_t result1 =
      slp_scanner_find_group(scanner, '(', ')', NULL, false);
  ASSERT(result1.success);
  ASSERT(result1.index_of_start_symbol == 0);
  ASSERT(result1.index_of_closing_symbol == 6);
  ASSERT(scanner->position == 6);

  scanner->position = 7;
  slp_scanner_find_group_result_t result2 =
      slp_scanner_find_group(scanner, '(', ')', NULL, false);
  ASSERT(result2.success);
  ASSERT(result2.index_of_start_symbol == 7);
  ASSERT(result2.index_of_closing_symbol == 14);
  ASSERT(scanner->position == 14);

  scanner->position = 15;
  slp_scanner_find_group_result_t result3 =
      slp_scanner_find_group(scanner, '(', ')', NULL, false);
  ASSERT(result3.success);
  ASSERT(result3.index_of_start_symbol == 15);
  ASSERT(result3.index_of_closing_symbol == 21);
  ASSERT(scanner->position == 21);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_mixed_delimiters() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = "(a)[b]{c}";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);

  slp_scanner_find_group_result_t result1 =
      slp_scanner_find_group(scanner, '(', ')', NULL, false);
  ASSERT(result1.success);
  ASSERT(result1.index_of_start_symbol == 0);
  ASSERT(result1.index_of_closing_symbol == 2);

  scanner->position = 3;
  slp_scanner_find_group_result_t result2 =
      slp_scanner_find_group(scanner, '[', ']', NULL, false);
  ASSERT(result2.success);
  ASSERT(result2.index_of_start_symbol == 3);
  ASSERT(result2.index_of_closing_symbol == 5);

  scanner->position = 6;
  slp_scanner_find_group_result_t result3 =
      slp_scanner_find_group(scanner, '{', '}', NULL, false);
  ASSERT(result3.success);
  ASSERT(result3.index_of_start_symbol == 6);
  ASSERT(result3.index_of_closing_symbol == 8);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_different_custom_delimiters() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = "!foo$<bar>@baz#";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);

  slp_scanner_find_group_result_t result1 =
      slp_scanner_find_group(scanner, '!', '$', NULL, false);
  ASSERT(result1.success);
  ASSERT(result1.index_of_start_symbol == 0);
  ASSERT(result1.index_of_closing_symbol == 4);

  scanner->position = 5;
  slp_scanner_find_group_result_t result2 =
      slp_scanner_find_group(scanner, '<', '>', NULL, false);
  ASSERT(result2.success);
  ASSERT(result2.index_of_start_symbol == 5);
  ASSERT(result2.index_of_closing_symbol == 9);

  scanner->position = 10;
  slp_scanner_find_group_result_t result3 =
      slp_scanner_find_group(scanner, '@', '#', NULL, false);
  ASSERT(result3.success);
  ASSERT(result3.index_of_start_symbol == 10);
  ASSERT(result3.index_of_closing_symbol == 14);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_escaped_quotes() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = "\"hello \\\"world\\\"!\"";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  uint8_t escape = '\\';
  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '"', '"', &escape, false);

  ASSERT(result.success);
  ASSERT(result.index_of_start_symbol == 0);
  ASSERT(result.index_of_closing_symbol == 17);
  ASSERT(scanner->position == 17);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_multiple_escaped_end_symbols() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = "(a\\)b\\)c)";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  uint8_t escape = '\\';
  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '(', ')', &escape, false);

  ASSERT(result.success);
  ASSERT(result.index_of_start_symbol == 0);
  ASSERT(result.index_of_closing_symbol == 8);
  ASSERT(scanner->position == 8);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_escape_at_end_of_buffer() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = "(hello\\";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  uint8_t escape = '\\';
  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '(', ')', &escape, false);

  ASSERT(!result.success);
  ASSERT(scanner->position == 0);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_escape_followed_by_non_end_symbol() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = "(hello\\world)";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  uint8_t escape = '\\';
  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '(', ')', &escape, false);

  ASSERT(result.success);
  ASSERT(result.index_of_start_symbol == 0);
  ASSERT(result.index_of_closing_symbol == 12);
  ASSERT(scanner->position == 12);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_wrong_start_symbol() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = "[hello)";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '(', ')', NULL, false);

  ASSERT(!result.success);
  ASSERT(scanner->position == 0);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_null_scanner() {
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(NULL, '(', ')', NULL, false);

  ASSERT(!result.success);
}

static void test_find_group_empty_buffer() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '(', ')', NULL, false);

  ASSERT(!result.success);
  ASSERT(scanner->position == 0);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_position_at_end() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = "(hello)";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 7);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '(', ')', NULL, false);

  ASSERT(!result.success);
  ASSERT(scanner->position == 7);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_position_not_at_start_symbol() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = "x(hello)";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '(', ')', NULL, false);

  ASSERT(!result.success);
  ASSERT(scanner->position == 0);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_missing_end_symbol() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = "(hello world";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '(', ')', NULL, false);

  ASSERT(!result.success);
  ASSERT(scanner->position == 0);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_only_start_symbol() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = "(";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '(', ')', NULL, false);

  ASSERT(!result.success);
  ASSERT(scanner->position == 0);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_all_escaped_no_real_end() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = "(hello\\)world\\)";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  uint8_t escape = '\\';
  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '(', ')', &escape, false);

  ASSERT(!result.success);
  ASSERT(scanner->position == 0);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_same_start_end_symbols() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = "|content|";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '|', '|', NULL, false);

  ASSERT(result.success);
  ASSERT(result.index_of_start_symbol == 0);
  ASSERT(result.index_of_closing_symbol == 8);
  ASSERT(scanner->position == 8);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_empty_group() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = "()";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '(', ')', NULL, false);

  ASSERT(result.success);
  ASSERT(result.index_of_start_symbol == 0);
  ASSERT(result.index_of_closing_symbol == 1);
  ASSERT(scanner->position == 1);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_nested_groups() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = "(outer(inner))";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '(', ')', NULL, false);

  ASSERT(result.success);
  ASSERT(result.index_of_start_symbol == 0);
  ASSERT(result.index_of_closing_symbol == 13);
  ASSERT(scanner->position == 13);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_no_escape_byte() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = "(hello)";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '(', ')', NULL, false);

  ASSERT(result.success);
  ASSERT(result.index_of_start_symbol == 0);
  ASSERT(result.index_of_closing_symbol == 6);
  ASSERT(scanner->position == 6);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_complex_content() {
  slp_buffer_t *buffer = slp_buffer_new(128);
  uint8_t data[] = "(add 1 2 (mul 3 4) 5)";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '(', ')', NULL, false);

  ASSERT(result.success);
  ASSERT(result.index_of_start_symbol == 0);
  ASSERT(result.index_of_closing_symbol == 20);
  ASSERT(scanner->position == 20);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_with_whitespace() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = "( hello world )";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '(', ')', NULL, false);

  ASSERT(result.success);
  ASSERT(result.index_of_start_symbol == 0);
  ASSERT(result.index_of_closing_symbol == 14);
  ASSERT(scanner->position == 14);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_deeply_nested() {
  slp_buffer_t *buffer = slp_buffer_new(128);
  uint8_t data[] = "(a(b(c(d(e)f)g)h)i)";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '(', ')', NULL, false);

  ASSERT(result.success);
  ASSERT(result.index_of_start_symbol == 0);
  ASSERT(result.index_of_closing_symbol == 18);
  ASSERT(scanner->position == 18);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_multiple_groups_in_buffer() {
  slp_buffer_t *buffer = slp_buffer_new(256);
  uint8_t data[] =
      "(first) some text (second (nested)) more [different] {another}";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);

  slp_scanner_find_group_result_t result1 =
      slp_scanner_find_group(scanner, '(', ')', NULL, false);
  ASSERT(result1.success);
  ASSERT(result1.index_of_start_symbol == 0);
  ASSERT(result1.index_of_closing_symbol == 6);

  scanner->position = 18;
  slp_scanner_find_group_result_t result2 =
      slp_scanner_find_group(scanner, '(', ')', NULL, false);
  ASSERT(result2.success);
  ASSERT(result2.index_of_start_symbol == 18);
  ASSERT(result2.index_of_closing_symbol == 34);

  scanner->position = 41;
  slp_scanner_find_group_result_t result3 =
      slp_scanner_find_group(scanner, '[', ']', NULL, false);
  ASSERT(result3.success);
  ASSERT(result3.index_of_start_symbol == 41);
  ASSERT(result3.index_of_closing_symbol == 51);

  scanner->position = 53;
  slp_scanner_find_group_result_t result4 =
      slp_scanner_find_group(scanner, '{', '}', NULL, false);
  ASSERT(result4.success);
  ASSERT(result4.index_of_start_symbol == 53);
  ASSERT(result4.index_of_closing_symbol == 61);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_escaped_quotes_complex() {
  slp_buffer_t *buffer = slp_buffer_new(128);
  uint8_t data[] = "\"start \\\"nested \\\"double\\\" escape\\\" end\"";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  uint8_t escape = '\\';
  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '"', '"', &escape, false);

  ASSERT(result.success);
  ASSERT(result.index_of_start_symbol == 0);
  ASSERT(result.index_of_closing_symbol == 39);
  ASSERT(scanner->position == 39);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_mixed_nested_with_escapes() {
  slp_buffer_t *buffer = slp_buffer_new(128);
  uint8_t data[] = "(outer \"with \\\"quotes\\\" inside\" (inner))";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '(', ')', NULL, false);

  ASSERT(result.success);
  ASSERT(result.index_of_start_symbol == 0);
  ASSERT(result.index_of_closing_symbol == 39);

  uint8_t escape = '\\';
  scanner->position = 7;
  slp_scanner_find_group_result_t result2 =
      slp_scanner_find_group(scanner, '"', '"', &escape, false);
  ASSERT(result2.success);
  ASSERT(result2.index_of_start_symbol == 7);
  ASSERT(result2.index_of_closing_symbol == 30);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_asymmetric_nesting() {
  slp_buffer_t *buffer = slp_buffer_new(128);
  uint8_t data[] = "((()))";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '(', ')', NULL, false);

  ASSERT(result.success);
  ASSERT(result.index_of_start_symbol == 0);
  ASSERT(result.index_of_closing_symbol == 5);

  scanner->position = 1;
  slp_scanner_find_group_result_t result2 =
      slp_scanner_find_group(scanner, '(', ')', NULL, false);
  ASSERT(result2.success);
  ASSERT(result2.index_of_start_symbol == 1);
  ASSERT(result2.index_of_closing_symbol == 4);

  scanner->position = 2;
  slp_scanner_find_group_result_t result3 =
      slp_scanner_find_group(scanner, '(', ')', NULL, false);
  ASSERT(result3.success);
  ASSERT(result3.index_of_start_symbol == 2);
  ASSERT(result3.index_of_closing_symbol == 3);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_unbalanced_inside_quotes() {
  slp_buffer_t *buffer = slp_buffer_new(128);
  uint8_t data[] = "(text \"with ) inside\" more)";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '(', ')', NULL, false);

  ASSERT(result.success);
  ASSERT(result.index_of_start_symbol == 0);
  ASSERT(result.index_of_closing_symbol == 12);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_large_buffer_with_many_groups() {
  slp_buffer_t *buffer = slp_buffer_new(512);
  uint8_t data[] = "(a)(b)(c)(d)(e)(f)(g)(h)(i)(j)(k)(l)(m)(n)(o)(p)";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);

  for (size_t i = 0; i < 16; i++) {
    scanner->position = i * 3;
    slp_scanner_find_group_result_t result =
        slp_scanner_find_group(scanner, '(', ')', NULL, false);
    ASSERT(result.success);
    ASSERT(result.index_of_start_symbol == i * 3);
    ASSERT(result.index_of_closing_symbol == i * 3 + 2);
  }

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_escape_escape_character() {
  slp_buffer_t *buffer = slp_buffer_new(128);
  uint8_t data[] = "(text with \\\\ backslash)";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  uint8_t escape = '\\';
  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '(', ')', &escape, false);

  ASSERT(result.success);
  ASSERT(result.index_of_start_symbol == 0);
  ASSERT(result.index_of_closing_symbol == 23);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_find_group_max_depth_stress() {
  slp_buffer_t *buffer = slp_buffer_new(512);

  size_t depth = 50;
  for (size_t i = 0; i < depth; i++) {
    slp_buffer_copy_to(buffer, (uint8_t *)"(", 1);
  }
  slp_buffer_copy_to(buffer, (uint8_t *)"x", 1);
  for (size_t i = 0; i < depth; i++) {
    slp_buffer_copy_to(buffer, (uint8_t *)")", 1);
  }

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '(', ')', NULL, false);

  ASSERT(result.success);
  ASSERT(result.index_of_start_symbol == 0);
  ASSERT(result.index_of_closing_symbol == 100);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_consume_leading_ws_with_spaces() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = "   (hello)";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '(', ')', NULL, true);

  ASSERT(result.success);
  ASSERT(result.index_of_start_symbol == 3);
  ASSERT(result.index_of_closing_symbol == 9);
  ASSERT(scanner->position == 9);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_consume_leading_ws_with_tabs() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = "\t\t[data]";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '[', ']', NULL, true);

  ASSERT(result.success);
  ASSERT(result.index_of_start_symbol == 2);
  ASSERT(result.index_of_closing_symbol == 7);
  ASSERT(scanner->position == 7);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_consume_leading_ws_with_newlines() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = "\n\n\r{content}";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '{', '}', NULL, true);

  ASSERT(result.success);
  ASSERT(result.index_of_start_symbol == 3);
  ASSERT(result.index_of_closing_symbol == 11);
  ASSERT(scanner->position == 11);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_consume_leading_ws_mixed_whitespace() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = " \t\n\r  (test)";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '(', ')', NULL, true);

  ASSERT(result.success);
  ASSERT(result.index_of_start_symbol == 6);
  ASSERT(result.index_of_closing_symbol == 11);
  ASSERT(scanner->position == 11);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_consume_leading_ws_false_fails_on_whitespace() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = "   (hello)";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '(', ')', NULL, false);

  ASSERT(!result.success);
  ASSERT(scanner->position == 0);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_consume_leading_ws_all_whitespace_buffer() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = "   \t\n\r  ";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '(', ')', NULL, true);

  ASSERT(!result.success);
  ASSERT(scanner->position == 0);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_consume_leading_ws_no_whitespace() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = "(immediate)";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '(', ')', NULL, true);

  ASSERT(result.success);
  ASSERT(result.index_of_start_symbol == 0);
  ASSERT(result.index_of_closing_symbol == 10);
  ASSERT(scanner->position == 10);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

static void test_consume_leading_ws_nested_groups() {
  slp_buffer_t *buffer = slp_buffer_new(64);
  uint8_t data[] = "  (outer(inner))";
  slp_buffer_copy_to(buffer, data, strlen((char *)data));

  slp_scanner_t *scanner = slp_scanner_new(buffer, 0);
  slp_scanner_find_group_result_t result =
      slp_scanner_find_group(scanner, '(', ')', NULL, true);

  ASSERT(result.success);
  ASSERT(result.index_of_start_symbol == 2);
  ASSERT(result.index_of_closing_symbol == 15);
  ASSERT(scanner->position == 15);

  slp_scanner_free(scanner);
  slp_buffer_free(buffer);
}

int main(void) {
  test_find_group_simple_parens();
  test_find_group_simple_brackets();
  test_find_group_simple_braces();
  test_find_group_custom_delimiters();

  test_find_group_sequential_groups();
  test_find_group_mixed_delimiters();
  test_find_group_different_custom_delimiters();

  test_find_group_escaped_quotes();
  test_find_group_multiple_escaped_end_symbols();
  test_find_group_escape_at_end_of_buffer();
  test_find_group_escape_followed_by_non_end_symbol();

  test_find_group_wrong_start_symbol();
  test_find_group_null_scanner();
  test_find_group_empty_buffer();
  test_find_group_position_at_end();
  test_find_group_position_not_at_start_symbol();

  test_find_group_missing_end_symbol();
  test_find_group_only_start_symbol();
  test_find_group_all_escaped_no_real_end();

  test_find_group_same_start_end_symbols();
  test_find_group_empty_group();
  test_find_group_nested_groups();
  test_find_group_no_escape_byte();
  test_find_group_complex_content();
  test_find_group_with_whitespace();

  test_find_group_deeply_nested();
  test_find_group_multiple_groups_in_buffer();
  test_find_group_escaped_quotes_complex();
  test_find_group_mixed_nested_with_escapes();
  test_find_group_asymmetric_nesting();
  test_find_group_unbalanced_inside_quotes();
  test_find_group_large_buffer_with_many_groups();
  test_find_group_escape_escape_character();
  test_find_group_max_depth_stress();

  test_consume_leading_ws_with_spaces();
  test_consume_leading_ws_with_tabs();
  test_consume_leading_ws_with_newlines();
  test_consume_leading_ws_mixed_whitespace();
  test_consume_leading_ws_false_fails_on_whitespace();
  test_consume_leading_ws_all_whitespace_buffer();
  test_consume_leading_ws_no_whitespace();
  test_consume_leading_ws_nested_groups();

  return 0;
}
