# SLP - S-expression List Parser

A compact S-expression parser for Lisp-like syntax with efficient binary storage and type-safe access.

## Supported Syntax

### Data Types
- **Integers**: `42`, `-123`, `9223372036854775807`
- **Reals**: `3.14`, `1.23e10`, `-5.67e-3`
- **Symbols**: `hello`, `my-symbol`, `my_variable`
- **Strings**: `"hello world"` (DQ_LIST type)

### List Forms
- **Paren Lists**: `(1 2 3)` - Parentheses-delimited lists
- **Bracket Lists**: `[(let a 2) (let b 0)]` - Square bracket-delimited lists
- **Brace Lists**: `{my-env a}` - Curly brace-delimited lists

SLP does not enforce semantics for list types - they are syntactic distinctions available for higher-level interpreters to assign meaning.

### Operators
- **Quote**: `'(1 2 3)` - Wraps object in SOME type
- **Error**: `@not-found` - Marks object as ERROR type
- **Comments**: `; comment text` - Line comments (semicolon to newline)

### Nesting
All forms can be arbitrarily nested: `(let env [(let a 2)]) (putln {env a})`

## Usage

```cpp
#include <slp/slp.hpp>

auto result = slp::parse("(let env [(let a 2) (let b 0)])");
if (result.is_success()) {
    auto obj = result.object();
    auto list = obj.as_list();
    
    auto bracket_elem = list.at(2);
    if (bracket_elem.type() == slp::slp_type_e::BRACKET_LIST) {
        auto bracket_list = bracket_elem.as_list();
        auto first_elem = bracket_list.at(0);
    }
}
```

## Architecture

### Binary Storage Format
Objects are stored as `slp_unit_of_store_t` structures in a contiguous byte buffer:
- **Header**: Type information (PAREN_LIST, INTEGER, SYMBOL, etc.)
- **Flags**: Element count for lists, character count for strings
- **Data Union**: Type-specific payload (int64, float64, uint64 symbol ID, etc.)

### Symbol Tables
Symbols are deduplicated via a symbol table mapping uint64 IDs to strings, reducing memory overhead for repeated symbols.

### View-Based Access
`slp_object_c` provides a view over the binary data without copying. Objects use move semantics only - no copy constructor or assignment.

### List and String Accessors
- `list_c`: Type-safe list iteration with `size()`, `empty()`, `at(index)`
- `string_c`: String access with `size()`, `at(index)`, `to_string()`

Both accessors validate types and return safe defaults for invalid operations.

## Parse Errors

Parsing returns `slp_parse_result_c` which can be checked with `is_success()` or `is_error()`. Error types include:
- `UNCLOSED_PAREN_LIST`
- `UNCLOSED_BRACKET_LIST`
- `UNCLOSED_BRACE_LIST`
- `UNCLOSED_DQ_LIST`
- `MALFORMED_NUMERIC_LITERAL`

Each error includes byte position and descriptive message.

## Type Safety

Type accessors return safe defaults when called on incorrect types:
- `as_int()` returns `0` for non-integers
- `as_real()` returns `0.0` for non-reals
- `as_symbol()` returns `""` for non-symbols
- `as_list()` returns empty list for non-lists
- `as_string()` returns empty string for non-strings

