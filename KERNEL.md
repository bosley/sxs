# SXS Kernel System

## What are Kernels?

Kernels are dynamically-loaded native extensions that extend the SXS runtime with additional functionality. They are compiled as shared libraries (dylibs on macOS, .so files on Linux) and loaded at runtime, allowing you to write performance-critical or system-level code in C++ while maintaining a clean interface with the SXS interpreter.

Kernels provide a plugin-style architecture where external developers can create extensions without needing to link against the entire SXS core library.

## Architecture Overview

### C ABI Interface

Kernels use a C-based Application Binary Interface (ABI) to avoid C++ ABI compatibility issues and minimize dependencies. The kernel only needs a single header file (`kernel_api.h`) and doesn't need to link against the SXS core at compile time.

Key components:

1. **Opaque Pointers**: Context and object types are passed as void pointers, hiding implementation details
2. **Function Registration**: Kernels register their functions through a callback mechanism
3. **API Table**: The runtime provides an API table with functions to interact with SXS objects
4. **Dynamic Loading**: Kernels are loaded via `dlopen()` at runtime when requested

### Loading Flow

1. SXS script calls `#(load "kernel_name")`
2. Runtime searches for `kernel_name/kernel.sxs` in include paths
3. Runtime parses `kernel.sxs` to get the dylib filename
4. Runtime loads the specified dylib from the same directory and calls `kernel_init()`
5. Kernel registers its functions via the registration callback
6. Functions become available as `kernel_name/function_name` in the script

## Building a Kernel

### Required Files

A kernel consists of three main files:

1. **kernel.sxs** - Declaration file defining the kernel's interface
2. **implementation.cpp** - C++ implementation of kernel functions
3. **Makefile** - Build instructions for the shared library

### Kernel Declaration (kernel.sxs)

The `kernel.sxs` file declares the kernel, its dylib filename, and its functions:

```lisp
#(define-kernel io "libkernel_io.dylib" [
    (define-function put (format :str obj :any..) :int)
])
```

The second parameter is the dylib filename that will be loaded from the same directory. This allows you to name your dylib whatever you want.

### Implementation Structure

Include the kernel API header:

```cpp
#include "core/kernels/kernel_api.h"
#include "slp/slp.hpp"
```

Store the API table globally:

```cpp
static const struct sxs_api_table_t *g_api = nullptr;
```

Implement your kernel functions:

```cpp
static sxs_object_t my_function(sxs_context_t ctx, sxs_object_t args) {
    void *list = g_api->as_list(args);
    size_t size = g_api->list_size(list);
    
    if (size < 2) {
        return g_api->create_int(-1);
    }
    
    sxs_object_t arg = g_api->list_at(list, 1);
    sxs_object_t evaled = g_api->eval(ctx, arg);
    sxs_type_t type = g_api->get_type(evaled);
    
    if (type == SXS_TYPE_INT) {
        long long val = g_api->as_int(evaled);
        return g_api->create_int(val * 2);
    }
    
    return g_api->create_int(0);
}
```

Implement the kernel_init function:

```cpp
extern "C" void kernel_init(sxs_registry_t registry,
                            const struct sxs_api_table_t *api) {
    g_api = api;
    
    // Register functions: (registry, name, function_ptr, return_type, variadic)
    api->register_function(registry, "my_function", my_function, SXS_TYPE_INT, 0);
}
```

### Type Constants

Use the `sxs_type_t` enum constants from `kernel_api.h`:

- `SXS_TYPE_NONE` = 0
- `SXS_TYPE_SOME` = 1
- `SXS_TYPE_PAREN_LIST` = 2
- `SXS_TYPE_BRACE_LIST` = 4
- `SXS_TYPE_STRING` = 5
- `SXS_TYPE_SYMBOL` = 7
- `SXS_TYPE_RUNE` = 8
- `SXS_TYPE_INT` = 9
- `SXS_TYPE_REAL` = 10
- `SXS_TYPE_BRACKET_LIST` = 11
- `SXS_TYPE_ERROR` = 12
- `SXS_TYPE_DATUM` = 13
- `SXS_TYPE_ABERRANT` = 14

### Makefile Template

```makefile
CXX = clang++
CXXFLAGS = -std=c++20 -fPIC -I../../pkg
LDFLAGS = -shared

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    TARGET = mykernel.dylib
    LDFLAGS += -dynamiclib
else
    TARGET = mykernel.so
endif

all: $(TARGET)

$(TARGET): mykernel.cpp mykernel.hpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ mykernel.cpp

clean:
	rm -f $(TARGET)

.PHONY: all clean
```

Name your dylib however you like - just make sure it matches what you specify in `kernel.sxs`.

### Build Commands

```bash
cd kernels/mykernel
make
```

This produces `libkernel_mykernel.dylib` (or `.so`) in the same directory.

## Example: IO Kernel

The `io` kernel provides formatted output functionality.

### Directory Structure

```
kernels/io/
├── kernel.sxs
├── io.cpp
├── io.hpp
├── Makefile
└── libkernel_io.dylib
```

### kernel.sxs

```lisp
#(define-kernel io "libkernel_io.dylib" [
    (define-function put (format :str obj :any..) :int)
])
```

### io.cpp (simplified)

```cpp
#include "io.hpp"

static const struct sxs_api_table_t *g_api = nullptr;

static sxs_object_t io_put(sxs_context_t ctx, sxs_object_t args) {
    void *list = g_api->as_list(args);
    size_t size = g_api->list_size(list);
    
    // Get format string (arg 1)
    sxs_object_t format_obj = g_api->list_at(list, 1);
    sxs_object_t evaled = g_api->eval(ctx, format_obj);
    const char *format_str = g_api->as_string(evaled);
    
    // Process remaining arguments and format output
    // ...
    
    printf("%s", output.c_str());
    
    // Cleanup and return
    return g_api->create_int(bytes_written);
}

extern "C" void kernel_init(sxs_registry_t registry,
                            const struct sxs_api_table_t *api) {
    g_api = api;
    api->register_function(registry, "put", io_put, 4, 1);
}
```

## Using Kernels in SXS Scripts

### Loading a Kernel

Kernels must be loaded at the start of a script, before any non-datum expressions:

```lisp
[
    #(load "io")
    
    ; Now io functions are available
    (io/put "Hello, World! %d" 42)
]
```

### Function Naming Convention

Kernel functions are accessed with the pattern: `kernel_name/function_name`

```lisp
(io/put "Value: %d" 123)
```

### Using Kernels in Imports

Kernels loaded in a main file are also available to imported files:

```lisp
; main.sxs
[
    #(load "io")
    #(import mylib "mylib.sxs")
    
    (mylib/some_function)
]

; mylib.sxs
[
    (export some_function (fn () :int [
        (io/put "Called from imported file")
        42
    ]))
]
```

## API Reference

### Registration Functions

- `register_function(registry, name, fn_ptr, return_type, variadic)` - Register a kernel function

### Object Functions

- `eval(ctx, obj)` - Evaluate an SXS object in the current context
- `get_type(obj)` - Get the type of an object (returns type enum value)
- `as_int(obj)` - Extract integer value from object
- `as_real(obj)` - Extract real (double) value from object
- `as_string(obj)` - Extract string value from object (returns const char*)
- `as_list(obj)` - Get list representation of object
- `list_size(list)` - Get size of a list
- `list_at(list, index)` - Get element at index in list

### Object Creation

- `create_int(value)` - Create an integer object
- `create_none()` - Create a none (null) object

## Memory Management

Kernel developers are responsible for cleaning up objects they create or retrieve from the API:

```cpp
sxs_object_t obj = g_api->list_at(list, 0);
// Use obj...
delete static_cast<slp::slp_object_c*>(obj);
```

Lists obtained via `as_list()` must also be deleted:

```cpp
void *list = g_api->as_list(args);
// Use list...
delete static_cast<slp::slp_object_c::list_c*>(list);
```

## Future Development

### Planned Tooling (TBD)

The following commands are planned for future implementation:

- `sxs kernel --new <name>` - Generate a new kernel template with all required files
- `sxs kernel --compile <name>` - Build a kernel with correct flags and validation

These tools will make kernel development even easier by handling boilerplate generation and compilation automatically.

### Validation

Future versions may validate that the dylib's registered functions match the declarations in `kernel.sxs` at load time.

## Best Practices

1. Always store the API table in a global variable
2. Clean up all objects you create or retrieve
3. Use thread_local storage for string buffers to avoid race conditions
4. Document your kernel's functions in kernel.sxs
5. Handle errors gracefully and return appropriate values
6. Test with small scripts before deploying in production
7. Version your kernels if you change the interface

