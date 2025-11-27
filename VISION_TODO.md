pivot.

we use this to describe a virtual machine. the language is used to describe a virtual machine like nabla or others ive made. Then from the schematic this sxs program will read that in and combile an executable that is the virtual processor and also an executable that is the assembler for that processor. 

This means it would "make nabla" and "make solace" so then the programmer could write the assembly, compile it with their assembler and then run that resulting binary with the other binary this program produces











We have the COMMANDS and type stuff down enugh for this level i think.
We can validate types for any given block of code and can work


Now we need to set a define to enable/disable the type enforcement at eval

THe idea is this:

Anywhere we have a dehydrated object (something that needs to be parsed) we ALSO
run the ts object over it. If it passes parsing and type checking, then we should
be safe to disable physical type checks in hot paths of the fns/ 

I want this toggleable for debugging and as an option (the hot path part) but
the type enforcement should be 100% standard. Nothing should hit the processor
that hasn't been type checked AND hydrated (parsed.)

This is a key reason why we store objects in the kv store "raw" this means we can
trust the type and encoded data!

It may be advantageous to implement lambdas FIRST. 


we will need to add to the type list in helpers.hpp the type signatures for
tainted types

(type with # prefixed)

Regular, pure types:
```
:int
:real
:symbol
:str
:list-p
:list-c
:list-s
:some
:none
:error
```

Tainted types:
```
#int
#real
#symbol
#str
#list-p
#list-c
#list-s
#some
#none
#error            
```

    note: #error : we need this but what the fuck. (error, error) either you wanted an error or something unexpected happened while we got it for you. 

The operating philosophy here is that "types dont appear out of nowhere. A pure type
is one whose act of formation was checked. So for instance;

`{(core/kv/set x 3) (core/kv/get x)}` 

The get on this could have failed. Conceptually, if x was null (if this was a compiled language in the traditional sense), or in our case, if the
data store access fails, x is in a quasi state of "int or error." Given that our
operating plane is a "backend" for a higher level language we want a concept of "insistence."
Its not so much an assert as it is "a promise that an assertion has been made." Directionally similar to assert, but not the same. 

When we `core/util/insist` on a symbol what we are saying is "we insist that this WILL NOT be an error." In the current implementation when we do this we hoist the error up the C++ stack with an exception to the root-most runtime layer, bypassing any potential user-code implemented on the system meant to handle errors. Its like a panic, but one that nobody can handle. Once we have the type system in place and bolted down, we can remove this exception entirely in the configuration of having a "local" kv/store (we havent addressed local/remote concept in the repository yet but it is conceptually planned.) We can, and are targeting it being disabled post type-system implementation for the standard configuration. 

When we disable it that also means that the runtime will "evaporate" the insistence instructions on ingestion so not even the bytecode remains

    The promise an assertion has been made. 



```

; the core/new will be where we have instructions for making more complex types
; at our "runtime." Functions are just a body of instructions with some type data
; so we will comprise whaat we need to execute the function then when called we can
; run it
(core/kv/set x (core/rt/fn (a :int b :real c :str) :some {

}))

; however, due to system taints and all we need to invoke/load correctly
; We will make "core/rt/call" who can load things and call witha rgs
; but ehn we also need a way to define an item is a lambda even if we didnt 
; make it ourselves (for the type system)
;
; This is where [] can come in?
;
;  [x :some :int :real :str] ; meaning "function returning :some, taking ..." 
;
;   Seemingly ordered backwards but "return type" is always required
;
;
;  [x :some :..] ; variadic
;
;   These lists are to be entirely consumed by the type checker
;   this will instate the promises in-context that are required to resolve
;   the types
;
;
;


```

if we add to slp parser a thing similiar in category to ' and @, maybe "#" we can
make a new type of quote we can call the `tsq` or something meaning to be consumed
by the type system/ general "macro" like object

so `#(macro NAME ARGS..)` wuld be possible but most importantly

`#(extern x (:int :real :str) :some)`

Then we culd maintain parity with structure of definition and intent, also opening up
lots of pre-processor type commands which may ellucidate new pathways by-way of widening the conceptual scope as I implement it,.



We should diagram "the weird realm" somewhere to see this top-down

ealistically any "higher level type" should be able to be described to the runtime like fn, and others
so we can have a save representation and means to access them from the data store after construction/etc

if we create the "minimal object instruction set" then we could, in theory, black-box any custom slp object
directly from our own instructions

Say "i have this object "[]" and i call it "bob" and then say "in bob i have <types>" this is how i want
to get and set them, then our objects can be made on the go. if we do this all within "#" lists, then
that means we can do it on-the-fly but also before-hand and skip it for every single instruction.


Perhaps since we are leveraging a kv store uth and hare in the psuedo db realm we can call object blueprints
schemas and nobody will question?

```
; This stores the schema into the current entitie's local sconf as "an object the user likes to use"
; This is distinct from the traditional notion of "my program has a library with object x" its "part of my work
; flow incorperates data that i organize this way"

; Now we are in the hash list we can have a whle different suite of commands and not even recognize the `core/`
; commands. hell yeah

#(blueprint my_data_blueprint
    some_member_symbol :int 0
    some_other_symbol :str ""
)

; Now its part of the types!
#(blueprint my_other_blueprint
    some_member_symbol :my_data_blueprint
)

I think i will want these to be installed on the system under the root path on first run and then auto-loaded
into every runtime for compilation-sake. 

this way we can dynamically create all of the types we will need to work with

If we can find a means to "transact" against an object we can bypass load/taints/and runtime type checking
entirely if we consider it like this:

- given some known to exist; a valid symbol 
- create a queue to describe a set of operations to be done against the object
- provide potential fallback actions
- execute

Usually this is done on the db level or the k/v level but im talking about doing this on the
individual "v" of the "k/v" level where we have some value "v" NOT loaded into memory (still on disk)
that we can describe operations on, then, run the transaction: load the item into memory, perform operations, store back into memory.

This makes these SIMILAR to types and SIMILAR to a db schema but not at all the same.


```






