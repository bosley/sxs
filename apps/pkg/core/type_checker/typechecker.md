# Type Checker

Static type validation pass that runs before execution. Creates an isolated compiler context to walk the parsed instruction tree and validate types without executing code.

## How It Works

**Parse → Validate → Report**: The type checker parses source into SLP objects, creates a fresh compiler context with standard callable symbols, then calls `eval_type()` to recursively validate the entire tree.

**Delegation Model**: Each instruction (def, fn, if, try, etc.) provides its own `typecheck_function` that validates its specific semantics. The type checker just dispatches to these functions and manages the shared compiler context.

**Context State**: The compiler context maintains scoped symbol tables, lambda signatures, function signatures, and import tracking. Instructions use this context to define symbols, lookup types, push/pop scopes, and register lambdas.

## What Gets Validated

- Symbol resolution (undefined variables, duplicate definitions)
- Type matching (function arguments, branch consistency, return types)
- Scope correctness (local vs parent scope access)
- Circular import detection
- Control flow constraints (done outside loops, etc)
- Lambda signatures and function arities

Errors throw exceptions caught by `check_source()` and reported via logger.
