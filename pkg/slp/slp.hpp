#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>

#include "slp/buffer.hpp"

class slp_test_accessor;

namespace slp {

typedef struct slp_unit_of_store_s slp_unit_of_store_t;

class slp_parse_result_c;
class slp_object_c;

union data_u {
  std::int8_t int8;
  std::int16_t int16;
  std::int32_t int32;
  std::int64_t int64;
  std::uint8_t uint8;
  std::uint16_t uint16;
  std::uint32_t uint32;
  std::uint64_t uint64;
  float float32;
  double float64;
  bool boolean;

  slp_unit_of_store_t *typed_range; // Meaning that the data will consist of all
                                    // unit of stores, sequentially

  data_u *data_ptr; // SOME data. Think something similar to '()
};

struct slp_unit_of_store_s {
  std::uint32_t header;
  std::uint32_t flags;
  data_u data;
};

enum class slp_type_e {
  NONE = 0,
  SOME = 1,
  PAREN_LIST = 2,
  BRACE_LIST = 4,
  DQ_LIST = 5,
  SYMBOL = 7,
  RUNE = 8,
  INTEGER = 9,
  REAL = 10,
  BRACKET_LIST = 11,
  ERROR = 12,
};

/*
    We dont actually copy the data into a new object, we just point at
    the raw data, and then infer based on the "meta" how to read the data_u

    The slp_object_c is a "functional" wrapper around this unit of data, meaning
    that it adds means to interact with/ understand the raw data
*/
class slp_object_c {
public:
  class list_c {
  public:
    list_c();
    explicit list_c(const slp_object_c *parent);

    size_t size() const;
    bool empty() const;
    slp_object_c at(size_t index) const;

  private:
    const slp_object_c *parent_;
    bool is_valid_;
  };

  class string_c {
  public:
    string_c();
    explicit string_c(const slp_object_c *parent);

    size_t size() const;
    bool empty() const;
    char at(size_t index) const;
    std::string to_string() const;

  private:
    const slp_object_c *parent_;
    bool is_valid_;
  };

  slp_object_c();
  ~slp_object_c();
  slp_object_c(const slp_object_c &) = delete;
  slp_object_c &operator=(const slp_object_c &) = delete;
  slp_object_c(slp_object_c &&other) noexcept;
  slp_object_c &operator=(slp_object_c &&other) noexcept;

  slp_type_e type() const;
  std::int64_t as_int() const;
  double as_real() const;
  const char *as_symbol() const;
  list_c as_list() const;
  string_c as_string() const;
  bool has_data() const;

  const slp_buffer_c &get_data() const;
  const std::map<std::uint64_t, std::string> &get_symbols() const;
  size_t get_root_offset() const;

  static slp_object_c
  from_data(const slp_buffer_c &data,
            const std::map<std::uint64_t, std::string> &symbols,
            size_t root_offset);

private:
  slp_unit_of_store_t *view_;
  slp_buffer_c data_;
  size_t root_offset_;
  std::map<std::uint64_t, std::string> symbols_;

  friend slp_parse_result_c parse(const std::string &source);
  friend class ::slp_test_accessor;
};

enum class slp_parse_error_e {
  UNCLOSED_PAREN_LIST = 1,
  UNCLOSED_BRACKET_LIST = 2,
  UNCLOSED_BRACE_LIST = 3,
  UNCLOSED_DQ_LIST = 4,
  MALFORMED_NUMERIC_LITERAL = 5,
};

struct slp_parse_error_s {
  slp_parse_error_e error_code;
  std::string message;
  std::uint32_t byte_position;
};

class slp_parse_result_c {
public:
  slp_parse_result_c();
  ~slp_parse_result_c();
  slp_parse_result_c(const slp_parse_result_c &) = delete;
  slp_parse_result_c &operator=(const slp_parse_result_c &) = delete;
  slp_parse_result_c(slp_parse_result_c &&) noexcept = default;
  slp_parse_result_c &operator=(slp_parse_result_c &&) noexcept = default;

  bool is_error() const { return error_.has_value(); }
  bool is_success() const { return !error_.has_value(); }
  const slp_parse_error_s &error() const { return error_.value(); }
  const slp_object_c &object() const { return object_.value(); }
  slp_object_c take() { return std::move(object_).value(); }

private:
  std::optional<slp_parse_error_s> error_;
  std::optional<slp_object_c> object_;

  friend slp_parse_result_c parse(const std::string &source);
};

extern slp_parse_result_c parse(const std::string &source);

} // namespace slp