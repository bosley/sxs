# Imports

## Architecture

- `imports_manager_c` owns import resolution and interpreter lifecycle
- `import_context_if` exposes operations to runtime
- Each import gets dedicated interpreter stored in core's map
- Interpreters persist for entire program lifetime

## Multi-Interpreter Model

- Main interpreter in `core_c::run()`
- Import interpreters created on-demand in `imports_manager_c`
- Stored in `core_c::import_interpreters_` map (symbol → interpreter)
- Each import has isolated scope, shared kernel/import contexts
- Import symbol becomes namespace prefix for cross-context calls

## Import Resolution

- Search order: absolute path → include_paths → working_directory
- Looks for file relative to each search path
- Canonicalize path for duplicate detection
- `resolve_file_path()` returns canonical path or empty string

## Import Loading Flow

1. Check if already imported (via canonical path set)
2. Detect circular imports (via `currently_importing_` set)
3. Parse import file as SLP
4. Create new interpreter with instruction symbols
5. Evaluate import file in isolated context
6. Store interpreter in map under import symbol
7. Register mutex for thread-safe access
8. Mark file as imported

## Circular Dependency Detection

- `currently_importing_` tracks active import chain
- `import_stack_` maintains full path for error reporting
- `import_guard_c` RAII adds/removes from tracking sets
- Cycle throws exception with full import chain

## Export Mechanism

- `export` instruction in imported file
- Defines symbol in import's local scope
- Registers in `current_exports_` map (unused currently)
- Symbols accessible via namespace: `imported_symbol/exported_name`

## Lifetime Management

- Import interpreters owned by `core_c::import_interpreters_`
- Destructors run at end of program execution
- No unloading or cleanup during runtime
- Lambdas in imports cleaned up with their defining scopes

## Lock Behavior

- Imports only allowed during initialization
- `imports_locked_` flag prevents late imports
- Lock triggered by first non-datum instruction
- Lock call propagates from main interpreter to manager

## Thread Safety

- `import_interpreter_locks_` provides per-import mutex
- Cross-context calls acquire shared lock
- Concurrent access to same import serialized
- Different imports can execute in parallel

## Key Files

- `apps/pkg/core/imports/imports.cpp` - manager implementation
- `apps/pkg/core/imports/imports.hpp` - interfaces
- `apps/pkg/core/core.cpp` - interpreter storage
- `apps/pkg/core/interpreter.cpp` - namespace resolution

