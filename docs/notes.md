

## Runtime Type Checking vs Build-Time Type Enforcement

### Current Approach (Runtime)
Lambda return types are validated at runtime. When a function returns a value that doesn't match its declared return type, the interpreter returns an ERROR object (`@(internal function error: returned unexpected type)`) instead of throwing an exception.

This "soft" error approach treats type mismatches like internal server errors in a distributed system - the error is returned as data rather than crashing the process.

### Future Approach (Build-Time)
During the build step in the `sup` project manager, we will implement a separate type-checking pass that:
- Validates all function return types match their declarations
- Ensures symbol resolution is correct
- Performs static type analysis before execution

This will allow us to:
1. Run a "VM" mode that executes pre-validated `.bin` files without safety checks
2. Catch type errors during development/build rather than runtime
3. Optimize execution by skipping redundant runtime checks

The runtime ERROR objects serve as a safety net during development and for dynamically-loaded code.

## build step sup

we have a build step in sup for kernels. we are most likely going to have special kernels that need building always.
once we get the core functions in (the ones that the core binary will always have available) implemented and interpreted
successfully we can make another "interpreter" like object that just does the type checking and symbol resolution during build
step. then, we could run a "vm" that takes the hydrated slp object raw from a .bin and executres it without all the safety checks.

The reason im not targeting this immediatly is that this is not meant to be "a language" so much as a "modular plug-and-play platform" 
centered around an entity (user) on a repl in a network/social setting.

Its a bit different in needs than a language, but for the sake of speed and performance one day I would LIKE to do this.