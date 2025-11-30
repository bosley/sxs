SLP objects are copy only views into binary blobs. This is to keep memory management simple.

Because of this we are unable to dynamically form the objects at runtime. its copy only.

Forge is how we transform types that would otherwise be unable to be transformed. 

For instance, say we have an int. An int is easy to change. +1 BOOm now its new.
This is because there is the guarantee (kinda) that the int width wont change.

strings and lists dont have this. updating a single char would be fine but we cant guarantee a STRING is only of size 1

So, given `'(0 0 0 0 0 0)` being fixed in memory, pushing one is impossible. so forge comes ine and offers meta instructions
to change things.

Strings `""` are represented in SLP as `DQ_LIST` because its just like `()` `[]` `{}` in form: `""` we just call it `:str` to make programmers feel safe.

All of forge revolves around lists with a couple of exceptions.

so, any given item can be given to forge commands `:any` and if the item is NOT a list, it will be turned into a list first
as that is a safe up-cast

--------

FIRST HALF

--------

```

(load "alu")

; define a new list; note the ' as its evaluated, so to make it a list we '
(def x0 '(1 2 3 4 5 6 7 8 9 0))

;runtime doesnt leet us redefine symbols, but doing this will, in practice yield the desired functionality
; source: trust me bro, im an engineer
;
; its important to do this because once we have forge im going to permit "set" to update things
; but i want this tested first and passing type checker. so for every def here pretend its a set 
;;
(def x1 (forge/resize x0 0 20)) ; if it grows, default val is 0 

When the forge hands a value back it will be of type `:any` use `cast` and supply your expected type
to validate it. if its the type you expect, it will be a no-op , and the type system will be satisfied

- forge/pf <target> <object> -> new list of same overall type with new object in it
- forge/pb <target> -> new list without the back (last element)
- forge/rf <target> -> remove front
- forge/rb <target> -> remove back

for removal, if its a removal on an empty list, return an empty list

this way bad types in yielding empty lists wont cause problems

- forge/lsh -> left-shift the list
- forge/rsh -> right-shift the list
- forge/rev -> reverse the list
- forge/count -> return length of list

- forge/visit takes an any, but its a nop unless the given item is a lambda
    the visit takes something like:
```
(forge/visit my_list (fn (val :any) :any [


    ; Return 0 means no error, continue visiting
    ; if the return is not an int, visiting stops and the non-int val is returned as the result
    ; if the val is integer, but is non-0 (neg or pos) visiting stops and that value is returned as result
    ; the only way visitng continues is if `0` is returned as the lambda value
    0
]))



--------

SECOND HALF: 

Once the above is fully implemented, we need to get the following upgraded which will entail the 
updating of the TCS system.

The TCS system is fine for now as-is but if we permit or otherwise describe and define the ability
of kernels to "inject" variables without explicitly defining them then we need a way to export those
type symbols at the kernel interface level (sxs file) and mark what command they are permitted "under"
so that TCS can track them dynamically. This is not a capability in the current TCS tracker.
Technically a kernel COULD still inject right now but that would lead to some wicked UB at runtime
given that we assume the kernel is doing exactly as it promised (in kernel.sxs) and nothing more.

--------

```

Since lists aren't mutable and things are strange, forge injects a command, available only in the body
of the itrator lambda:

```

(def some_condition 1)
(forge/visit my_list (fn (val :any) :any [

    (if some_condition [
        ($update 44) ; the special injected fucntion to update the value being visitied (creates a temprioaryu lambda tied to special symbol in interpreter with ephemeral context)
        0 ; continue
    ]
    1) ; if condition is false, we stop visiting

]))


```

Now, the main language doesnt have a loop mechanic. that is by design. we are doing that in the forge. 
I am thinking about this in terms of "each item is a unique object" and function calling is cloning them so we 
all have a unique copy. When we work with them we destroy them and make new replicas with modifications, etc.
Why am i saying this with loops? Because its rotating/tumbling these objects together to ultimately produce a new
object

```

;
; NOTE: We will need to upgrade kernel.sxs ingestion to indicate injected symbols that
; can be utilized in the context of a command under the kernel. 
; this way it can track type of injected vars, for inmmstance: 
;
;   define-injected do $complete :fn -> function that takes no args returns nothing (or :none)
;
;
;
(forge/do [

    (def x 0) ; the whole scope of do is dropped at the end, so defs can be done safely
    (def y 10) ;

    ; we dont mutate symbols in the core language so it would require a mutable symbol extension like kv
    ; (not depicted) to trigger the "do" special injected lambda:

    ; We dont need to identify the loop because the fn is made for this body of this specific iteration so its 
    ; triggering will hit the correct instance of the flag
    ($complete)
])

```


ideaing:

not to be implemented until like.. way later, but to help debugging and whatnot:


    ;   $iterations - integer ; the number of times the loop has fully looped
    ;   $do_debug   - lambda  ; prints a colorful mapping of all objects in the do scope and their values at the moment of invokation