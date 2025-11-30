# Kernels

## Architecture

- `kernel_manager_c` owns dylib loading and function registration
- `kernel_context_if` exposes operations to runtime
- Kernels are directories containing `kernel.sxs` + dylib
- Functions registered via C API during `kernel_init`

## Dynamic Library Loading

- `dlopen()` loads shared library at runtime
- `dlsym()` resolves `kernel_init` entry point
- `kernel_init` receives registry handle and API table
- Registration callbacks populate `registered_functions_` map
- Optional lifecycle hooks (`on_init`, `on_exit`) can be defined
- `on_init` called automatically after `kernel_init` if present
- `on_exit` called automatically before dylib unload if present
- Handles stored for cleanup on shutdown

## Kernel Resolution

- Search order: absolute path → include_paths → working_directory
- Looks for `kernel_name/kernel.sxs` in each path
- `kernel.sxs` contains metadata: `#(define-kernel name dylib [functions])`
- Dylib name extracted from metadata, loaded from same directory
- Optional `define-ctor` and `define-dtor` specify lifecycle hook names

## Function Registration

- Kernel calls `api->register_function()` during init
- Provides: name, function pointer, return type, variadic flag
- Manager wraps kernel function in `callable_symbol_s` lambda
- Stored as `kernel_name/function_name` in registry
- Functions callable from interpreter like built-in symbols

## Lifecycle Hooks

Kernels can optionally define initialization and cleanup functions:

- `define-ctor` in kernel.sxs specifies init function name (e.g., `on_init`)
- `define-dtor` in kernel.sxs specifies cleanup function name (e.g., `on_exit`)
- Both hooks receive only the API table: `void hook_fn(const sxs_api_table_t *api)`
- `on_init` called automatically after `kernel_init` completes
- `on_exit` called automatically before dylib is closed
- If hooks not found via `dlsym`, silently skipped (no error)
- Existing kernels without hooks continue to work unchanged

## C API Boundary

### Opaque Types
- `sxs_context_t` → `callable_context_if*`
- `sxs_object_t` → `slp_object_c*`
- `sxs_registry_t` → `registration_context_s*`

### API Table
- Registration: `register_function`
- Evaluation: `eval`
- Type operations: `get_type`, `as_int`, `as_real`, `as_string`, `as_symbol`, `as_list`
- List operations: `list_size`, `list_at`
- Creation: `create_int`, `create_real`, `create_string`, `create_symbol`, `create_none`
- List creation: `create_paren_list`, `create_bracket_list`, `create_brace_list`
- SOME operations: `some_has_value`, `some_get_value`

## Memory Management

- Args passed to kernel functions are borrowed references
- Return values created by kernel are owned by kernel
- Wrapper lambda copies data, deletes kernel-allocated object
- Lists from `as_list` are heap-allocated, kernel must manage
- Strings from `as_string` use thread_local buffer

## Object Creation Overhead

- `create_int/real/symbol` convert to string, parse back via `slp::parse()`
- `create_list` builds string representation, parses
- `create_string` uses direct creation without parsing
- Acknowledged performance trade-off for correctness

## Lock Behavior

- Kernels only loadable during initialization
- `kernels_locked_` flag prevents late loads
- Lock triggered by first non-datum instruction
- Shared with import lock mechanism

## Function Invocation

- Interpreter looks up `kernel_name/function_name` in registry
- Wrapper lambda invoked with context and args
- Kernel function receives opaque pointers
- Kernel extracts args via API, computes, creates result
- Wrapper copies result data back to interpreter

## Key Files

- `apps/pkg/core/kernels/kernels.cpp` - manager and API implementation
- `apps/pkg/core/kernels/kernels.hpp` - interfaces
- `libs/pkg/kernel_api.h` - C API header for kernel authors
- `apps/pkg/core/interpreter.cpp` - kernel function dispatch

