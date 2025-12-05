# SXS

This library does a couple of things.

1) Defines callbacks for managing complex `SLP` types (required for SLP parsing)
2) Defines callbacks for `SLP` parsing regarding when specific objects are found (virtual list, lists, and primitive object definitions)
3) Reads in a file with a given runtime, leverages `SLP` buffer parsing to execute a minimal s-expression adjacent language

When we parse, if the first char found is not `(` we say we are in a "virtual list" meaning we just tack-on a '(' operationally. Then, if
a virtual list is whats being parsed, the last newline follwing the virtual list becomes the ')' operationally meaning that we get a callback saying that an instruction has been completed.

The SXS takes in these SLP callbacks as if they were commands. Each one doing something to the runtime to create objects and perform calls.

`(@ 0 0)` for instance 

`(` start a new context
`@` symbol found - is builtin so convert to builtin fn call, store in context list
`0` integer - push to context list
`0` integer - push to context list
`)` EOL - convert proc list into `slp object` list variant. clear the proc list. eval the object. push result to parent context or current if no parent given 

Each `()` is a context. sub `()` is a sub-context so if `()` has a parent, its result is copied to the parent callers context and added to their list as the result

# Commands

`@` - 3 variants 

1) get `(@ <int>)`  -> returns a copy of the value
2) set `(@ <int> <any>)` -> returns a copy of the value that was set
3) cas `(@ <int> <any> <any>)` -> returns 1 if updated, 0 therwise. CAS is done by `type` and then `deep equals` (byte comparison)