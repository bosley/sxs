

Next up:

Now that i have figured out the core commands to use on the system this is what i want to do:

Types. 


We only have 12 commands. We have only SLP types. We have the oddity that
we are inherently "Recurrent" meaning that a variable might have data backing it before it actually is defined (kv backed vars)

We need a means to type check an entire outer-list

We cant allow anything to be "get'd" unless its already been explicitly set.
This means also that we have to ensure a "set" and "setnx" didnt fail. 

if we consider this as follows

set -> (type of raw data in command | ERROR )

then, if we INSIST that the error is absolved before trusting the result of set (by presence of instruction) then we can infer that the value is of the type set.

This means we need to think of a new core command that can be used to give us this promise

```

(core/kv/set x 33) ; this means an error can be below it

;   If we add a top level core/insist we can assert "this will not be an error type"
;   as its presence will throw the runtime and bail the current execution explicitly
;   this is so higher level abstractions can still work with errors returned at this
;   level (individual execution) but we dont need to care for the sake of our type checking
(core/insist (core/kv/set 33))

; now we GUARANTEE x is of type "integer"
; however, since we have a disk-backed storage of x we actually can't work with it yet! 
; a permission change or constraint at runtime might toss us an error type on get!
;
;

; Good thing core/insist will pass-through the value of whatever it evaluates
; so anything that comes from a core/insist we can assume is of proper type
(core/insist (core/kv/get x))

```

The goal is to create a type-based filter to run the slp objects through before we touch it off to the event system to be
processed. If we do this under the hood in the eventer on that categeory then we can have the promise that only correctly
typed bytecode is being processed. We can then remove all conditional statements checking types in the hot-paths
that ar ethe fns groups that we are discussing.

I want to place a build macro somewhere in fns/macros.hpp so we can enable/disable the type-safe runtime conditional checking.
it will definitely be useful later for debugging. that meaning, in c++ have a macro that preserves the CURRENT flow in fns/
and when modified, removes all checking in the actual lambdas on paramters coming in. 

we will have the overhead of an extra instruction `insist` but we will have less potential branching and checking.
I don't know at what point that optimization would be noticeable but I know it will eventually.

The other goal is to ensure a level of type safety. 

- I might want to do categories of types as well like "numbers" for "ints" and "Reals" but we need to think of a way to express this in code. im considering ":symbols" that we then explicitly remove from the slp objects before processing that being just some identifier with a prefix like ":" "^" or "#" to indicate types in a way that we can extract from slp easily 

```
;   SLP does not recognize a function as an actual "type"
;   this is by design, as you cant, with ease of reading, define a function WITH type information
;   without some concept of actual syntax. 
;   For this reason we introduce "fn" at this level where we artificially construct an object at "runtime"
;   and tuck it behind a "bracket list". A "bracket list" is a [] that does not occur in the slp lists that
;   we have yet used or defined in any command. 

[ (x :int y :real z :list) :none   ; The types here help us not just break away from branching c++ instructions, but also
                                   ; inform the invoke command how much data, and what symbols to pre-load the modified slp object with
                                   ; note: modified as in architectually, not at runtime. slp objects are technically immutable once
                                   ; instantiated.

    ; Notice we dont have to load them! we reference them with `$` as they will be injected into the
    ; function instructions on invokation, pre-loaded from the db and pre-evaluated/copied to the function instruction
    (core/util/log $x $y $z)
]


```

We have limited, but slightly different than normal types:
```
    :number
        :int
        :real
    :some
    :none
    :any
    :list
    :str
    :sym
```

Since we don't actually process identifiers implicitly we can content with `:sym` type meaning
"its some contiguous symbol thats not in double quotes and doesnt mean anything else in-context"

Passing the "symbol" type through is very important at this stage. we are at a weird intermediate level
of the software. A weird sort of firmwhere where we aren't REALLY making functions and we aren't REALLY
using variables. We are using symbols to indicate where we should load from or not and then have extremly
primitive types on copy-only objects to coordinate system behavior. We can't even do math here.

If we can get these 12 + 2 (invoke/insist) and setup the runtime to take bracket lists and automatically
form them into a function, AND we make the pre-process type examination filter THEN we can safely
call this middle-firm-ware decent and build the more expressive language with higher forms of types and
type considerations, abstracting the need to store/load variables explicitly and having sane functions
that aren't literally loaded from a database at invokation time.

```

; note the "'" on the :list meaning the  arguments are going to be evaluated before invoking, as to ensure
; the data is resolved (if core get invoke etc) is done to load something in the arg as we dont have actual symbols!
; since the list is quoted by the time it actually is processed by f it is `(1 2 3)` as it would expect. this needs to be
; remembered when implementeing type system
;
(core/invoke (core/insist (core/kv/get f)) 42 3.14159 '(1 2 3))

```

That said, it might be a good idea to make "object registers" on the processor that we could load functions into.
working that into the type system would be simple and then we could forego the insist/get combo on loading functions
and simply do one `(core/r0 (core/insist (core/kv/get f))` and then add symbols `$r0` etc for: 
`(core/invoke $r0 42 3.14159 '(1 2 3))`

This will work great becasue each top-level line of slp is considered its own context, and ultimately the program itself
is a single slp object!

If each individual line sent to the processor is type valid, then all lines together will be type valid. 

A higher level language would result in a single SLP object that we would receive as "the program" that is then copied out of
and stored/used as needed by the runtime. 