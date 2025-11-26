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



