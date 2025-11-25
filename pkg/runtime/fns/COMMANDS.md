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

1. **No Dynamic Allocation**: Everything lives in one contiguous `vector<uint8_t>` buffer
2. **Buffer Ownership Model**: The `view_` pointer in `slp_object_c` is lightweight (just points into the buffer), but when accessing list elements via `at()`, each child object gets its own copy of the full buffer and symbol table for safety and simplicity
3. **Offset Arrays**: Lists don't store children directly - they store an array of offsets pointing to where each child lives in the buffer
4. **Symbol Deduplication**: Symbols are referenced by ID, actual strings live in a separate map
5. **Self-Describing**: Each unit's header tells you what type it is (PAREN_LIST, SYMBOL, INTEGER, etc.)
6. **No Keywords**: "let" and "==" are just identifiers - the runtime gives them meaning

Moving `slp_object_c` is efficient (uses vector move semantics), but extracting children from lists copies the buffer. The architecture prioritizes safety and ownership clarity over minimal copying.