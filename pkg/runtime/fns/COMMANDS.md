# Commands for the runtime

These come in the form of slp lists. A SLP list is a homoiconic lisp-like string that has no notions of keywords, but instead, bases all input text as formed "types"
based on structure of the ascii data, making it the language AND the instruction. Since we have no key words its even easier to form slp lists into whatever we want here
at the runtime level.

Right now we have it setup suchthat the functions are broken into groups "core/kv" "core/event" "core/expr" and "core/util" These are the core concerns of the language now. Each command
is emitted to one of the defined "processors" that interpret the slp list.

The key part to note about slp lists is that they don't have any dynmaically allocated data. Each list '()' is fully contained set of data to describe itself. 
"DUH" you might think to yourself, slp lists have no keywords so they dont have any actions!

Yep, thats the thing. Now. In slp we have the notion of "identifiers" which the slp list correctly indicates they are, but the data of each individual piece and sub-piece
contained within the list is done so simply by pointing at ranges to the underlying data.

For instance in the string:

```
(let a (== 1 2))
```

This is made up syntax, as slp doesnt have a notion of "==" or "let" yet, these are just identifiers, so the slp object would look like so:

```
Memory Layout for: (let a (== 1 2))

┌─────────────────────────────────────────────────────────────────┐
│                    Contiguous Data Buffer                        │
└─────────────────────────────────────────────────────────────────┘

Offset 0x00: ┌──────────────────────┐
             │ slp_unit_of_store_t  │  ← Root object (outer list)
             ├──────────────────────┤
             │ header: PAREN_LIST   │
             │ flags:  3            │  (3 elements)
             │ data.uint64: 0x100   │  (→ offset array at 0x100)
             └──────────────────────┘

Offset 0x10: ┌──────────────────────┐
             │ slp_unit_of_store_t  │  ← Symbol "let"
             ├──────────────────────┤
             │ header: SYMBOL       │
             │ flags:  0            │
             │ data.uint64: 1       │  (→ symbol_table[1] = "let")
             └──────────────────────┘

Offset 0x20: ┌──────────────────────┐
             │ slp_unit_of_store_t  │  ← Symbol "a"
             ├──────────────────────┤
             │ header: SYMBOL       │
             │ flags:  0            │
             │ data.uint64: 2       │  (→ symbol_table[2] = "a")
             └──────────────────────┘

Offset 0x30: ┌──────────────────────┐
             │ slp_unit_of_store_t  │  ← Inner list (== 1 2)
             ├──────────────────────┤
             │ header: PAREN_LIST   │
             │ flags:  3            │  (3 elements)
             │ data.uint64: 0x118   │  (→ offset array at 0x118)
             └──────────────────────┘

Offset 0x40: ┌──────────────────────┐
             │ slp_unit_of_store_t  │  ← Symbol "=="
             ├──────────────────────┤
             │ header: SYMBOL       │
             │ flags:  0            │
             │ data.uint64: 3       │  (→ symbol_table[3] = "==")
             └──────────────────────┘

Offset 0x50: ┌──────────────────────┐
             │ slp_unit_of_store_t  │  ← Integer 1
             ├──────────────────────┤
             │ header: INTEGER      │
             │ flags:  0            │
             │ data.int64: 1        │
             └──────────────────────┘

Offset 0x60: ┌──────────────────────┐
             │ slp_unit_of_store_t  │  ← Integer 2
             ├──────────────────────┤
             │ header: INTEGER      │
             │ flags:  0            │
             │ data.int64: 2        │
             └──────────────────────┘

─────────────────────────────────────────────────────────────────

Offset 0x100: ┌────────┬────────┬────────┐  ← Outer list element offsets
              │  0x10  │  0x20  │  0x30  │  (let, a, inner-list)
              └────────┴────────┴────────┘

Offset 0x118: ┌────────┬────────┬────────┐  ← Inner list element offsets
              │  0x40  │  0x50  │  0x60  │  (==, 1, 2)
              └────────┴────────┴────────┘

─────────────────────────────────────────────────────────────────

Symbol Table (separate map):
┌──────────┬──────────┐
│ ID       │ String   │
├──────────┼──────────┤
│ 1        │ "let"    │
│ 2        │ "a"      │
│ 3        │ "=="     │
└──────────┴──────────┘
```

**Key Points:**

1. **No Dynamic Allocation**: Everything lives in one contiguous buffer managed by `slp_buffer_c`
2. **Buffer Ownership Model**: The `view_` pointer in `slp_object_c` is lightweight (just points into the buffer), but when accessing list elements via `at()`, each child object gets its own copy of the full buffer and symbol table for safety and simplicity
3. **Offset Arrays**: Lists don't store children directly - they store an array of offsets pointing to where each child lives in the buffer
4. **Symbol Deduplication**: Symbols are referenced by ID, actual strings live in a separate map
5. **Self-Describing**: Each unit's header tells you what type it is (PAREN_LIST, SYMBOL, INTEGER, etc.)
6. **No Keywords**: "let" and "==" are just identifiers - the runtime gives them meaning

Moving `slp_object_c` is efficient (transfers buffer ownership), but extracting children from lists copies the buffer. The architecture prioritizes safety and ownership clarity over minimal copying.

## Core Command Reference

Most commands return `true` (boolean symbol) on success or an ERROR type `@"message"` on failure. Some commands return other types as documented. Parameters are positional starting at index 1 (index 0 is the command itself).

### Symbol Semantics

**Important:** In the KV store, symbols represent explicit storage locations (like variables). You must explicitly use `core/kv/set` to store values and `core/kv/get` to retrieve them. There is no implicit value loading.

**Exception:** Symbols prefixed with `$` are context variables:

**Event Function Context Variables** (available only in event-related functions):
- `$CHANNEL_A` through `$CHANNEL_F` - evaluate to symbols `A` through `F`, used in core/event/pub and core/event/sub

**Handler-Injected Context Variables** (available only in specific handlers):
- `$key` - injected in `core/kv/iterate` handler bodies, contains the current iteration key as DQ_LIST (string). Can be used with `core/kv/load`, `core/kv/del`, and `core/kv/exists`.
- `$data` - injected in `core/event/sub` handler bodies, contains the event payload with type matching the subscription's `expected_type` parameter

These context variables are provided by the runtime and do not require explicit loading from the KV store.

**Context Variable Resolution:** Functions that accept context variables (those with `$` prefix) will automatically resolve them from the handler's context map. If a context variable is not found, an ERROR is returned.

### core/kv/set

Set a key-value pair in the session store.

**Signature:** `(core/kv/set key value)`

| Parameter | Type | Evaluated | Description |
|-----------|------|-----------|-------------|
| key | symbol | no | Key to set (literal symbol name) |
| value | any | yes | Value to store (evaluated, then converted to string) |

**Returns:** SYMBOL `true` on success, ERROR on failure

**Note:** The key parameter is used as a literal symbol name for storage. It is not evaluated, so using context variables like `$key` would create a key literally named "$key" rather than using the dynamic value. To work with dynamic keys from context variables, use `core/kv/load` within iteration handlers.

**Example:**

```
(core/kv/set mykey "hello world")
(core/kv/set counter 42)
(core/kv/set user:123 "active")
```

---

### core/kv/get

Retrieve a value by key from the session store.

**Signature:** `(core/kv/get key)`

| Parameter | Type | Evaluated | Description |
|-----------|------|-----------|-------------|
| key | symbol | no | Key to retrieve (static symbols only, not $ variables) |

**Returns:** DQ_LIST (string value) on success, ERROR if key not found or no permission

**Note:** This function requires a static symbol key and does NOT accept `$` context variables. For dynamic key access using `$` variables (e.g., `$key` in iteration handlers), use `core/kv/load` instead.

**Example:**

```
(core/kv/get mykey)
(core/kv/get user:123)
```

---

### core/kv/del

Delete a key from the session store.

**Signature:** `(core/kv/del key)`

| Parameter | Type | Evaluated | Description |
|-----------|------|-----------|-------------|
| key | symbol | no | Key to delete (literal symbol name or context variable) |

**Returns:** SYMBOL `true` on success, ERROR on failure

**Note:** The key parameter can be either a literal symbol name or a context variable (prefixed with `$`). When a `$` prefixed symbol is provided (e.g., `$key`), it will be resolved from the context. This enables dynamic key deletion during iteration.

**Example:**

```
(core/kv/del mykey)
(core/kv/del user:123)

(core/kv/iterate temp: 0 100 {
  (core/kv/del $key)
})
```

---

### core/kv/exists

Check if a key exists in the session store.

**Signature:** `(core/kv/exists key)`

| Parameter | Type | Evaluated | Description |
|-----------|------|-----------|-------------|
| key | symbol | no | Key to check (literal symbol name or context variable) |

**Returns:** SYMBOL `true` if exists, SYMBOL `false` if not, ERROR if context variable not found

**Note:** The key parameter can be either a literal symbol name or a context variable (prefixed with `$`). When a `$` prefixed symbol is provided (e.g., `$key`), it will be resolved from the context. This enables dynamic key existence checks during iteration.

**Example:**

```
(core/kv/exists mykey)
(core/kv/exists user:123)

(core/kv/iterate cache: 0 50 {
  (core/kv/exists $key)
})
```

---

### core/kv/snx

Set a key-value pair only if the key does not already exist (atomic operation).

**Signature:** `(core/kv/snx key value)`

| Parameter | Type | Evaluated | Description |
|-----------|------|-----------|-------------|
| key | symbol | no | Key to set (literal symbol name) |
| value | any | yes | Value to store if key doesn't exist |

**Returns:** SYMBOL `true` if set successfully, SYMBOL `false` if key already exists, ERROR on invalid parameters or missing store

**Note:** Like `core/kv/set`, the key parameter is a literal symbol name and is not evaluated. Context variables like `$key` cannot be used directly with this function.

**Example:**

```
(core/kv/snx lock:resource1 "acquired")
(core/kv/snx counter 0)
```

---

### core/kv/cas

Compare and swap: atomically set a new value only if the current value matches the expected value.

**Signature:** `(core/kv/cas key expected_value new_value)`

| Parameter | Type | Evaluated | Description |
|-----------|------|-----------|-------------|
| key | symbol | no | Key to update (literal symbol name) |
| expected_value | any | yes | Expected current value (evaluated) |
| new_value | any | yes | New value to set if expectation matches |

**Returns:** SYMBOL `true` if swap succeeded, SYMBOL `false` if current value doesn't match expected, ERROR on invalid parameters or missing store

**Note:** The key parameter is a literal symbol name and is not evaluated. Context variables like `$key` cannot be used directly with this function.

**Example:**

```
(core/kv/cas counter "5" "6")
(core/kv/cas status "pending" "active")
```

---

### core/kv/iterate

Iterate over keys matching a prefix, executing a handler for each key. Injects `$key` variable into handler context.

**Signature:** `(core/kv/iterate prefix offset limit handler_body)`

| Parameter | Type | Evaluated | Description |
|-----------|------|-----------|-------------|
| prefix | symbol | no | Key prefix to match (symbol literal) |
| offset | integer | no | Number of matching keys to skip |
| limit | integer | no | Maximum number of keys to process |
| handler_body | brace-list | no | Handler code block (must be `{...}`) |

**Context Variables:** `$key` (DQ_LIST type) is injected with the current key as a string for each iteration

**Returns:** SYMBOL `true` when iteration completes, ERROR on invalid parameters

**Example:**

```
(core/kv/iterate user: 0 10 {
  (core/util/log "Found key:" $key)
  (core/kv/del $key)
})
```

**Design Note:** The prefix parameter must be a symbol literal (e.g., `user:`). Dynamic prefix iteration is NOT supported by design - at this level, we are concerned with loading specific variable objects via `core/kv/load`, not iterating over dynamic ranges. If you need to work with keys from a dynamic prefix, store the prefix as a symbol and reference it directly.

Within the handler, `$key` contains the current key as a string and can be used with `core/kv/load` (to retrieve values), `core/kv/del` (to delete keys), or `core/kv/exists` (to check existence).

---

### core/kv/load

Retrieve and parse a value by key from the session store, returning it as a quoted SLP object. Requires the `$key` context variable.

**Signature:** `(core/kv/load key)`

| Parameter | Type | Evaluated | Description |
|-----------|------|-----------|-------------|
| key | symbol | no | Must be `$key` context variable specifically |

**Returns:** Quoted SLP object (SOME type) on success, ERROR if key not found or no permission

**Implementation Note:** The function has `can_return_error = false` in its type declaration but still returns ERROR at runtime. This is an inconsistency between the type system declaration and runtime behavior.

**Note:** Unlike `core/kv/get` which accepts static symbol keys and returns DQ_LIST (tainted string), `core/kv/load` ONLY accepts the `$key` context variable (injected by `core/kv/iterate` handlers) and returns a SOME type (quoted value). The returned value is created by parsing `'` + value, which quotes the stored string. Use `core/expr/eval` to evaluate the loaded SOME value if needed.

**Example:**

```
(core/kv/iterate user: 0 10 {
  (core/util/log "Loading value for key:" $key)
  (core/kv/load $key)
})
```

---

### core/event/pub

Publish an event to a specified channel and topic.

**Signature:** `(core/event/pub channel topic_id data)`

| Parameter | Type | Evaluated | Description |
|-----------|------|-----------|-------------|
| channel | symbol | yes | Channel identifier: `$CHANNEL_A` through `$CHANNEL_F` |
| topic_id | integer | no | Topic identifier (uint16) |
| data | any | yes | Event payload (evaluated, converted to string) |

**Returns:** SYMBOL `true` on success, ERROR on failure (rate limit, permission, etc.)

**Example:**

```
(core/event/pub $CHANNEL_A 100 "notification message")
(core/event/pub $CHANNEL_B 200 42)
```

---

### core/event/sub

Subscribe to events on a channel and topic with type filtering. Only events matching the expected type will trigger the handler.

**Signature:** `(core/event/sub channel topic_id expected_type handler_body)`

| Parameter | Type | Evaluated | Description |
|-----------|------|-----------|-------------|
| channel | symbol | yes | Channel identifier: `$CHANNEL_A` through `$CHANNEL_F` |
| topic_id | integer | no | Topic identifier (uint16) |
| expected_type | symbol | no | Expected data type: `:int`, `:real`, `:str`, `:some`, `:none`, `:error`, `:symbol`, `:list-p`, `:list-s`, `:list-c`, `:rune` |
| handler_body | brace-list | no | Handler code block (must be `{...}`) |

**Context Variables:** `$data` (type matches expected_type) is injected with event payload for each matching event

**Implementation Note:** The function has `handler_context_vars = {}` (empty) in its type declaration but still injects `$data` at runtime. This is an inconsistency between the type system declaration and runtime behavior.

**Type Filtering:** The runtime checks the published event data type against `expected_type`. Only events with matching types invoke the handler. This enables type-safe event handling and allows multiple handlers on the same topic with different type expectations.

**Type Filtering Behavior:**

The runtime enforces strict type matching between published events and subscription handlers:

1. When an event is published, its data payload is serialized to a string
2. When received by subscribers, the payload is parsed back into an SLP object
3. The parsed object's type is compared against the handler's `expected_type`
4. Only handlers with exactly matching types are invoked
5. If parsing fails or types don't match, the handler is skipped (no implicit conversions)

This allows multiple handlers on the same channel/topic to receive different types of events safely:
- A `:str` handler only receives string-typed events
- An `:int` handler only receives integer-typed events  
- Published integers will NOT be auto-converted to strings for `:str` handlers
- Unparseable data will NOT trigger any handlers

Type safety is enforced at both compile-time (via the type checker) and runtime (via strict type matching).

**Returns:** SYMBOL `true` on successful subscription, ERROR on failure

**Example:**

```
(core/event/sub $CHANNEL_A 100 :str {
  (core/util/log "String event:" $data)
  (core/kv/set last_message $data)
})

(core/event/sub $CHANNEL_A 100 :int {
  (core/util/log "Integer event:" $data)
  (core/kv/set counter $data)
})

(core/event/pub $CHANNEL_A 100 "hello")
(core/event/pub $CHANNEL_A 100 42)
```

---

### core/expr/eval

Evaluate SLP script text dynamically.

**Signature:** `(core/expr/eval script_text)`

| Parameter | Type | Evaluated | Description |
|-----------|------|-----------|-------------|
| script_text | any | yes | Script text to parse and evaluate (converted to string) |

**Returns:** Result of evaluating the script (any type), or ERROR on parse/eval failure

**Example:**

```
(core/expr/eval "42")
(core/expr/eval "(core/kv/get mykey)")
```

---

### core/util/log

Log one or more messages to the session logger.

**Signature:** `(core/util/log message...)`

| Parameter | Type | Evaluated | Description |
|-----------|------|-----------|-------------|
| message... | any (variadic) | yes | Messages to log (space-separated when multiple) |

**Returns:** SYMBOL `true` after logging, ERROR if no message provided

**Example:**

```
(core/util/log "Processing started")
(core/util/log "User" "Alice" "logged in" "at" 12345)
```

---

### core/util/insist

Detaint operator: execute a function call and ensure it did not return an error, halting execution if it did.

**Signature:** `(core/util/insist expr)`

| Parameter | Type | Evaluated | Description |
|-----------|------|-----------|-------------|
| expr | paren-list | no | Function call to execute (must be `(...)` form) |

**Returns:** The result of the function call (passes through unchanged if not an error)

**Implementation Note:** This function has `can_return_error = false` in its type declaration because it throws an exception instead of returning ERROR.

**Critical Design Constraint:** This function ONLY accepts unevaluated PAREN_LIST expressions (function calls in `(...)` form). It does NOT accept literals, symbols, or other types.

**Why This Constraint Exists:**

1. **Single Evaluation Semantics:** The parameter is unevaluated, and `insist` evaluates it exactly once. This prevents double evaluation issues and ensures correct semantics.

2. **Type Preservation:** The result is returned as-is. If a function returns `SOME` (quoted value), `insist` returns `SOME` unchanged - it does NOT unquote it. This is critical for preserving type semantics.

3. **Intent Clarity:** `insist` wraps fallible function calls to assert success. The pattern `(insist (function ...))` clearly expresses "call this function and ensure it didn't error."

4. **Type System Role (Detainter):** In the type system, `insist` is a detainter - it removes the taint from a type. If `(core/kv/get x)` returns `#DQ_LIST` (tainted), then `(insist (core/kv/get x))` returns `DQ_LIST` (pure). The type checker verifies the argument is tainted before allowing detaint.

5. **Runtime Role:** At runtime, if the function call returns ERROR, `insist` throws `insist_failure_exception`, terminating execution. If not ERROR, the result passes through unchanged.

**Example (Correct Usage):**

```
(core/util/insist (core/kv/get required_key))
(core/util/insist (core/kv/cas counter "5" "6"))
(core/util/insist (core/kv/load $key))
```

**Invalid Usage (Will Fail Type Checking):**

```
(core/util/insist x)
(core/util/insist 42)
(core/util/insist "hello")
```