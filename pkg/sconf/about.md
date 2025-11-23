# SCONF - S-expression Configuration Parser

A type-safe configuration parser built on SLP for validating and loading structured configuration data with compile-time type requirements.

## Format

Configuration files use SLP bracket lists containing key-value pairs:

```lisp
[(server_port 8080)
 (server_host "localhost")
 (worker_threads (1 2 3 4))
 (max_connections 1000)
 (timeout 30.5)]
```

Each entry is a paren list pair `(key value)` where keys are symbols and values can be any SLP type.

## Supported Types

### Scalar Types
- **INT8, INT16, INT32, INT64**: Signed integers
- **UINT8, UINT16, UINT32, UINT64**: Unsigned integers
- **FLOAT32, FLOAT64**: Floating-point numbers
- **STRING**: Double-quoted strings from SLP
- **LIST**: Any list type (PAREN_LIST, BRACKET_LIST, BRACE_LIST)

### List Types
- **LIST_INT8** through **LIST_INT64**: Homogeneous integer lists
- **LIST_UINT8** through **LIST_UINT64**: Homogeneous unsigned integer lists
- **LIST_FLOAT32, LIST_FLOAT64**: Homogeneous floating-point lists
- **LIST_STRING**: Homogeneous string lists
- **LIST_LIST**: Lists containing other lists (non-recursive validation)

All typed lists enforce homogeneity - every element must match the declared type.

## Usage

```cpp
#include <sconf/sconf.hpp>

std::string config = R"([
    (port 8080)
    (host "localhost")
    (workers (1 2 3 4))
    (timeout 30.5)
])";

auto result = sconf::sconf_builder_c::from(config)
    .WithField(sconf::sconf_type_e::INT64, "port")
    .WithField(sconf::sconf_type_e::STRING, "host")
    .WithList(sconf::sconf_type_e::INT64, "workers")
    .WithField(sconf::sconf_type_e::FLOAT64, "timeout")
    .Parse();

if (result.is_success()) {
    const auto& config_map = result.config();
    
    int64_t port = config_map.at("port").as_int();
    std::string host = config_map.at("host").as_string().to_string();
    auto workers = config_map.at("workers").as_list();
    double timeout = config_map.at("timeout").as_real();
}
```

## Builder Pattern

The builder enforces required fields before parsing:

1. **from(source)**: Creates builder with source data
2. **WithField(type, name)**: Declares required scalar field
3. **WithList(element_type, name)**: Declares required list field
4. **Parse()**: Validates and returns result

The builder chains fluently and only parses on `Parse()` call.

## Architecture

### Type Validation

SCONF validates at two levels:

1. **Structure Validation**: Ensures bracket list of paren pairs with symbol keys
2. **Type Validation**: Checks each required field matches declared type

For lists, SCONF validates homogeneity by checking every element matches the expected type.

### LIST_LIST Handling

`LIST_LIST` accepts any list type (paren, bracket, brace) without validating inner contents. This allows heterogeneous data structures:

```cpp
std::string source = "[(matrix ((1 2 3) (4 5 6) (7 8 9)))]";

auto result = sconf::sconf_builder_c::from(source)
    .WithList(sconf::sconf_type_e::LIST, "matrix")
    .Parse();
```

Each element must be a list, but inner elements can be any type.

### Return Type

`Parse()` returns `sconf_result_c` containing either:
- **Success**: `map<string, slp_object_c>` for direct SLP object access
- **Error**: `sconf_error_s` with error code, message, and field name

Configuration values are raw `slp_object_c` instances, allowing full SLP API access.

## Error Handling

### Error Types

- **MISSING_FIELD**: Required field not found in configuration
- **TYPE_MISMATCH**: Field value doesn't match declared type
- **INVALID_LIST_ELEMENT**: List contains wrong element type or non-homogeneous data
- **INVALID_STRUCTURE**: Configuration format is invalid (not bracket list of pairs)
- **SLP_PARSE_ERROR**: Underlying SLP parse failure

Each error includes:
```cpp
sconf_error_e error_code;
std::string message;
std::string field_name;
```

### Validation Strategy

SCONF validates in order:
1. SLP parse succeeds
2. Root is bracket list
3. Each entry is paren pair (key value)
4. Keys are symbols
5. All required fields present
6. Each field matches declared type
7. Lists are homogeneous (except LIST_LIST)

First failure stops validation and returns specific error.

## Design Philosophy

SCONF is not a programming language - it leverages SLP for parsing but enforces strict typing for configuration data. The type system maps directly to C++ primitives, making it ideal for validating application configuration without dynamic typing overhead.

Extra fields are allowed but ignored, supporting forward compatibility where older code can parse newer configuration files with additional fields.

## Type Safety

Unlike dynamic configuration parsers, SCONF requires declaring all fields upfront:

```cpp
.WithField(sconf::sconf_type_e::INT64, "port")
```

This provides:
- **Compile-time documentation**: Required fields visible in code
- **Early validation**: Configuration errors detected at load time
- **Type guarantees**: No runtime type checks needed after parsing

