#pragma once

#include "slp/slp.hpp"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace pkg::core {

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

  // scopes are used for symbol scopes. if we push, we will shadow on eval
  virtual bool push_scope() = 0;
  virtual bool pop_scope() = 0;
};

struct callable_parameter_s {
  std::string name;
  slp::slp_type_e type;
};

struct callable_symbol_s {
  slp::slp_type_e return_type;
  std::vector<callable_parameter_s> required_parameters;
  bool variadic{false};
  std::function<slp::slp_object_c(callable_context_if &context,
                                  slp::slp_object_c &args_list)>
      function;
};

std::unique_ptr<callable_context_if> create_interpreter(
    const std::map<std::string, callable_symbol_s> &callable_symbols);

} // namespace pkg::core