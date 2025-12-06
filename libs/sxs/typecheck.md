# SXS Type Checking System

## Overview

The SXS type checking system performs static analysis of source code before runtime execution. It validates that all function calls match their declared form signatures and tracks the types of values stored in registers throughout the program.

## Architecture

The type checker operates as a **two-pass system**:

```
Source File
    ↓
[PASS 1: Type Check]
    ↓
Type Valid? ──No──→ Report Errors & Exit
    ↓ Yes
[PASS 2: Runtime Execution]
    ↓
Result
```

## Core Components

### 1. Typecheck Context

The typecheck context maintains state during the type checking pass:

- **Register Type Tracking**: An array of 8192 slots tracking the inferred type of each register
- **Error Accumulation**: Collects all type errors found (unlike runtime which stops at first error)
- **Context Stack**: Mirrors runtime's context hierarchy for nested list evaluation
- **Builtin Registry**: Reference to available commands and their form signatures

### 2. Form System Integration

The type checker leverages the existing form definitions declared in callable structures:

```
Callable Definition
├── Variants (overloads)
│   ├── Variant 1: (int) → any
│   ├── Variant 2: (int any) → any
│   └── Variant 3: (int any any) → any
└── Handler Function
```

Each variant specifies:
- **Parameter forms**: Expected types for each argument
- **Return form**: Type of value returned
- **Variadic support**: Whether parameters accept variable arguments

### 3. Generic Typecheck Function

A single generic function validates ALL builtins:

```
For each function call:
    1. Determine type of each argument (recursively)
    2. Find matching variant based on argument types
    3. If no match found → Type Error
    4. If match found → Success
    5. Track result type (for register operations)
```

## Type Checking Flow

### Expression Type Determination

```
Object Type Mapping:
    Integer Literal    → int
    Real Literal       → real
    Symbol             → symbol
    Quoted Expression  → some
    String List ""     → list-s
    Bracket List []    → list-b
    Curly List {}      → list-c
    Paren List ()      → Evaluate as function call
    Builtin/Lambda     → fn
    None               → none
```

### Function Call Validation

```
Given: (function arg1 arg2 ...)

Step 1: Identify Function
    ↓
Step 2: Determine Type of Each Argument
    arg1 → type1
    arg2 → type2
    ...
    ↓
Step 3: Find Matching Variant
    For each variant in function:
        Does (type1, type2, ...) match variant signature?
    ↓
Step 4: Result
    Match Found     → Success, track return type
    No Match Found  → Type Error with details
```

### Variant Matching Algorithm

```
For each variant:
    1. Check argument count
       - Exact match for non-variadic
       - At least N for variadic (N = param count)
    
    2. Check each argument type
       - Direct match: int matches int
       - Any match: any matches everything
       - Variadic match: int.. matches int (repeatedly)
    
    3. All arguments match?
       - Yes → Use this variant
       - No → Try next variant
```

## Register Type Tracking

The type checker tracks what types are stored in registers:

```
Initial State:
    register[0..8191] = unknown

After: (@ 10 42)
    register[10] = int

After: (@ 20 [1 2 3])
    register[20] = list-b

Later: (@ 10)
    Returns: int (from tracked type)
```

This enables:
- **Type inference**: Knowing what type a register holds
- **Error detection**: Catching type mismatches early
- **Better error messages**: "Expected int, got list-b from register 10"

## Variadic Form Handling

Variadic forms accept variable numbers of arguments:

```
Example: debug command
    Signature: (debug any..)
    Meaning: Accepts 1 or more arguments of any type

Valid calls:
    (debug 42)              ✓ 1 argument
    (debug 42 "hello")      ✓ 2 arguments
    (debug 1 2 3 4 5)       ✓ 5 arguments

Invalid calls:
    (debug)                 ✗ 0 arguments (needs at least 1)
```

Variadic matching:
- **Base type extraction**: `int..` → `int`
- **Repeated matching**: All extra arguments must match base type
- **Special case**: `any..` matches any type, any count

## Error Reporting

The type checker accumulates multiple errors:

```
Error Structure:
    ├── Position: Source location of error
    ├── Message: Human-readable description
    └── Context: What was expected vs received

Example Error:
    Position 347: No matching variant for argument types
    Expected: (@ int any) or (@ int any any)
    Received: (@ string int)
```

Benefits of error accumulation:
- Fix multiple issues at once
- See all type problems in one pass
- Better developer experience

## Callback System

The type checker uses SLP callbacks to process parsed code:

```
SLP Parser
    ↓
Callbacks:
    ├── on_object       → Process tokens (int, symbol, etc.)
    ├── on_list_start   → Create new context
    ├── on_list_end     → Validate function call
    ├── on_virtual_list → Handle top-level expressions
    └── on_error        → Record parse errors
```

Key differences from runtime callbacks:
- **No evaluation**: Only type analysis, no execution
- **No side effects**: Doesn't modify registers or state
- **Type tracking**: Builds type information instead of values

## Integration with Runtime

```
main()
    ↓
Initialize builtins
    ↓
Create registry
    ↓
┌─────────────────────┐
│ TYPECHECK PHASE     │
│ - Parse source      │
│ - Validate types    │
│ - Track registers   │
│ - Accumulate errors │
└─────────────────────┘
    ↓
Errors found? ──Yes──→ Print errors & Exit
    ↓ No
┌─────────────────────┐
│ RUNTIME PHASE       │
│ - Parse source      │
│ - Execute code      │
│ - Produce results   │
└─────────────────────┘
    ↓
Output result
```

## Advantages of This Design

### 1. Automatic Type Checking
New builtins automatically get type checking by declaring their form signatures. No additional implementation needed.

### 2. Single Source of Truth
Form definitions serve both runtime variant selection and type checking. Changes propagate automatically.

### 3. Extensibility
Adding new types or forms requires minimal changes:
- Add form type enum
- Update type mapping function
- Forms system handles the rest

### 4. Early Error Detection
Type errors caught before execution:
- Faster feedback loop
- No runtime crashes from type mismatches
- Better error messages with context

### 5. Zero Runtime Overhead
Type checking happens once, before execution. Runtime has no type checking overhead.

## Future Enhancements

Potential improvements to the type checker:

1. **Lambda Type Checking**: Validate lambda bodies before invocation
2. **Flow Analysis**: Track control flow for conditional type refinement
3. **Type Inference**: Infer types for more complex expressions
4. **Custom Forms**: User-defined composite types
5. **Exhaustiveness**: Verify all code paths return expected types

## Example Walkthrough

Given this program:
```
(@ 0 42)
(@ 1 [1 2 3])
(debug (@ 0))
```

Type checking proceeds:

**Line 1: `(@ 0 42)`**
- Function: `@` (load_store builtin)
- Arg 0: `0` → type `int`
- Arg 1: `42` → type `int`
- Matching variant: `(@ int any)` ✓
- Side effect: `register[0] = int`

**Line 2: `(@ 1 [1 2 3])`**
- Function: `@` (load_store builtin)
- Arg 0: `1` → type `int`
- Arg 1: `[1 2 3]` → type `list-b`
- Matching variant: `(@ int any)` ✓
- Side effect: `register[1] = list-b`

**Line 3: `(debug (@ 0))`**
- Inner call: `(@ 0)`
  - Function: `@` (load_store builtin)
  - Arg 0: `0` → type `int`
  - Matching variant: `(@ int)` ✓
  - Returns: `int` (from register[0])
- Outer call: `(debug <int>)`
  - Function: `debug` builtin
  - Arg 0: result of `(@ 0)` → type `int`
  - Matching variant: `(debug any..)` ✓

**Result**: All type checks pass ✓

## Summary

The SXS type checker provides static type safety through:
- Form-based type validation
- Register type tracking
- Variadic argument support
- Comprehensive error reporting
- Zero runtime overhead

It leverages the existing form system to automatically validate all function calls, catching type errors before execution begins.

