When we load source we allocate a buffer containing the source. thats the instruction set.
Its allocated.

when scanning, if we accept base types UNTIL a marker then we can have a view into the instruction of a command:

```
    add r0 1 2;
    add r0 r0 r0;
```

First scan:

`add` `r0` `1` `1`

FN DEST LHS RHS

If we determine the full suite of instructions we need we can make a custom hash algorithm for the
symbol set and get a tight and performant lookup, or we can be monsters and not do lookups yet

```
    %0 r0 1 2; 
```

where `%` and `r` are the "instruction code" in ascii form and we switch on them, `%0` being "add" and `r0` being "register"
