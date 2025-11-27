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
#(define-category .list [:list-p :list-c :list-b .dq_list])
```

so you see the parser is the source of truth. 

it's what delivers the types. eval is a subset of the "Creator" that can birth new objects, but the meta- the true source is the C++ slp parser. If it delivers upon us an error: fuck it, we ball. If it delivers upon us an integer: fuck it, we ball. 

fiwb is our "big bang" 

its how the atoms of the universe come to be.

We categorize them as things based on them being distinct alone.. indivisible, and then base all our type masturbation on top of that