#pragma once

#include <map>
#include <optional>
#include <slp/slp.hpp>
#include <string>

namespace sconf {

enum class sconf_type_e {
  INT8 = 1,
  INT16 = 2,
  INT32 = 3,
  INT64 = 4,
  UINT8 = 5,
  UINT16 = 6,
  UINT32 = 7,
  UINT64 = 8,
  FLOAT32 = 9,
  FLOAT64 = 10,
  STRING = 11,
  LIST = 12,
  LIST_INT8 = 13,
  LIST_INT16 = 14,
  LIST_INT32 = 15,
  LIST_INT64 = 16,
  LIST_UINT8 = 17,
  LIST_UINT16 = 18,
  LIST_UINT32 = 19,
  LIST_UINT64 = 20,
  LIST_FLOAT32 = 21,
  LIST_FLOAT64 = 22,
  LIST_STRING = 23,
  LIST_LIST = 24,
};

enum class sconf_error_e {
  MISSING_FIELD = 1,
  TYPE_MISMATCH = 2,
  INVALID_LIST_ELEMENT = 3,
  INVALID_STRUCTURE = 4,
  SLP_PARSE_ERROR = 5,
};

struct sconf_error_s {
  sconf_error_e error_code;
  std::string message;
  std::string field_name;
};

class sconf_result_c {
public:
  sconf_result_c();
  ~sconf_result_c();
  sconf_result_c(const sconf_result_c &) = delete;
  sconf_result_c &operator=(const sconf_result_c &) = delete;
  sconf_result_c(sconf_result_c &&) noexcept = default;
  sconf_result_c &operator=(sconf_result_c &&) noexcept = default;

  bool is_error() const { return error_.has_value(); }
  bool is_success() const { return !error_.has_value(); }
  const sconf_error_s &error() const { return error_.value(); }
  const std::map<std::string, slp::slp_object_c> &config() const {
    return config_.value();
  }

private:
  std::optional<sconf_error_s> error_;
  std::optional<std::map<std::string, slp::slp_object_c>> config_;

  friend class sconf_builder_c;
};

class sconf_builder_c {
public:
  static sconf_builder_c from(const std::string &source);

  sconf_builder_c &WithField(sconf_type_e type, const std::string &name);
  sconf_builder_c &WithList(sconf_type_e element_type, const std::string &name);
  sconf_result_c Parse();

  sconf_builder_c(const sconf_builder_c &) = delete;
  sconf_builder_c &operator=(const sconf_builder_c &) = delete;
  sconf_builder_c(sconf_builder_c &&) noexcept = default;
  sconf_builder_c &operator=(sconf_builder_c &&) noexcept = default;

private:
  explicit sconf_builder_c(const std::string &source);

  struct requirement_s {
    sconf_type_e type;
    bool is_list;
  };

  std::string source_;
  std::map<std::string, requirement_s> requirements_;
};

} // namespace sconf
