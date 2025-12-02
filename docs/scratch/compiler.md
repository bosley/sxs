Right now the slp interpreter is working great. we have imports and kernels and I think its time
to start the compiler.

For clarification of what i mean by "compiler"

The SLP interpreter known as SXS is, and forever will be, a tree walking interpreter. This interpreter's purpose
is to compile a target. Each program a user writes is written for an interpreted environment (SXS) that may or may not describe how a program is to be compiled.

If the SXS script does NOT explain a program, no binary will be produced, and the language can be used as-is - an interpreted language thats dyanmic af.

Then, im going to add a "compiler" or "bin" or something "kernel" level instruction (written in c++) that will, itself, be the SLP compiler.

This way we can compile code in-language and write/save it to disk as we please. We can also inspect/manipulate the generated binaries. 

This will make SXS technically an interactive/configurable meta-compiler where we can make a kernel for a different "backend" and from the interactive (interpreted) language we can define/ build the target.

This is the reason why we "install" core to the host system - we will be using it in-kernel for compiling lists
at interpretation time.

We could

- make a vm backend
- make a llvm backend
- make a c backend

Essentially, core itself being exposed to the kernels will permit us a meta-development where as long as we can convert a structured SLP object -> binary we can compile anything


Here is a sxs script now:
```
; Brackests kick-off the execution of instructions (lists wont exec globally)
; this forces a single object to be produced by the file
[
    ; not required, but common, is loading of kernels or imports
    ; when the interpreter runs NOW is the only time we can import or load from external
    ; resources. the moment a non-datum list `#()` is hit in this root-bracket list the kernel
    ; and module systems are LOCKED
    #(load "io" "kv" "fs")
    #(import my_import "some_file.sxs")

    ; Once this instruction is hit, the resources are locked as its a non-datum list
    ; and since we are in the root bracket list this instruction will now be interpreted
    ; using the available symbols and resources
    (io/put "OI!")


]

```

I propose that we define a data layout using SLP (not sxs -  a fine distinction) that represents
"a program" and then any backend can "expect" that program. 

Then, we make a "bin" built-in command that takes in 1 object, aberrant and checks to ensure that
this "program" is fully and functioanlly defined.

- type checker inherently validates the program
- builtin takes-in program, validates it, and 
- builtin also takes in symbol which should direct to a function that takes in 
    what is required to compile the program, and returns a `{}` list where each member
    is a SLP object representing 8 bytes of the target source. The function will be responsible
    for packing the binary into the ints themselves.
    Then, we can use fs/write to dump the bin to disk

This way we can coordinate the compilation process and define the program in SLP+SXS, but offload
the particuar implementation to a c++ defined "kernel" 

This opens the door for:

    - a VM kernel that doubles as a compiler for itself. we could then add a (vm/run "/path/to/bin") that would run the "compiled" slp in the vm, giving the runtime interpreter the ability to compile at runtime and "potentially" optimize (for static non-shared resources of course - or if shared through a 3rd party LIKE KVS BABY) [note: kvs is a value store kernel that is backed by rocksdb. this means we could share the same key scope and have cross-talk between compiled and interpreted SXS because they all speak SLP anyway]


This dynamic system also means that if we had a runtime-shared eventing kernel that we could mix together:
    - interpreted slp, compiled slp in the same process, starting from compiled slp (if they use the lib) or starting from the interpreter (doesnt matter)
    - vm-compiled slp, compiled in a vm kernel we could make it also interpret in vm. if interpreted then it could interact with the running proces by sharing event store information OR through process scope in kvs
    - target-compiled slp, if we implement llvm or c/c++ backend where we transpose SLP to actual machine code then we could an interpreter to define/ create that compiled binary, and then from that binary we could link into sxs core installed libs to invke an interpreter and bind into the required existing kernels for its runtime work

    