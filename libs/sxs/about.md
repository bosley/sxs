# SXS Runtime

SXS is an s-expression adjacent language runtime built on top of SLP (S-expression List Processor). It provides a minimal, context-based execution environment for evaluating s-expression-like code with a persistent object storage system.

## System Architecture

SXS operates as a runtime that processes source code through the SLP parser, which generates callbacks for different syntactic elements. The runtime maintains:

- A context hierarchy for managing execution scope
- A persistent object storage array with 8192 slots
- An evaluation engine that processes parsed objects
- A type system based on forms for function argument validation

## Data Types

### Primitives

- **Integer**: Signed 64-bit integers
- **Real**: Double-precision floating-point numbers
- **Symbol**: Identifiers and names represented as byte sequences
- **None**: Represents absence of value

### Lists

- **Parentheses `()`**: Evaluated as function calls when encountered
- **Brackets `[]`**: Data structures, not evaluated
- **Curlies `{}`**: Data structures, not evaluated
- **Quoted Strings `""`**: String literals

### Quoted Expressions

The quote operator `'` prevents immediate evaluation:
- `'symbol` - Returns the symbol itself without evaluation
- `'(...)` - Returns the list structure without calling it as a function
- `'[...]` - Returns the bracket list without evaluation
- `'{...}` - Returns the curly list without evaluation

When a quoted expression is evaluated, the runtime processes its contents and returns the result.

### Functions

- **Builtin**: Native functions implemented in C, identified by special symbols
- **Lambda**: User-defined functions (implementation in progress)

### Special Types

- **Error**: First-class error objects that can be handled by the runtime

## Evaluation Model

### Expression Evaluation

The evaluation rules are:

1. **Parenthesized lists `()`** are evaluated as function calls
   - First element must be a callable (builtin or lambda)
   - Remaining elements are arguments
   - Arguments are evaluated before being passed to the function
   - Result is returned to the parent context

2. **Brackets `[]` and curlies `{}`** are data structures
   - Not evaluated as function calls
   - Returned as-is when encountered

3. **Primitives** (integers, reals) evaluate to themselves

4. **Symbols** evaluate to themselves (variable lookup not yet implemented)

5. **Quoted expressions** are evaluated by processing their contents

### Virtual Lists

Top-level expressions that don't start with `(` are wrapped in an implicit "virtual list":

- When the first non-whitespace character is not `(`, a virtual list begins
- The virtual list continues until a newline is encountered
- The newline acts as the closing `)` for evaluation purposes
- This allows writing expressions without explicit parentheses at the top level

Example:
```
@ 0 42
```
Is equivalent to:
```
(@ 0 42)
```

## Context System

### Execution Contexts

Each parenthesized expression creates a new evaluation context:

- Contexts have a unique ID assigned sequentially
- Each context maintains a processing list of up to 16 objects
- Contexts have a parent reference (except the root context)
- When a context completes, its result is passed to the parent

### Context Lifecycle

1. **List Start**: A new context is created and becomes the current context
2. **Object Processing**: Objects are added to the context's processing list
3. **List End**: The processing list is converted to a list object, evaluated, and the result is passed to the parent
4. **Context Cleanup**: The child context is freed after passing its result

### Object Storage

The runtime provides persistent storage separate from contexts:

- 8192 storage slots indexed from 0 to 8191
- Storage persists across context boundaries
- Accessed via the `@` builtin command
- Stores copies of objects (deep copy semantics)

## Built-in Commands

### `@` - Load/Store/Compare-And-Swap

The `@` symbol is the primary builtin for interacting with object storage. It has three variants based on argument count:

#### Get (1 argument)

Retrieves a value from storage.

Syntax: `(@ <index>)`

- `<index>`: Integer storage slot index (0-8191)
- Returns: A copy of the stored object, or `none` if the slot is empty
- Errors: If index is out of bounds

#### Set (2 arguments)

Stores a value in storage.

Syntax: `(@ <index> <value>)`

- `<index>`: Integer storage slot index (0-8191)
- `<value>`: Any object to store
- Returns: A copy of the value that was stored
- Errors: If index is out of bounds

The value is evaluated before being stored.

#### Compare-And-Swap (3 arguments)

Atomically compares and swaps a value if it matches.

Syntax: `(@ <index> <compare> <new>)`

- `<index>`: Integer storage slot index (0-8191)
- `<compare>`: Value to compare against current slot contents
- `<new>`: New value to store if comparison succeeds
- Returns: Integer `1` if swap occurred, `0` if comparison failed
- Errors: If index is out of bounds

Comparison is done by type equality followed by deep byte comparison. Both `<compare>` and `<new>` are evaluated before the operation.

## Forms (Type System)

Forms define the expected types of function arguments. Functions can have multiple variants (overloading) with different form signatures.

### Base Forms

- `int` - Integer type
- `real` - Real number type
- `symbol` - Symbol type
- `list-s` - String list (quoted strings)
- `list-p` - Parenthesized list
- `list-b` - Bracket list
- `list-c` - Curly list
- `some` - Quoted expression
- `fn` - Function (builtin or lambda)
- `any` - Any type (no constraint)
- `none` - None type

### Variadic Forms

Forms with `..` suffix accept variable numbers of arguments:

- `int..` - Variable integers
- `real..` - Variable reals
- `symbol..` - Variable symbols
- `list-s..` - Variable string lists
- `list-p..` - Variable parenthesized lists
- `list-b..` - Variable bracket lists
- `list-c..` - Variable curly lists
- `some..` - Variable quoted expressions
- `fn..` - Variable functions
- `any..` - Variable any type

### Function Variants

The `@` builtin demonstrates multiple variants:

1. Variant 1: `(@ :int)` - Get operation
2. Variant 2: `(@ :int :any)` - Set operation
3. Variant 3: `(@ :int :any :any)` - CAS operation

The runtime selects the matching variant based on argument count and types after evaluation.

## Error Handling

Errors are first-class objects in SXS:

- Errors have a type, message, position, and source buffer reference
- When an error is created, the runtime sets an error flag
- Errors propagate through the evaluation chain
- Error objects can be returned and handled like any other value

### Error Types

- `UNCLOSED_GROUP` - Missing closing delimiter
- `UNCLOSED_QUOTED_GROUP` - Missing closing delimiter in quoted expression
- `PARSE_QUOTED_TOKEN` - Failed to parse quoted token
- `PARSE_TOKEN` - Failed to parse token
- `ALLOCATION` - Memory allocation failure
- `BUFFER_OPERATION` - Buffer operation failure

When an error occurs during parsing, the runtime clears the current context's processing list and creates an error object.

## Builtin Command Implementation

### Architecture

Builtin commands are implemented through a three-layer architecture:

1. **Handler Functions**: Individual implementation files in `impls/` directory containing the command logic
2. **Callable Definitions**: Type metadata structures that define function signatures, parameter forms, and variants
3. **Registry Integration**: A runtime registry that maps symbols to handler functions

The `sxs_command_impl_t` structure pairs a symbol string with its handler function, allowing the runtime to resolve symbols to implementations during evaluation.

### Current Implementations

The system includes three builtin commands:

**`@` (load_store)**: The primary storage interface with three variants based on argument count:
- Single argument: Get operation retrieving a value from storage
- Two arguments: Set operation storing a value
- Three arguments: Compare-and-swap atomic operation

**`d` (debug_simple)**: A variadic debugging command that accepts any number of arguments and prints simplified object representations showing type and basic value information.

**`D` (debug_full)**: A variadic debugging command that prints detailed recursive dumps of objects, including memory addresses, buffer contents in hex, and nested structure traversal.

### Implementation Pattern

Creating a new builtin command follows this pattern:

1. **Define Handler Function**: Create a function matching the signature `(runtime, callable, args, arg_count) -> object`. The handler receives unevaluated arguments.

2. **Evaluate Arguments**: Iterate through arguments, evaluate each one, and check for errors. Propagate any errors immediately.

3. **Type Checking**: Use the variant system to match evaluated arguments against form definitions. The runtime selects the appropriate variant based on argument count and types.

4. **Execute Logic**: Perform the command's operation and return a result object.

5. **Define Callable**: In `builtins.c`, create an initialization function that allocates a callable structure, defines variants with parameter forms, and links to the handler.

6. **Register Command**: Add the command to the registry with its symbol, and include initialization/deinitialization in the global init/deinit functions.

### Key Concepts

**Variants**: Commands can have multiple signatures (overloading). Each variant specifies parameter count and form constraints. The runtime matches arguments to variants after evaluation.

**Form-Based Typing**: The forms system defines expected types for parameters. Forms can be specific (int, real, symbol) or flexible (any, variadic forms with `..` suffix).

**Error Propagation**: Handler functions must check for evaluation errors and propagate them. Any command that can produce errors should return `FORM_TYPE_ANY` rather than a specific type.

**Argument Evaluation**: Handlers receive unevaluated arguments and must evaluate them explicitly. This allows commands to control evaluation order and handle special evaluation semantics.

## Processing Flow

1. **File Loading**: Source file is loaded into a buffer
2. **Parsing**: SLP scanner tokenizes and parses the buffer
3. **Callbacks**: Parser invokes callbacks for objects, list boundaries, and errors
4. **Context Management**: Runtime creates/destroys contexts as lists are encountered
5. **Evaluation**: Parenthesized lists are evaluated as function calls
6. **Result**: The last object in the root context is the final result

## Limitations and Future Work

- Variable binding and lookup not yet implemented
- Lambda function evaluation not yet implemented
- User-defined forms not yet implemented

