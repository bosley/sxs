#include "sconf/sconf.hpp"
#include <slp/slp.hpp>

namespace sconf {

sconf_result_c::sconf_result_c()
    : error_(std::nullopt), config_(std::nullopt) {}

sconf_result_c::~sconf_result_c() {}

sconf_builder_c::sconf_builder_c(const std::string &source) : source_(source) {}

sconf_builder_c sconf_builder_c::from(const std::string &source) {
  return sconf_builder_c(source);
}

sconf_builder_c &sconf_builder_c::WithField(sconf_type_e type,
                                            const std::string &name) {
  requirements_[name] = requirement_s{type, false};
  return *this;
}

sconf_builder_c &sconf_builder_c::WithList(sconf_type_e element_type,
                                           const std::string &name) {
  sconf_type_e list_type;
  switch (element_type) {
  case sconf_type_e::INT8:
    list_type = sconf_type_e::LIST_INT8;
    break;
  case sconf_type_e::INT16:
    list_type = sconf_type_e::LIST_INT16;
    break;
  case sconf_type_e::INT32:
    list_type = sconf_type_e::LIST_INT32;
    break;
  case sconf_type_e::INT64:
    list_type = sconf_type_e::LIST_INT64;
    break;
  case sconf_type_e::UINT8:
    list_type = sconf_type_e::LIST_UINT8;
    break;
  case sconf_type_e::UINT16:
    list_type = sconf_type_e::LIST_UINT16;
    break;
  case sconf_type_e::UINT32:
    list_type = sconf_type_e::LIST_UINT32;
    break;
  case sconf_type_e::UINT64:
    list_type = sconf_type_e::LIST_UINT64;
    break;
  case sconf_type_e::FLOAT32:
    list_type = sconf_type_e::LIST_FLOAT32;
    break;
  case sconf_type_e::FLOAT64:
    list_type = sconf_type_e::LIST_FLOAT64;
    break;
  case sconf_type_e::STRING:
    list_type = sconf_type_e::LIST_STRING;
    break;
  case sconf_type_e::LIST:
    list_type = sconf_type_e::LIST_LIST;
    break;
  default:
    list_type = element_type;
    break;
  }
  requirements_[name] = requirement_s{list_type, true};
  return *this;
}

static bool validate_type(const slp::slp_object_c &obj,
                          sconf_type_e expected_type) {
  auto slp_type = obj.type();

  switch (expected_type) {
  case sconf_type_e::INT8:
  case sconf_type_e::INT16:
  case sconf_type_e::INT32:
  case sconf_type_e::INT64:
  case sconf_type_e::UINT8:
  case sconf_type_e::UINT16:
  case sconf_type_e::UINT32:
  case sconf_type_e::UINT64:
    return slp_type == slp::slp_type_e::INTEGER;

  case sconf_type_e::FLOAT32:
  case sconf_type_e::FLOAT64:
    return slp_type == slp::slp_type_e::REAL;

  case sconf_type_e::STRING:
    return slp_type == slp::slp_type_e::DQ_LIST;

  case sconf_type_e::LIST:
    return slp_type == slp::slp_type_e::PAREN_LIST ||
           slp_type == slp::slp_type_e::BRACKET_LIST ||
           slp_type == slp::slp_type_e::BRACE_LIST;

  default:
    return false;
  }
}

static sconf_type_e get_element_type(sconf_type_e list_type) {
  switch (list_type) {
  case sconf_type_e::LIST_INT8:
    return sconf_type_e::INT8;
  case sconf_type_e::LIST_INT16:
    return sconf_type_e::INT16;
  case sconf_type_e::LIST_INT32:
    return sconf_type_e::INT32;
  case sconf_type_e::LIST_INT64:
    return sconf_type_e::INT64;
  case sconf_type_e::LIST_UINT8:
    return sconf_type_e::UINT8;
  case sconf_type_e::LIST_UINT16:
    return sconf_type_e::UINT16;
  case sconf_type_e::LIST_UINT32:
    return sconf_type_e::UINT32;
  case sconf_type_e::LIST_UINT64:
    return sconf_type_e::UINT64;
  case sconf_type_e::LIST_FLOAT32:
    return sconf_type_e::FLOAT32;
  case sconf_type_e::LIST_FLOAT64:
    return sconf_type_e::FLOAT64;
  case sconf_type_e::LIST_STRING:
    return sconf_type_e::STRING;
  case sconf_type_e::LIST_LIST:
    return sconf_type_e::LIST;
  default:
    return sconf_type_e::INT64;
  }
}

static bool validate_list(const slp::slp_object_c &obj,
                          sconf_type_e expected_list_type,
                          std::string &error_msg) {
  auto slp_type = obj.type();

  if (slp_type != slp::slp_type_e::PAREN_LIST &&
      slp_type != slp::slp_type_e::BRACKET_LIST &&
      slp_type != slp::slp_type_e::BRACE_LIST) {
    error_msg = "Expected list type";
    return false;
  }

  auto list = obj.as_list();
  sconf_type_e element_type = get_element_type(expected_list_type);

  if (expected_list_type == sconf_type_e::LIST_LIST) {
    for (size_t i = 0; i < list.size(); ++i) {
      auto elem = list.at(i);
      auto elem_type = elem.type();
      if (elem_type != slp::slp_type_e::PAREN_LIST &&
          elem_type != slp::slp_type_e::BRACKET_LIST &&
          elem_type != slp::slp_type_e::BRACE_LIST) {
        error_msg =
            "List element at index " + std::to_string(i) + " is not a list";
        return false;
      }
    }
  } else {
    for (size_t i = 0; i < list.size(); ++i) {
      auto elem = list.at(i);
      if (!validate_type(elem, element_type)) {
        error_msg = "List element type mismatch at index " + std::to_string(i);
        return false;
      }
    }
  }

  return true;
}

sconf_result_c sconf_builder_c::Parse() {
  sconf_result_c result;

  auto parse_result = slp::parse(source_);

  if (parse_result.is_error()) {
    sconf_error_s err;
    err.error_code = sconf_error_e::SLP_PARSE_ERROR;
    err.message = parse_result.error().message;
    err.field_name = "";
    result.error_ = err;
    return result;
  }

  const auto &root = parse_result.object();

  if (root.type() != slp::slp_type_e::BRACKET_LIST) {
    sconf_error_s err;
    err.error_code = sconf_error_e::INVALID_STRUCTURE;
    err.message = "Configuration must be a bracket list";
    err.field_name = "";
    result.error_ = err;
    return result;
  }

  auto root_list = root.as_list();
  std::map<std::string, slp::slp_object_c> config_map;

  for (size_t i = 0; i < root_list.size(); ++i) {
    auto pair = root_list.at(i);

    if (pair.type() != slp::slp_type_e::PAREN_LIST) {
      sconf_error_s err;
      err.error_code = sconf_error_e::INVALID_STRUCTURE;
      err.message = "Each configuration entry must be a paren list pair";
      err.field_name = "";
      result.error_ = err;
      return result;
    }

    auto pair_list = pair.as_list();
    if (pair_list.size() != 2) {
      sconf_error_s err;
      err.error_code = sconf_error_e::INVALID_STRUCTURE;
      err.message = "Each configuration entry must be a pair (key value)";
      err.field_name = "";
      result.error_ = err;
      return result;
    }

    auto key_obj = pair_list.at(0);
    if (key_obj.type() != slp::slp_type_e::SYMBOL) {
      sconf_error_s err;
      err.error_code = sconf_error_e::INVALID_STRUCTURE;
      err.message = "Configuration keys must be symbols";
      err.field_name = "";
      result.error_ = err;
      return result;
    }

    std::string key = key_obj.as_symbol();
    auto value_obj = pair_list.at(1);

    config_map.emplace(key, std::move(value_obj));
  }

  for (const auto &req : requirements_) {
    const std::string &field_name = req.first;
    const requirement_s &requirement = req.second;

    auto it = config_map.find(field_name);
    if (it == config_map.end()) {
      sconf_error_s err;
      err.error_code = sconf_error_e::MISSING_FIELD;
      err.message = "Required field not found: " + field_name;
      err.field_name = field_name;
      result.error_ = err;
      return result;
    }

    const auto &value = it->second;

    if (requirement.is_list) {
      std::string error_msg;
      if (!validate_list(value, requirement.type, error_msg)) {
        sconf_error_s err;
        err.error_code = sconf_error_e::INVALID_LIST_ELEMENT;
        err.message = "Field '" + field_name + "': " + error_msg;
        err.field_name = field_name;
        result.error_ = err;
        return result;
      }
    } else {
      if (!validate_type(value, requirement.type)) {
        sconf_error_s err;
        err.error_code = sconf_error_e::TYPE_MISMATCH;
        err.message = "Field '" + field_name + "' has incorrect type";
        err.field_name = field_name;
        result.error_ = err;
        return result;
      }
    }
  }

  result.config_ = std::move(config_map);
  return result;
}

} // namespace sconf
