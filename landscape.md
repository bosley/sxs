# Landscape 

It might be hard to see without having the intimate knowledge of the builder so I figure ill throw this down

This system is a meta-compiler compiler that makes the setup of the main compiler that compiles sxs code by interpreting it. We then take that compiled output, that leverages the interpreter at runtime, but itself is compiled, to run the software we are building.

Intended Stages:

0. Install libs/
1. Build/install apps
2. Use `sup` command to produce a "project". Sup's build WILL output a binary, one that will require the kernels used to produce it. 
3. Use the binary from "project" setup to create a compiler that transforms SLP -> binary and manages dylib linking 
3. Compile the sup project to a binary -> `stage0` is this binary. This binary can then be used to compile SLP directly to binary

Right now i am in the middle of making the system able to finish `2`. Type system is in place and im developing kernels that can
be used by the interpreted sxs AND by the compiled sxs in c++. Once I hit critical mass and have enough functionality in these
kernels I can use `sup` to create the project that will be the sxs `base compiler.` This `base compiler` is BUILT by the interpreter
that is written in C++ but itself is not interpreted despite the language looking exactly the same. 

-------------------

This is how an AI phrased the above when i asked it to make it more clear what I was doing.

```
# Landscape

Without intimate knowledge of the implementation, the architecture might be unclear. This document explains the bootstrapping strategy.

## Overview

**sxs** is a bootstrapped compiler system. We start with a C++ tree-walking interpreter that executes sxs (s-expression) source code directly. Through staged compilation, we eventually produce a self-hosting compiler that generates native binaries.

## Bootstrap Stages

**Stage 0: Foundation (Current)**
- Install kernel libraries (`libs/std/`, `libs/pkg/`)
- Build toolchain (`apps/`) including the C++ interpreter and `sup` build tool

**Stage 1: Ahead-of-Time Compilation with Runtime**
- Use `sup` to compile sxs projects to native binaries
- These binaries link against kernel shared libraries (dylibs) for runtime support
- The compiled code is no longer interpreted, but requires the kernel FFI at runtime
- This is similar to how languages like Python or Node.js have native compiled extensions

**Stage 2: SLP Backend**
- Build a compiler (using Stage 1) that transforms SLP (sxs serialized representation) directly to machine code
- Manages dynamic library linking without interpreter overhead
- This compiler is written in sxs, compiled by the C++ interpreter, but itself produces fully AOT-compiled output

**Stage 3: Self-Hosting**
- The resulting binary becomes `stage0` - a self-hosting sxs compiler
- Can recompile itself from sxs source
- No dependency on the original C++ interpreter

## Current Progress

Developing kernel primitives that work in both contexts:
- Callable as FFI from interpreted sxs code (via C++ interpreter)
- Linkable as native libraries for compiled binaries

Once sufficient kernel functionality exists (I/O, data structures, reflection, etc.), we can bootstrap the base compiler using `sup`. This compiler will be the first major sxs application that compiles to native code while being written entirely in sxs.
```

----

to be honest i dont know if i will ever want to fully self-host. i might at one point because its like some sort of achievement to some folks but I just want the system that is being compiled to binary (end stage 2) to build the software I have in mind




-----

Honestly kind of thinking about just making a kernel that is used to generate nodes in a code gen graph, then on completion having it dump the bin
Thus making it so that the SLP sxs interpreter process would be to build the code generation step. like for a backend. then, any frontend that
is transformed into SLP lists could then be compiled. like a shittier llvm, but better in that its mine and the code gen backend could be used
to easily target "virtual" enviornments (nabla, umbra etc) so you could quickly use sup to spin-up a program written in slp that takes in whatever
files(s) you throw at it and it generates bytecode for your backend so each sup project would be a backend whose compiled output would transform
slp->target bytes