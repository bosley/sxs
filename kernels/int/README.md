A kernel is a "bounded orchestration meant to sample and observe a system's substate.

A function is an aspect of a kernel that takes in samples from the system.
A function will always produce 1 unit of sampling (an object) as a result of observing the samples given to it.

Our core kernel is built into the runtime. This core kernel is what will load all of the supporting kernels at initialization time. 

Each kernel is represented as a "function group" to the core so that each method present in the kernel may be leveraged.

All sample data will be of the SLP object form.

Each kernel will be as an "actor" in an actor model, and know of no state outside of itself except for by-way of another actor or input samples (function args)

The int kernel is the first present as it and the other primitives will be simple to create. We will create one kernel for every type present in the SLP type system so the system will have a means of efficiently creating and modifying types at runtime.

Since the SLP object packs together a bunch of binary data and then stores maps to symbols we can efficiently take in, modify and produce slp objects quickly in a way not able to be easily done naturally by such wide objects


Int defined as taking "some" type and returning a "tainted int" as its possible
that the conversion failed, and the result will need to be checked.

JOSH: Note how if we use `?` we dedicate it as a specific category "Aberrant" rather
than having it be a symbol prefix. If we do it this way then we can be rid of "insist"
and have the processor perform the exception throwing automatically after call.
We can still use aberrants elsewhere because the whole thing is going to be behind
the "#" so the ? presence will hit the type analyzer first which can then produce
the ACTUAL insist call, meaning "?" outside of a previous "#" can still be used elsewhere!

These objects wont be able to discern any symbols that they do not anage internally. 
Meaning that they wont be resolving anything directly (yet)

`#([int/from_bits :some] ?int)` 

```
(int/from_bits '1000_0111) 

; define the int type (we might need this to bootstrap type system)
#(type int .numeric [int/to_int])   ; .numeric special category meaning float or int in ts 

; Declare the bool type as derived from int, offer definition
; to call to cast int to boolean - will be exported
; and usable by user as :    [int/to_boolean :int] :bool
; but will be auto used by compiler to cast int to bool
#(type bool .int [int/to_boolean :int])

#([int/add :int :int] :int)
#([int/sub :int :int] :int)
#([int/div :int :int] :int)
#([int/mod :int :int] :int)
#([int/mul :int :int] :int)

#([int/gt :int :int] :bool)
#([int/lt :int :int] :bool) 
```