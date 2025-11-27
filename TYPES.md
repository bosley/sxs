We will declare types of functions that already exist in c++ in a `#` list to be processes separately from the actual instructions.

They will create a type table that is somehow attached to the context they operate in while "compiling" the scripts to a singular slp object.

The compiler can output an aberrant object.

Then, the runtime, if getting an aberrant on launch will know "this has some extra info to inform me about prechecked safety we can validate, and then disable the runtime type checking" if we are given something that's not aberrant, we can then run the type checker, get that information, and THEN proceed. 

How we do it now isn't important. What is important now is that we

1 - Fully define every builtin with a new type system to replace the flimsy one we removed

2 - Ensure they permit bootstrapping of the type system

3 - are dynamic and can be used to load "kernel" dynamic libraries from the install dir on the system (contain functions)

The goal is that we keep the ~12 core commands built into the runtime but ALL OTHERS are built in external "kernel" libraries in sauros language, see kernels/int

# SLP Types:

`integer`
`real`
`dq_list`
`none`
`some`
`paren_list`
`brace_list`
`symbol`
`rune`
`bracket_list`
`error`
`datum`
`aberrant`

From these types we have to derive everything. We can add types of categories. We define the categories 1-1 with the type on purpose. The type system at the start of the program doesn't have a notion of any actual types. Since these SLP types already exist and are structured we need to bootstrap them into the type system. We declare each existing type as a category of its own (self referential because it IS a thing that exists that we then derive types from)


Use `#` datum scoped instructions to trigger type system
`#()` - A list to be executed by the type system interpreter

The type system interpreter ensures the correct form for calling and construciton
of objects in the slp main domain (the processor context).

`#(define-type int .integer #slp)`

`#slp` means `type-system symbol -> slp` which states "this thing can be constructed by being parsed by the slp parser" 

Only valid slp ordered-substates can yield a slp type as they are mutually distinct in form.

So We follow by defining all at startup:

```

#(define-type int .integer #slp)
#(define-type real .real #slp)
#(define-type str .dq_list #slp) ; note we now make dq_list no longer called a list to make it more understandable to the programmer
#(define-type none .none #slp)
#(define-type some .some #slp)
#(define-type list-p .paren_list #slp)
#(define-type list-c .brace_list #slp)
#(define-type symbol .symbol #slp)
#(define-type rune .rune #slp)
#(define-type list-b .bracket_list #slp)
#(define-type error .error #slp)
#(define-type datum .datum #slp)
#(define-type aberrant .aberrant #slp)
```

Now we have defined types we can resolve in `#` scoped objects via symbols prefixed by their name `:int` `:str` `:none` and so on.

To extend the categories from where we start off at to make definitions easier:

```

; note we dont prefix the "." 
#(define-category numeric [:int :real]) ; Now we have .numeric available

; Here we add the self ref cat .dq_list as well to ensure
; :str is considered a list, and to demonstrate that categories
; can be used as well
#(define-category list [:list-p :list-c :list-b .dq_list])

#(define-category any [.numeric .list .error .datum .aberrant .none .some])

; Note: categories can NOT be used to decorate actual functions or types and are
; ONLY available in # lists so we make :any this way:
#(define-type any .any)
```

so you see the parser is the source of truth. 

it's what delivers the types. eval is a subset of the "Creator" that can birth new objects, but the meta- the true source is the C++ slp parser. If it delivers upon us an error: fuck it, we ball. If it delivers upon us an integer: fuck it, we ball. 

fiwb is our "big bang" 

its how the atoms of the universe come to be.

We categorize them as things based on them being distinct alone.. indivisible, and then base all our type masturbation on top of that

`Oddity:` A "rune" is not something that someone can specifically create. A rune exists only as a member of a dq list, when the dq list is being sub-divided or through explicit conversion-to (more below)

Now if we want to define a smaller "fake" type for lower level considerations (smaller than objects) we can do so, but knowing they will manifest in reality as slp objects. Consider:

```
#(define-type block .integer #zero) 
; We can define a seperate originator promising a specific integer, 
; bypassing need for parsing. We know this is a uint64_t so we can now
;  deribe bytes as a reference to a sub-index of a block if we have a 
; function to conveert an integer to a list! 
#()
```

We will do this LATER though, once we get the type system up to where the core commands are so we can start implementing kernels

I rest now wondering .. since we are doing the compiling step, nt interpreting ive made that decision.. we need to consider whats going on.


Wach top-level object that hits a processor MUST be compiled and type checked within its own boundary. If it needs to reference something external to itself it needs to define how to get them as described above, and then, it also needs to mark itself up so when it goes to get compiled it can be checked...

In core if we define a function it will need to be compiled (handlers too) and core commands are runtime focused. so perhaps all functions should be a # call? i suspect this beacuse the commands in core are meant to be run at runtime. if we make a function at that time we have to compile it. if we define functions in # alone then we say "create a function definition" and then whenever we call it we invoke from # so the resulting bytecode is injected.. making a sort of macro system on a bytecode you write by hand...

No. we keep it standard lambda. when we hit it runtime we compile that object to a small executable and probably store it but we can move it aroudn to. we willbuild it on top of aberrant type where the body is the raw data of the function information that the runtime needs to load the logic to execute 

```

(core/kv/set x (core/fn/new (arg1 :int arg2 :str arg3 :real) :any {
    (core/util/log "hi" arg1 arg2 arg3)
}))

; as a core command we dont want to "load" x because then we bring its type baggage
; we make the "type" required ":symbol" and let the core evaluate it from the store
; thats the difference from core kernel vs other kernels, is that they can implicitly
; resolve symbols set by kv. This means we can consider kv the interface for which
; the system ingests data to be utilized, and this instruction layer can operate on it
; through the core

(core/fn/call x 44 "hello" 3.14) ; type system should see this as valid and resolve this
                                 ; list's type as :any


```

From here we can see how we will load kernels, and how we could possible instruct
the program to, itself, build the c/c++ kernels itself, and link to it at runtime
to ensure that the things are up to date. part of our compile process COULD be to compile
all the dynamic libs we need into a dir with an SCONF script informing the runtime how to load
then


OR better. we do this and fuck off with the idea of dlls atm. Use the core commands as primitives to orchestrate the combilation of c/c++ code we generate from these commands, 
leave the type system out of this??? idk. i need to think about it.