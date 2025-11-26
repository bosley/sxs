#include "runtime/encoder.hpp"

namespace runtime {

std::string slp_object_to_string(const slp::slp_object_c &obj) {
  auto type = obj.type();

  switch (type) {
  case slp::slp_type_e::INTEGER:
    return std::to_string(obj.as_int());

  case slp::slp_type_e::REAL:
    return std::to_string(obj.as_real());

  case slp::slp_type_e::SYMBOL:
    return obj.as_symbol();

  case slp::slp_type_e::DQ_LIST: {
    std::string content = obj.as_string().to_string();
    std::string escaped;
    escaped.reserve(content.size() + 2);
    escaped += '"';
    for (char c : content) {
      if (c == '"' || c == '\\') {
        escaped += '\\';
      }
      escaped += c;
    }
    escaped += '"';
    return escaped;
  }

  case slp::slp_type_e::ERROR:
    return obj.as_string().to_string();

  case slp::slp_type_e::SOME: {
    if (!obj.has_data()) {
      return "'nil";
    }
    const auto &data = obj.get_data();
    const auto &symbols = obj.get_symbols();
    const slp::slp_unit_of_store_t *some_unit =
        reinterpret_cast<const slp::slp_unit_of_store_t *>(
            &data[obj.get_root_offset()]);
    slp::data_u *inner_ptr = some_unit->data.data_ptr;
    if (!inner_ptr) {
      return "'nil";
    }
    size_t inner_offset =
        reinterpret_cast<const std::uint8_t *>(inner_ptr) - &data[0];
    auto inner_obj = slp::slp_object_c::from_data(data, symbols, inner_offset);
    return "'" + slp_object_to_string(inner_obj);
  }

  case slp::slp_type_e::PAREN_LIST: {
    auto list = obj.as_list();
    std::string result = "(";
    for (size_t i = 0; i < list.size(); i++) {
      if (i > 0)
        result += " ";
      result += slp_object_to_string(list.at(i));
    }
    result += ")";
    return result;
  }

  case slp::slp_type_e::BRACE_LIST: {
    auto list = obj.as_list();
    std::string result = "{";
    for (size_t i = 0; i < list.size(); i++) {
      if (i > 0)
        result += " ";
      result += slp_object_to_string(list.at(i));
    }
    result += "}";
    return result;
  }

  case slp::slp_type_e::BRACKET_LIST: {
    auto list = obj.as_list();
    std::string result = "[";
    for (size_t i = 0; i < list.size(); i++) {
      if (i > 0)
        result += " ";
      result += slp_object_to_string(list.at(i));
    }
    result += "]";
    return result;
  }

  case slp::slp_type_e::RUNE:
    return "nil";

  case slp::slp_type_e::NONE:
  default:
    return "nil";
  }
}

} // namespace runtime
