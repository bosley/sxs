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
(def x1 (forge/resize 'x0 0 20)) ; if it grows, default val is 0 (val (evaluated) - kernel sig is :symbol so we can pass easily despite type. on call it will unqote, then from the method we can load it)


```

also needed:

(forge/slp_int) ; takes in :any -> :int | throws error catchable by `recover` if fails
slp_str
slp_real
slp_list
..
sll slp types `:any` -> `:type_name` 


in addition to the type shenanigans and "resize" other list operrations:

```
forge/reverse (comes out an any, so use slp_type for type checker to undeta)
forge/
```