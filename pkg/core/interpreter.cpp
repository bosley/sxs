#include "interpreter.hpp"
#include "datum/datum.hpp"
#include <fmt/core.h>
#include <stdexcept>

namespace pkg::core {

class interpreter_c : public callable_context_if {
public:
  interpreter_c(
      const std::map<std::string, callable_symbol_s> &callable_symbols)
      : callable_symbols_(callable_symbols) {
    push_scope();
  }

  ~interpreter_c() override = default;

  slp::slp_object_c eval(slp::slp_object_c &object) override {
    auto type = object.type();

    switch (type) {
    case slp::slp_type_e::INTEGER:
    case slp::slp_type_e::REAL:
    case slp::slp_type_e::DQ_LIST:
    case slp::slp_type_e::RUNE:
      return std::move(object);

    case slp::slp_type_e::SYMBOL: {
      std::string sym = object.as_symbol();
      for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(sym);
        if (found != it->end()) {
          return slp::slp_object_c::from_data(found->second.get_data(),
                                              found->second.get_symbols(),
                                              found->second.get_root_offset());
        }
      }
      return std::move(object);
    }

    case slp::slp_type_e::PAREN_LIST: {
      auto list = object.as_list();
      if (list.empty()) {
        return std::move(object);
      }

      auto first = list.at(0);
      if (first.type() != slp::slp_type_e::SYMBOL) {
        return std::move(object);
      }

      std::string cmd = first.as_symbol();
      auto it = callable_symbols_.find(cmd);
      if (it == callable_symbols_.end()) {
        throw std::runtime_error(
            fmt::format("Unknown callable symbol: {}", cmd));
      }

      return it->second.function(*this, object);
    }

    case slp::slp_type_e::DATUM: {
      const std::uint8_t *base_ptr = object.get_data().data();
      const std::uint8_t *unit_ptr = base_ptr + object.get_root_offset();
      const slp::slp_unit_of_store_t *unit =
          reinterpret_cast<const slp::slp_unit_of_store_t *>(unit_ptr);

      size_t inner_offset = static_cast<size_t>(unit->data.uint64);

      auto inner_obj = slp::slp_object_c::from_data(
          object.get_data(), object.get_symbols(), inner_offset);

      if (inner_obj.type() != slp::slp_type_e::PAREN_LIST) {
        return std::move(object);
      }

      auto list = inner_obj.as_list();
      if (list.empty()) {
        return std::move(object);
      }

      auto first = list.at(0);
      if (first.type() != slp::slp_type_e::SYMBOL) {
        return std::move(object);
      }

      std::string cmd = first.as_symbol();
      auto datum_symbols = datum::get_standard_callable_symbols();
      auto it = datum_symbols.find(cmd);
      if (it == datum_symbols.end()) {
        throw std::runtime_error(
            fmt::format("Unknown datum callable symbol: {}", cmd));
      }

      return it->second.function(*this, inner_obj);
    }

    case slp::slp_type_e::BRACKET_LIST: {
      auto list = object.as_list();
      slp::slp_object_c result;
      for (size_t i = 0; i < list.size(); i++) {
        auto elem = list.at(i);
        result = eval(elem);
      }
      return result;
    }

    default:
      return std::move(object);
    }
  }

  bool has_symbol(const std::string &symbol, bool local_scope_only) override {
    if (local_scope_only) {
      return !scopes_.empty() &&
             scopes_.back().find(symbol) != scopes_.back().end();
    }

    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
      if (it->find(symbol) != it->end()) {
        return true;
      }
    }
    return false;
  }

  bool define_symbol(const std::string &symbol,
                     slp::slp_object_c &object) override {
    if (scopes_.empty()) {
      return false;
    }
    scopes_.back()[symbol] = slp::slp_object_c::from_data(
        object.get_data(), object.get_symbols(), object.get_root_offset());
    return true;
  }

  bool push_scope() override {
    scopes_.emplace_back();
    return true;
  }

  bool pop_scope() override {
    if (scopes_.empty()) {
      return false;
    }
    scopes_.pop_back();
    return true;
  }

private:
  std::map<std::string, callable_symbol_s> callable_symbols_;
  std::vector<std::map<std::string, slp::slp_object_c>> scopes_;
};

std::unique_ptr<callable_context_if> create_interpreter(
    const std::map<std::string, callable_symbol_s> &callable_symbols) {
  return std::make_unique<interpreter_c>(callable_symbols);
}

} // namespace pkg::core
