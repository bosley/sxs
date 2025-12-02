# Project

This project is a meta compiler/ interpreter (fyi)

Top level dir apps/ and libs/ are to be refactored as they are from an old legacy
seperation that no longer makes sense.

top level "wizard.sh" has --install --uninstall --reinstall that currently
builds everything and installs to `$SXS_HOME`. 


This is what the proper install looks like of our software:
```
 ~/workspace/sxs/ [tcs-rething] tree $SXS_HOME
/Users/bosley/.sxs
├── bin
│   ├── sup
│   └── sxs
├── include
│   └── sxs
│       ├── kernel_api.hpp
│       └── slp
│           ├── buffer.hpp
│           └── slp.hpp
└── lib
    ├── kernels
    │   ├── alu
    │   │   ├── kernel.sxs
    │   │   └── libkernel_alu.dylib
    │   ├── forge
    │   │   ├── kernel.sxs
    │   │   └── libkernel_forge.dylib
    │   ├── io
    │   │   ├── kernel.sxs
    │   │   └── libkernel_io.dylib
    │   ├── kv
    │   │   ├── kernel.sxs
    │   │   └── libkernel_kv.dylib
    │   ├── random
    │   │   ├── kernel.sxs
    │   │   └── libkernel_random.dylib
    │   └── sxs
    │       └── kernel.sxs
    └── libpkg_slp.a

13 directories, 17 files
```

### apps 

- cmd
    sup (executable)
    sxs (executable)

- pkg
    bytes (not yet used - planned to be used)
    core (currently in use primary product)

### libs

- pkg
    cache (made+tested planned use)
    kvds (in-use)
    random (in-use)
    slp (in-use)
    types (in-use)

These are each "kernels" that are individual items with make files
that depend on slp and others in pkg being installed. they are runtime dynamic
libraries that are read-in from sub and sxs
- std
    alu, forge, io, kv, random, sxs, etc


# Target Structure (code)


```

    sxs/ (app)
        ..

    sup/ (app)
        ..

    core/ (lib)
        imports
        instructions
        kernels
        type_checker
        ..
        ..

    root/ (first things to build and install to sxs_home for building project)
        kernel_api.hpp 
        slp/ (previously in libs/pkg/slp)

    integrants
        bytes
        cache
        kvds
        random
        types

    kernels
        alu
        forge
        io
        kv
        random
        sxs
        ..
```

When we build, this is the "big picture" and we need to refactor all cmake lists to conform to something sensible to help us configure wizard to do the following:

1. build slp and slp tests (cmake) 
2. install root contents to sxs home (see install example we must maintain same fs footprint)
3. build `core` library. the `core` library must be build as a dylib now. we must build it against the installed slp. we are to install the core library next-to slp in the sxs_home once built and tests run. ensure directory structure of core/ is maintained for header dirs and headers on isntall. 
4. Build integrants static library, ensure all tests for integrants run
5. iterate over all kernels and build their dylibs, installing into their sxs_home with their respective kernel.sxs files
6. build sxs & sup, install to sxs_home/bin
7. run all the sxs tests that we have that leverage the kernels for integration tests (wizard auto does this post-install)