# Adding Core Commands

`warning:` an ai generated this based off a diff from when i implemented a command in `core`. I reviewed the doc, but its possible
that i later asked it to update, didnt remember to update this message, and then the thing hallucinated. who knows!
for source of truth read the actual code, fool

## Location
`apps/pkg/core/instructions/instructions.cpp`

## Structure

### callable_symbol_s
```cpp
struct callable_symbol_s {
  slp::slp_type_e return_type;
  std::vector<callable_parameter_s> required_parameters;
  bool variadic;
  std::function<slp::slp_object_c(callable_context_if &context,
                                  slp::slp_object_c &args_list)> function;
};
```

### Return Types
- `slp::slp_type_e::NONE` - No return value
- `slp::slp_type_e::INTEGER` - Returns integer
- `slp::slp_type_e::REAL` - Returns real
- `slp::slp_type_e::SYMBOL` - Returns symbol
- `slp::slp_type_e::DQ_LIST` - Returns string
- `slp::slp_type_e::ABERRANT` - Returns any type (use for polymorphic returns)

Additional types supported in type map (for type checking with `:type` syntax):
- `:rune` - Single character
- `:list-p` / `:list` - Paren list
- `:list-c` - Brace list
- `:list-b` - Bracket list
- `:some` - Some value (option type)
- `:error` - Error type
- `:datum` - Datum type
- `:aberrant` - Aberrant type (lambdas)
- `:any` - Any type (maps to NONE for wildcard)
- Variadic suffix `..` - e.g., `:int..` for variadic parameters

## Adding a Command

### 1. Add to symbols map in `get_standard_callable_symbols()`

```cpp
symbols["command-name"] = callable_symbol_s{
    .return_type = slp::slp_type_e::ABERRANT,
    .required_parameters = {},
    .variadic = false,
    .function = [](callable_context_if &context,
                   slp::slp_object_c &args_list) -> slp::slp_object_c {
      auto list = args_list.as_list();
      
      // Validate argument count
      if (list.size() != 3) {
        throw std::runtime_error("command-name requires exactly 2 arguments");
      }
      
      // Extract arguments (index 0 is the command name)
      auto arg1 = list.at(1);
      auto arg2 = list.at(2);
      
      // Evaluate arguments if needed
      auto evaled_arg1 = context.eval(arg1);
      
      // Implement logic
      // ...
      
      // Return result
      return result_object;
    }};
```

## Context API

### Evaluation
- `context.eval(object)` - Evaluate an SLP object

### Symbol Management
- `context.has_symbol(name, local_only)` - Check if symbol exists
- `context.define_symbol(name, object)` - Define symbol in current scope

### Type Checking
- `context.is_symbol_enscribing_valid_type(symbol, out_type)` - Validate type symbols like `:int`

### Scope Management
- `context.push_scope()` - Create new scope
- `context.pop_scope()` - Exit scope

### Lambda Management
- `context.allocate_lambda_id()` - Get unique lambda ID
- `context.register_lambda(id, params, return_type, body)` - Register lambda

### Import/Kernel Access
- `context.get_import_context()` - Access import system
- `context.get_kernel_context()` - Access kernel functions
- `context.copy_lambda_from(source, lambda_id)` - Copy lambda definition from another context
- `context.get_import_interpreter(symbol_prefix)` - Get interpreter for imported module

## Argument Handling

### Accessing Arguments
```cpp
auto list = args_list.as_list();
auto first_arg = list.at(1);  // Index 0 is command name
```

### Type Checking
```cpp
if (arg.type() != slp::slp_type_e::SYMBOL) {
  throw std::runtime_error("Expected symbol");
}
```

### Extracting Values
```cpp
std::int64_t int_val = obj.as_int();
double real_val = obj.as_real();
std::string symbol_val = obj.as_symbol();
auto list_val = obj.as_list();
std::string string_val = obj.as_string().to_string();
```

## Creating Return Values

### Return Nothing
```cpp
slp::slp_object_c result;
return result;
```

### Return Existing Object
```cpp
return context.eval(some_object);
```

### Create ERROR Object
```cpp
std::string error_msg = "@(error message here)";
auto error_parse = slp::parse(error_msg);
return error_parse.take();
```

### Create ABERRANT (Lambda ID)
```cpp
std::uint64_t lambda_id = context.allocate_lambda_id();
context.register_lambda(lambda_id, parameters, return_type, body_obj);

slp::slp_buffer_c buffer;
buffer.resize(sizeof(slp::slp_unit_of_store_t));
auto *unit = reinterpret_cast<slp::slp_unit_of_store_t *>(buffer.data());
unit->header = static_cast<std::uint32_t>(slp::slp_type_e::ABERRANT);
unit->flags = 0;
unit->data.uint64 = lambda_id;

return slp::slp_object_c::from_data(buffer, {}, 0);
```

## Testing

### 1. Create test SXS file
`apps/tests/unit/core/test_command_name.sxs`

```sxs
[
  (def result (command-name arg1 arg2))
  (debug "Result:" result)
]
```

### 2. Create test CPP file
`apps/tests/unit/core/command_name_test.cpp`

```cpp
#include <core/instructions/instructions.hpp>
#include <core/interpreter.hpp>
#include <fstream>
#include <snitch/snitch.hpp>
#include <sstream>
#include <sxs/slp/slp.hpp>

namespace {
std::string load_test_file(const std::string &filename) {
  std::string path = std::string(TEST_DATA_DIR) + "/" + filename;
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open test file: " + path);
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}
}

TEST_CASE("command-name - basic test", "[unit][core][command-name]") {
  auto source = load_test_file("test_command_name.sxs");
  auto parse_result = slp::parse(source);
  CHECK(parse_result.is_success());
  
  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);
  
  auto obj = parse_result.take();
  CHECK_NOTHROW(interpreter->eval(obj));
}

TEST_CASE("command-name - specific behavior", "[unit][core][command-name]") {
  std::string source = R"([
    (def result (command-name 1 2))
  ])";
  
  auto parse_result = slp::parse(source);
  REQUIRE(parse_result.is_success());
  
  auto symbols = pkg::core::instructions::get_standard_callable_symbols();
  auto interpreter = pkg::core::create_interpreter(symbols);
  
  auto obj = parse_result.take();
  interpreter->eval(obj);
  
  CHECK(interpreter->has_symbol("result"));
  
  auto result_parsed = slp::parse("result");
  REQUIRE(result_parsed.is_success());
  auto result_obj = result_parsed.take();
  auto result_val = interpreter->eval(result_obj);
  
  CHECK(result_val.type() == slp::slp_type_e::INTEGER);
  CHECK(result_val.as_int() == 42);
}
```

### 3. Update CMakeLists.txt
`apps/tests/unit/core/CMakeLists.txt`

Add to `SXS_TEST_FILES`:
```cmake
test_command_name.sxs
```

Add test executable:
```cmake
add_executable(command_name_tests
  command_name_test.cpp
)

target_link_libraries(command_name_tests PRIVATE 
  extern::snitch
  pkg::core
  ${SXS_SLP_LIB}
)

add_dependencies(build_tests command_name_tests)
add_test(NAME command_name_tests COMMAND command_name_tests)
```

### 4. Build and run
```bash
cd apps/build
cmake ..
make command_name_tests
./tests/unit/core/command_name_tests
ctest
```

## Examples

### def (simple, no return)
- Validates 2 arguments
- Checks first is symbol
- Evaluates second argument
- Defines symbol in current scope
- Returns NONE

### fn (complex, returns ABERRANT)
- Validates 3 arguments
- Parses parameter list
- Validates type symbols
- Allocates lambda ID
- Registers lambda with body
- Returns ABERRANT object with lambda ID

### if (conditional, returns ABERRANT)
- Validates 3 arguments
- Evaluates condition
- Type-based truthiness (non-int = true, int: 0 = false, non-zero = true)
- Evaluates and returns selected branch
- Returns ABERRANT (polymorphic return)

### debug (variadic, returns NONE)
- Variadic arguments
- Iterates all arguments after command name
- Evaluates each
- Prints based on type
- Returns NONE

### export (uses context, returns NONE)
- Validates 2 arguments
- Evaluates value
- Defines in local scope
- Gets import context
- Registers export
- Returns NONE

    this all appears correect - bosley