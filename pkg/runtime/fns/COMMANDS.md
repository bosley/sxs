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

All commands return `true` (boolean) on success or an ERROR type `@"message"` on failure. Parameters are positional starting at index 1 (index 0 is the command itself).

### Symbol Semantics

**Important:** In the KV store, symbols represent explicit storage locations (like variables). You must explicitly use `core/kv/set` to store values and `core/kv/get` to retrieve them. There is no implicit value loading.

**Exception:** Symbols prefixed with `$` are context variables:

**Event Function Context Variables** (available only in event-related functions):
- `$CHANNEL_A` through `$CHANNEL_F` - evaluate to symbols `A` through `F`, used in core/event/pub, core/event/sub, and core/expr/await

**Handler-Injected Context Variables** (available only in specific handlers):
- `$key` - injected in `core/kv/iterate` handler bodies, contains the current iteration key
- `$data` - injected in `core/event/sub` handler bodies, contains the event payload

These context variables are provided by the runtime and do not require explicit loading from the KV store.

### core/kv/set

Set a key-value pair in the session store.

**Signature:** `(core/kv/set key value)`

| Parameter | Type | Evaluated | Description |
|-----------|------|-----------|-------------|
| key | symbol | no | Key to set |
| value | any | yes | Value to store (evaluated, then converted to string) |

**Returns:** `true` on success, ERROR on failure

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
| key | symbol | no | Key to retrieve |

**Returns:** String value on success, ERROR if key not found or no permission

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
| key | symbol | no | Key to delete |

**Returns:** `true` on success, ERROR on failure

**Example:**

```
(core/kv/del mykey)
(core/kv/del user:123)
```

---

### core/kv/exists

Check if a key exists in the session store.

**Signature:** `(core/kv/exists key)`

| Parameter | Type | Evaluated | Description |
|-----------|------|-----------|-------------|
| key | symbol | no | Key to check |

**Returns:** `true` if exists, `false` if not

**Example:**

```
(core/kv/exists mykey)
(core/kv/exists user:123)
```

---

### core/kv/snx

Set a key-value pair only if the key does not already exist (atomic operation).

**Signature:** `(core/kv/snx key value)`

| Parameter | Type | Evaluated | Description |
|-----------|------|-----------|-------------|
| key | symbol | no | Key to set |
| value | any | yes | Value to store if key doesn't exist |

**Returns:** `true` if set successfully, `false` if key already exists

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
| key | symbol | no | Key to update |
| expected_value | any | yes | Expected current value (evaluated) |
| new_value | any | yes | New value to set if expectation matches |

**Returns:** `true` if swap succeeded, `false` if current value doesn't match expected

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
| prefix | symbol or string | yes | Key prefix to match (evaluated, accepts symbol or DQ_LIST) |
| offset | integer | no | Number of matching keys to skip |
| limit | integer | no | Maximum number of keys to process |
| handler_body | brace-list | no | Handler code block (must be `{...}`) |

**Context Variables:** `$key` is injected for each iteration

**Returns:** `true` when iteration completes

**Example:**

```
(core/kv/iterate "user:" 0 10 {
  (core/util/log "Found key:" $key)
  (core/kv/del $key)
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

**Returns:** `true` on success, ERROR on failure (rate limit, permission, etc.)

**Example:**

```
(core/event/pub $CHANNEL_A 100 "notification message")
(core/event/pub $CHANNEL_B 200 42)
```

---

### core/event/sub

Subscribe to events on a channel and topic, executing a handler for each event. Injects `$data` variable into handler context.

**Signature:** `(core/event/sub channel topic_id handler_body)`

| Parameter | Type | Evaluated | Description |
|-----------|------|-----------|-------------|
| channel | symbol | yes | Channel identifier: `$CHANNEL_A` through `$CHANNEL_F` |
| topic_id | integer | no | Topic identifier (uint16) |
| handler_body | brace-list | no | Handler code block (must be `{...}`) |

**Context Variables:** `$data` is injected with event payload for each event

**Returns:** `true` on successful subscription, ERROR on failure

**Example:**

```
(core/event/sub $CHANNEL_A 100 {
  (core/util/log "Received:" $data)
  (core/kv/set last_event $data)
})
```

---

### core/expr/eval

Evaluate SLP script text dynamically.

**Signature:** `(core/expr/eval script_text)`

| Parameter | Type | Evaluated | Description |
|-----------|------|-----------|-------------|
| script_text | any | yes | Script text to parse and evaluate |

**Returns:** Result of evaluating the script, or ERROR on parse/eval failure

**Example:**

```
(core/expr/eval "42")
(core/expr/eval "(core/kv/get mykey)")
```

---

### core/expr/await

Execute a body and wait for a response on a specified channel and topic (blocking with timeout).

**Signature:** `(core/expr/await body response_channel response_topic)`

| Parameter | Type | Evaluated | Description |
|-----------|------|-----------|-------------|
| body | any | yes | Expression to execute before waiting |
| response_channel | symbol | yes | Channel to await response: `$CHANNEL_A` through `$CHANNEL_F` |
| response_topic | integer | no | Topic identifier to await response on |

**Returns:** String containing the event data received, or ERROR on timeout/failure

**Example:**

```
(core/expr/await
  (core/event/pub $CHANNEL_A 1 "request")
  $CHANNEL_B
  100)
```

---

### core/util/log

Log one or more messages to the session logger.

**Signature:** `(core/util/log message...)`

| Parameter | Type | Evaluated | Description |
|-----------|------|-----------|-------------|
| message... | any (variadic) | yes | Messages to log (space-separated when multiple) |

**Returns:** `true` after logging

**Example:**

```
(core/util/log "Processing started")
(core/util/log "User" "Alice" "logged in" "at" 12345)
```