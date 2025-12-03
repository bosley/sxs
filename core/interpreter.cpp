#include "interpreter.hpp"
#include "core/instructions/datum.hpp"
#include "core/kernels/kernels.hpp"
#include <atomic>
#include <fmt/core.h>
#include <stdexcept>

namespace pkg::core {

struct function_definition_s {
  std::vector<callable_parameter_s> parameters;
  slp::slp_type_e return_type;
  slp::slp_object_c body;
  size_t scope_level;
};

struct loop_context_s {
  std::atomic<bool> done_flag{false};
  slp::slp_object_c return_value;
  std::int64_t iteration{1};

  loop_context_s() = default;

  loop_context_s(loop_context_s &&other) noexcept
      : done_flag(other.done_flag.load()),
        return_value(std::move(other.return_value)),
        iteration(other.iteration) {}

  loop_context_s &operator=(loop_context_s &&other) noexcept {
    if (this != &other) {
      done_flag.store(other.done_flag.load());
      return_value = std::move(other.return_value);
      iteration = other.iteration;
    }
    return *this;
  }

  loop_context_s(const loop_context_s &) = delete;
  loop_context_s &operator=(const loop_context_s &) = delete;
};

class interpreter_c : public callable_context_if {
public:
  interpreter_c(
      const std::map<std::string, callable_symbol_s> &callable_symbols,
      kernels::kernel_context_if *kernel_context)
      : callable_symbols_(callable_symbols), kernel_context_(kernel_context),
        next_lambda_id_(1), current_scope_level_(0),
        kernels_locked_triggered_(false) {
    initialize_type_map();
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

    case slp::slp_type_e::ABERRANT:
      return std::move(object);

    case slp::slp_type_e::SOME: {
      const std::uint8_t *base_ptr = object.get_data().data();
      const std::uint8_t *unit_ptr = base_ptr + object.get_root_offset();
      const slp::slp_unit_of_store_t *unit =
          reinterpret_cast<const slp::slp_unit_of_store_t *>(unit_ptr);
      size_t inner_offset = static_cast<size_t>(unit->data.uint64);

      auto inner_obj = slp::slp_object_c::from_data(
          object.get_data(), object.get_symbols(), inner_offset);
      return std::move(inner_obj);
    }

    case slp::slp_type_e::PAREN_LIST: {
      auto list = object.as_list();
      if (list.empty()) {
        return std::move(object);
      }

      auto first = list.at(0);
      if (first.type() != slp::slp_type_e::SYMBOL) {
        throw std::runtime_error(fmt::format("Cannot call non-symbol type: {}",
                                             static_cast<int>(first.type())));
      }

      std::string cmd = first.as_symbol();
      auto it = callable_symbols_.find(cmd);
      if (it != callable_symbols_.end()) {
        return it->second.function(*this, object);
      }

      if (kernel_context_ && kernel_context_->has_function(cmd)) {
        auto *kernel_func = kernel_context_->get_function(cmd);
        if (kernel_func) {
          return kernel_func->function(*this, object);
        }
      }

      auto evaled_first = eval(first);
      if (evaled_first.type() == slp::slp_type_e::ABERRANT) {
        return handle_aberrant_call(evaled_first, list);
      }

      throw std::runtime_error(fmt::format("Unknown callable symbol: {}", cmd));
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

      auto local_it = callable_symbols_.find(cmd);
      if (local_it != callable_symbols_.end()) {
        return local_it->second.function(*this, inner_obj);
      }

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

        if (!kernels_locked_triggered_ &&
            elem.type() != slp::slp_type_e::DATUM) {
          trigger_kernel_lock();
          kernels_locked_triggered_ = true;
        }

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

  bool is_symbol_enscribing_valid_type(const std::string &symbol,
                                       slp::slp_type_e &out_type) override {
    auto it = type_symbol_map_.find(symbol);
    if (it != type_symbol_map_.end()) {
      out_type = it->second;
      return true;
    }

    if (symbol.size() > 1 && symbol[0] == ':') {
      std::string form_name = symbol.substr(1);

      if (form_name.size() > 2 &&
          form_name.substr(form_name.size() - 2) == "..") {
        form_name = form_name.substr(0, form_name.size() - 2);
      }

      if (form_definitions_.find(form_name) != form_definitions_.end()) {
        out_type = slp::slp_type_e::BRACE_LIST;
        return true;
      }
    }

    return false;
  }

  bool push_scope() override {
    scopes_.emplace_back();
    current_scope_level_++;
    return true;
  }

  bool pop_scope() override {
    if (scopes_.empty()) {
      return false;
    }
    cleanup_lambdas_at_scope(current_scope_level_);
    scopes_.pop_back();
    current_scope_level_--;
    return true;
  }

  std::uint64_t allocate_lambda_id() override { return next_lambda_id_++; }

  bool register_lambda(std::uint64_t id,
                       const std::vector<callable_parameter_s> &parameters,
                       slp::slp_type_e return_type,
                       const slp::slp_object_c &body) override {
    function_definition_s def;
    def.parameters = parameters;
    def.return_type = return_type;
    def.body = slp::slp_object_c::from_data(body.get_data(), body.get_symbols(),
                                            body.get_root_offset());
    def.scope_level = current_scope_level_;
    lambda_definitions_[id] = std::move(def);
    return true;
  }

  kernels::kernel_context_if *get_kernel_context() override {
    return kernel_context_;
  }

  std::string get_lambda_signature(std::uint64_t lambda_id) override {
    auto it = lambda_definitions_.find(lambda_id);
    if (it == lambda_definitions_.end()) {
      return "";
    }

    const auto &def = it->second;

    auto type_to_string = [](slp::slp_type_e type) -> std::string {
      switch (type) {
      case slp::slp_type_e::INTEGER:
        return "int";
      case slp::slp_type_e::REAL:
        return "real";
      case slp::slp_type_e::DQ_LIST:
        return "str";
      case slp::slp_type_e::SYMBOL:
        return "symbol";
      case slp::slp_type_e::PAREN_LIST:
        return "list-p";
      case slp::slp_type_e::BRACE_LIST:
        return "list-c";
      case slp::slp_type_e::BRACKET_LIST:
        return "list-b";
      case slp::slp_type_e::NONE:
        return "none";
      case slp::slp_type_e::SOME:
        return "some";
      case slp::slp_type_e::ERROR:
        return "error";
      case slp::slp_type_e::DATUM:
        return "datum";
      case slp::slp_type_e::ABERRANT:
        return "aberrant";
      case slp::slp_type_e::RUNE:
        return "rune";
      default:
        return "unknown";
      }
    };

    std::string signature = ":fn<";
    for (size_t i = 0; i < def.parameters.size(); i++) {
      if (i > 0)
        signature += ",";
      signature += type_to_string(def.parameters[i].type);
    }
    signature += ">";
    signature += type_to_string(def.return_type);

    return signature;
  }

  void push_loop_context() override { loop_contexts_.emplace_back(); }

  void pop_loop_context() override {
    if (!loop_contexts_.empty()) {
      loop_contexts_.pop_back();
    }
  }

  bool is_in_loop() override { return !loop_contexts_.empty(); }

  void signal_loop_done(slp::slp_object_c &value) override {
    if (loop_contexts_.empty()) {
      throw std::runtime_error("done called outside of do loop");
    }
    loop_contexts_.back().return_value = slp::slp_object_c::from_data(
        value.get_data(), value.get_symbols(), value.get_root_offset());
    loop_contexts_.back().done_flag.store(true);
  }

  bool should_exit_loop() override {
    if (loop_contexts_.empty()) {
      return false;
    }
    return loop_contexts_.back().done_flag.load();
  }

  slp::slp_object_c get_loop_return_value() override {
    if (loop_contexts_.empty()) {
      throw std::runtime_error("No loop context available");
    }
    return slp::slp_object_c::from_data(
        loop_contexts_.back().return_value.get_data(),
        loop_contexts_.back().return_value.get_symbols(),
        loop_contexts_.back().return_value.get_root_offset());
  }

  std::int64_t get_current_iteration() override {
    if (loop_contexts_.empty()) {
      throw std::runtime_error("No loop context available");
    }
    return loop_contexts_.back().iteration;
  }

  void increment_iteration() override {
    if (!loop_contexts_.empty()) {
      loop_contexts_.back().iteration++;
    }
  }

  bool define_form(const std::string &name,
                   const std::vector<slp::slp_type_e> &elements) override {
    form_definitions_[name] = elements;
    type_symbol_map_[":" + name] = slp::slp_type_e::BRACE_LIST;
    type_symbol_map_[":" + name + ".."] = slp::slp_type_e::BRACE_LIST;
    return true;
  }

  bool has_form(const std::string &name) override {
    return form_definitions_.find(name) != form_definitions_.end();
  }

  std::vector<slp::slp_type_e>
  get_form_definition(const std::string &name) override {
    auto it = form_definitions_.find(name);
    if (it != form_definitions_.end()) {
      return it->second;
    }
    throw std::runtime_error(
        fmt::format("Form '{}' not found in form definitions", name));
  }

private:
  void trigger_kernel_lock() {
    if (kernel_context_) {
      kernel_context_->lock();
    }
  }

  void initialize_type_map() {
    std::vector<std::pair<std::string, slp::slp_type_e>> base_types = {
        {"int", slp::slp_type_e::INTEGER},
        {"real", slp::slp_type_e::REAL},
        {"symbol", slp::slp_type_e::SYMBOL},
        {"str", slp::slp_type_e::DQ_LIST},
        {"list-p", slp::slp_type_e::PAREN_LIST},
        {"list-c", slp::slp_type_e::BRACE_LIST},
        {"list-b", slp::slp_type_e::BRACKET_LIST},
        {"none", slp::slp_type_e::NONE},
        {"some", slp::slp_type_e::SOME},
        {"error", slp::slp_type_e::ERROR},
        {"datum", slp::slp_type_e::DATUM},
        {"aberrant", slp::slp_type_e::ABERRANT},
        {"any", slp::slp_type_e::NONE}};

    for (const auto &[name, type] : base_types) {
      type_symbol_map_[":" + name] = type;
      type_symbol_map_[":" + name + ".."] = type;
    }

    type_symbol_map_[":list"] = slp::slp_type_e::PAREN_LIST;
    type_symbol_map_[":list.."] = slp::slp_type_e::PAREN_LIST;
  }

  void cleanup_lambdas_at_scope(size_t level) {
    for (auto it = lambda_definitions_.begin();
         it != lambda_definitions_.end();) {
      if (it->second.scope_level >= level) {
        it = lambda_definitions_.erase(it);
      } else {
        ++it;
      }
    }
  }

  slp::slp_object_c handle_aberrant_call(slp::slp_object_c &aberrant_obj,
                                         slp::slp_object_c::list_c list) {
    const std::uint8_t *base_ptr = aberrant_obj.get_data().data();
    const std::uint8_t *unit_ptr = base_ptr + aberrant_obj.get_root_offset();
    const slp::slp_unit_of_store_t *unit =
        reinterpret_cast<const slp::slp_unit_of_store_t *>(unit_ptr);
    std::uint64_t id = unit->data.uint64;

    /*
      Note: We will handle other compelxt types here. For now, we are just
      resolving the symbol as a lambda, but we will also permit it
    */
    auto lambda_it = lambda_definitions_.find(id);
    if (lambda_it != lambda_definitions_.end()) {
      return handle_lambda_call(id, list);
    }

    throw std::runtime_error("Unknown function");
  }

  slp::slp_object_c handle_lambda_call(std::uint64_t lambda_id,
                                       slp::slp_object_c::list_c list) {
    const auto &func_def = lambda_definitions_[lambda_id];

    if (list.size() - 1 != func_def.parameters.size()) {
      throw std::runtime_error(
          fmt::format("Function expects {} arguments, got {}",
                      func_def.parameters.size(), list.size() - 1));
    }

    std::vector<slp::slp_object_c> arg_values;
    for (size_t i = 1; i < list.size(); i++) {
      auto arg = list.at(i);
      auto evaled_arg = eval(arg);

      const auto &param = func_def.parameters[i - 1];
      if (param.type != slp::slp_type_e::NONE &&
          evaled_arg.type() != param.type) {
        throw std::runtime_error(fmt::format(
            "Argument {} type mismatch: expected {}, got {}", i,
            static_cast<int>(param.type), static_cast<int>(evaled_arg.type())));
      }

      arg_values.push_back(std::move(evaled_arg));
    }

    push_scope();

    for (size_t i = 0; i < func_def.parameters.size(); i++) {
      define_symbol(func_def.parameters[i].name, arg_values[i]);
    }

    auto body_copy = slp::slp_object_c::from_data(
        func_def.body.get_data(), func_def.body.get_symbols(),
        func_def.body.get_root_offset());
    auto result = eval(body_copy);

    if (func_def.return_type != slp::slp_type_e::NONE &&
        result.type() != func_def.return_type) {
      std::string error_msg =
          "@(internal function error: returned unexpected type)";
      auto error_parse = slp::parse(error_msg);
      pop_scope();
      return error_parse.take();
    }

    pop_scope();

    return result;
  }

  std::map<std::string, callable_symbol_s> callable_symbols_;
  std::vector<std::map<std::string, slp::slp_object_c>> scopes_;
  std::map<std::uint64_t, function_definition_s> lambda_definitions_;
  std::map<std::string, slp::slp_type_e> type_symbol_map_;
  std::map<std::string, std::vector<slp::slp_type_e>> form_definitions_;
  std::uint64_t next_lambda_id_;
  size_t current_scope_level_;
  kernels::kernel_context_if *kernel_context_;
  bool kernels_locked_triggered_;
  std::vector<loop_context_s> loop_contexts_;
};

std::unique_ptr<callable_context_if> create_interpreter(
    const std::map<std::string, callable_symbol_s> &callable_symbols,
    kernels::kernel_context_if *kernel_context) {
  return std::make_unique<interpreter_c>(callable_symbols, kernel_context);
}

} // namespace pkg::core
