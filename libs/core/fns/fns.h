#ifndef SXS_COMPILER_FNS_H
#define SXS_COMPILER_FNS_H

/*
    Here we will define the structs and types to fully enscribe what it 
    means to be a function in this system, a runtime lambda or compiled.

    Here we will also add a function to export all builtins

    in the c file we will build a map of all fns def/def.h .. et al to build
    three maps per instruction.

    One map is for runtime tree-walking interpretation against the runtime
    One map is for compiling the the command to our target that will use the emitter
        in-context to emit instructions after validating all type data with the type system

    This way we can build out the core of the language and it can compile/interpret. this way
    when we build our final prouct, it can use itself as a library and interpret at runtime as well.
*/

#endif