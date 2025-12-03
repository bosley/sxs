#pragma once

#include <functional>
#include <map>
#include <memory>
#include <slp/slp.hpp>
#include <string>
#include <vector>

#include "instructions/generation/generation.hpp"

namespace pkg::core {

class compiler_context_if;
struct type_info_s;

namespace kernels {
class kernel_context_if;
}

// SLP doesnt contain functions by design. its simple objects. that means in
// order to call a function we can't simply eval it. We will store lambdas as
// "aberrant" objects and use their integer internals as a lookup for the
// definition in-memory on eval so when we get an aberrant we will expect an
// aberrant int like "?my-fn" we wont type it like this obviously, but thats
// what the slp data would look like to "call" function mapped to my-fn instead
// they will use (my-fn arg1 arg2) and "my-fn" symbol will eval in the
// environment to "?my-fn" aberrant cell. we then see that we loaded this, and
// lookup fn "my-fn" to then load the slp object that is the body of the
// function and execute it given the arguments by pushing scope, setting symbols
// as-per the parameters passed and identifierd by the callable_symbol_c then we
// execute, then pop the scope.

struct callable_parameter_s {
  std::string name;
  slp::slp_type_e type;
};

class callable_context_if {
public:
  virtual ~callable_context_if() = default;

  // if a symbol needs to be evald we can do so by calling this to get the
  // actual data so function implementations dont have to worry about specific
  // lookup
  virtual slp::slp_object_c eval(slp::slp_object_c &object) = 0;

  virtual bool has_symbol(const std::string &symbol,
                          bool local_scope_only = false) = 0;

  // always defines to the local scope, can not define to a parent scope PERIOD
  virtual bool define_symbol(const std::string &symbol,
                             slp::slp_object_c &object) = 0;

  // when the function gets something that it needs to determin is a valid type
  // or not it calls this if returns true the out_type will be set tot he slp
  // type that the symbol encodes (example: :int :real :str etc)
  virtual bool is_symbol_enscribing_valid_type(const std::string &symbol,
                                               slp::slp_type_e &out_type) = 0;

  // scopes are used for symbol scopes. if we push, we will shadow on eval
  virtual bool push_scope() = 0;
  virtual bool pop_scope() = 0;

  virtual std::uint64_t allocate_lambda_id() = 0;
  virtual bool register_lambda(
      std::uint64_t id, const std::vector<callable_parameter_s> &parameters,
      slp::slp_type_e return_type, const slp::slp_object_c &body) = 0;

  virtual kernels::kernel_context_if *get_kernel_context() = 0;

  virtual std::string get_lambda_signature(std::uint64_t lambda_id) = 0;

  virtual void push_loop_context() = 0;
  virtual void pop_loop_context() = 0;
  virtual bool is_in_loop() = 0;
  virtual void signal_loop_done(slp::slp_object_c &value) = 0;
  virtual bool should_exit_loop() = 0;
  virtual slp::slp_object_c get_loop_return_value() = 0;
  virtual std::int64_t get_current_iteration() = 0;
  virtual void increment_iteration() = 0;

  virtual bool define_form(const std::string &name,
                           const std::vector<slp::slp_type_e> &elements) = 0;
  virtual bool has_form(const std::string &name) = 0;
  virtual std::vector<slp::slp_type_e>
  get_form_definition(const std::string &name) = 0;
};

struct callable_symbol_s {
  slp::slp_type_e return_type;

  // If they take "any" type use "aberrant"
  // if returns more than one type (always one object, but more than one type)
  // then use "aberrant"
  std::vector<callable_parameter_s> required_parameters;

  std::map<std::string, slp::slp_type_e> injected_symbols;

  // The function to call when compiling - this will produce the raw binary
  // equivalent of the builtin (stubbed in with function signatures expected to
  // change)
  instructions::generation::instruction_generator_fn_t instruction_generator;

  // if they are strictly variadic, leave required_params
  // as minimum size required. (variadic that permits 0 should
  // have 0 params. a variadic that takes at LEAST 1 has 1 ANY, etc)
  bool variadic{false};
  std::function<slp::slp_object_c(callable_context_if &context,
                                  slp::slp_object_c &args_list)>
      function;
  std::function<type_info_s(compiler_context_if &context,
                            slp::slp_object_c &args_list)>
      typecheck_function;
};

std::unique_ptr<callable_context_if> create_interpreter(
    const std::map<std::string, callable_symbol_s> &callable_symbols,
    kernels::kernel_context_if *kernel_context = nullptr);

} // namespace pkg::core