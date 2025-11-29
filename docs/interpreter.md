# Interpreter

## Architecture

- `interpreter_c` implements `callable_context_if`
- Created via `create_interpreter()` factory function
- Manages symbol scopes, lambda definitions, type mappings
- Orchestrates eval flow for all SLP object types

## Evaluation Paths

### Datum Path
- Objects prefixed with `#` (DATUM type)
- Unwraps inner object from datum wrapper
- Routes to `datum::get_standard_callable_symbols()`
- Handles: `debug`, `import`, `load`

### Instruction Path  
- Regular paren lists without datum prefix
- Routes to `instructions::get_standard_callable_symbols()`
- Handles: `def`, `fn`, `debug`, `export`

### Type-Based Routing
- Literals (int, real, string, rune) → pass through
- Symbols → scope lookup or import namespace resolution
- Paren lists → callable dispatch
- Bracket lists → sequential evaluation, triggers import locks
- Aberrant → lambda call dispatch

## Symbol Scoping

- Stack of scope maps: `vector<map<string, slp_object_c>>`
- Lookup walks scopes in reverse order
- `def` always defines in local scope only
- Scope cleanup on pop removes associated lambdas

## Lambda Management

- Lambdas stored as ABERRANT type with uint64 ID
- `lambda_definitions_` maps ID to definition
- Definitions include: parameters, return type, body, scope level
- Lambda calls: push scope → bind params → eval body → pop scope
- Cleanup: lambdas removed when defining scope is popped

## Import Namespaces

- Symbols with `/` delimiter: `prefix/suffix`
- `get_import_interpreter(prefix)` retrieves stored interpreter
- Symbol access: parse suffix, eval in target interpreter
- Function calls: eval args locally, reconstruct call string, eval remotely
- Thread-safe via `shared_mutex` per import

## Cross-Context Mechanics

- `import_interpreters_` map stores all import contexts
- `import_interpreter_locks_` provides synchronization
- `call_in_import_context()` serializes args to SLP string, parses in target
- Return values copied back to caller context
- Each import has isolated scope but shares kernel context

## Lock Triggering

- Imports/kernels lockable after initialization phase
- First non-datum in bracket list triggers locks
- `imports_locks_triggered_` flag prevents double-lock
- Locks propagate to import and kernel contexts

## Type System

- `type_symbol_map_` maps `:int`, `:real`, `:str`, etc to `slp_type_e`
- Variadic suffix `..` supported (`:int..`)
- Type validation on lambda calls enforces param types
- Special handling: `:any` maps to NONE (accepts anything)

## Key Files

- `apps/pkg/core/interpreter.cpp` - core eval logic
- `apps/pkg/core/interpreter.hpp` - interfaces
- `apps/pkg/core/instructions/instructions.cpp` - instruction symbols
- `apps/pkg/core/datum/datum.cpp` - datum symbols

