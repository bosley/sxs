# TS (type system)

Hacing knowledge of types will be of paramount importance. 

Our implementation plan is this:

- map-out all 12 + 3 (new commands tbd) and find expected type pattern associativty
- create tests for existing 12 types where we feed our type system the knowledge-of (definition) of the implemented types (from fns, but not loaded into anything we wont test processing here) so we can be sure we are always using the most up tod ate defined types rather than having a duplicated set
- run the type system in-test and ensure all type classifications and flow-graphs come out as intended


There are some "conceptual functions" that type systems have that we need to explicitly define how/if we support them. and example of this is implicit type conversion functions, as example: if i take the following concept `1 + 3.14` as a human it makes sense its `4.14` but we implicitly promoted the `1` to be a `real`. This implicit conversion concept is the source of much dismay for many programmers.

Before we can begin explaining how we handle these hidden functions we need to name them, and know our full domain of types:

| Type Name | Example |
|-----------|---------|
| PAREN_LIST | `(something inside here)` |
| BRACKET_LIST | `[something inside here]` |
| BRACE_LIST | `{something inside here}` |
| DQ_LIST | `"hello world"` |
| SYMBOL | `core/kv/set` or `counter` |
| INTEGER | `42` or `-17` |
| REAL | `3.14159` |
| SOME | `'something` |
| ERROR | `@something` |
| RUNE | (single character, used internally) |
| NONE | (empty/null value) |

We can't drop the concept of `rune` even though we can't work with it explicitly it in-code yet except for when instantiating a DQ_LIST `"this is a dq list, aka string"` where each "char" is a "rune."

Note we also have an explicit "error type"

We need to consider the following now:

How do types come into existence? We implicitly say "theres a type here" when we see a raw datum and we KNOW a function, which does something, CAN return a type. So we have:

- Raw datum existence
- Result of function call (because we have a none-type we know a type will be present)
- variable - IS a _symbol_ 

Note: variables are symbol types in our type system because we don't "have" variables. We do explicit load and stores, just like you would in an assembly. The "symbol" type is a means to direct instructions for load/store and are otherwise "passed through" as if they were just an int or real to ensure that higher-level langauges on-top of this micro vm bytecode can leverage them as they see fit. This means makes things much simpler for us as we only have to track if we know we've loaded the thing or not, and if that load was a success of failure, as the load itself is just the result of a functionc all

So.. 


Function Schema:

```
parameters: a simple list (actual listing) of types that indicate what the function takes in, and how many

return type: what the function can return. many things can return an expected type OR an error
            if something COULD return an error we need to make it visually/categorically distinct
            as a "maybe error, maybe <type>" and then have a means of distilling it to 
            enforce the <type>.
```

given the level we ar eoperating at error resolution is simple. We state there is a magical command
taht when a "maybe" goes through it we ONLY EVER get the type. if it is an error, that magic function makes sure everything dies and the execution is explicitly DEAD. Again, this is because of the level we are at. higher level language conditionals for error checking will yield the code that we are currently checking. they will count on us failing to ensure they can work!

This means we should add an `ephemeral` type or an `abstract` type that the user doesn't explicitly define but rather implicitly exists by-way of "tainting" the object.

for instance:

fn (int int) [int | err] -> #int (a tainted int)

in order to know for a fact that we have an int, we need to remove the taint

magic-fn T :: T = int = (#T) T -> T

Its important to note that the way we define these taints and types is important as we wil be expanding the core set of commands to include a function generator (meaning it makes a custom slp byte set taht is interpreted as a function) and it will utilize symbols to resolve expected types in its definition 

## Type Taint Notation

We use the `#` prefix (octothorpe) to indicate a **tainted type** - a type that may be an ERROR instead of the expected type. This is a novel notation not used by other languages.

**Examples:**
- `#INTEGER` - may be INTEGER or ERROR
- `#DQ_LIST` - may be DQ_LIST or ERROR
- `INTEGER` - pure INTEGER (cannot be ERROR)

**Type Grammar:**
```
BaseType := PAREN_LIST | BRACKET_LIST | BRACE_LIST | DQ_LIST | SYMBOL | 
            INTEGER | REAL | SOME | ERROR | RUNE | NONE

PureType := BaseType
TaintedType := "#" BaseType
ReturnType := PureType | TaintedType

FunctionSig := "(" ParamList ")" "->" ReturnType
Param := ["$"] BaseType            
```

The `$` prefix on parameters indicates the parameter is **unevaluated** (passed as-is, not evaluated before the function receives it).

**Function Signature Examples:**
```
(core/kv/get $SYMBOL) -> #DQ_LIST
(core/kv/set $SYMBOL any) -> #INTEGER
(core/kv/exists $SYMBOL) -> INTEGER
(core/kv/cas $SYMBOL any any) -> INTEGER
(core/kv/iterate DQ_LIST INTEGER INTEGER BRACE_LIST) -> INTEGER
(core/event/pub $SYMBOL INTEGER any) -> #INTEGER
(core/event/sub $SYMBOL INTEGER BRACE_LIST) -> #INTEGER
(core/expr/eval any) -> #any
(core/expr/await any $SYMBOL INTEGER) -> #DQ_LIST
(core/util/log any...) -> INTEGER
```

## Detainting (Type Refinement)

To convert a tainted type to its pure form, there must be an explicit detaint operation. If the value is ERROR, execution terminates. This is the primitive that higher-level language error handling compiles down to.

```
detaint :: #T -> T
```

If `#T` is ERROR, execution dies. Otherwise, yields T.

## Key Concepts

- Types spawn from raw datum (literals in code)
- Types result from function calls (return types)
- Variables ARE symbols (no implicit load/store - explicit core/kv operations required)
- Tainted types (`#T`) represent "may be T or ERROR"
- Pure types cannot be ERROR
- Detaint operation is the primitive error handling mechanism

# IDEA

In order to make this system work with runtime but independent from, we will take in a slp ojbect, just like the processor, and take in a series of functiond efinitions, just like runtime does. however, instead of processing them, we map the symbol to type notation so we can determin what functions are detainters, which produce functions, etc. 

We need to also handle `[]` 