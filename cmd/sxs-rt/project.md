# SXS RT 

Doesnt load/run scropts like sxs. it loads specifically constructed directories to setup and launch the main sxs-rt instance on some server. The sxs-rt kenels (TBD) will then
permit remote REPL sessions to join the server and interact in sandboxed ways. Think "interactive virtual pc" thats highly concurrent, customizable, and has chat baked in (all hype marketing
im just making a neat language with the concept that a "main app" is one that hosts a runtime in which any instance that has the main sxs-rt kernels will be able to connect and
compute with in user-defined ways)

Sxs is already installed on the platform:

```
/Users/bosley/.sxs
├── bin
│   └── sxs
├── include
│   └── sxs
│       ├── kernel_api.h
│       └── slp
│           ├── buffer.hpp
│           └── slp.hpp
└── lib
    ├── kernels
    │   ├── alu
    │   │   ├── kernel.sxs
    │   │   └── libkernel_alu.dylib
    │   ├── io
    │   │   ├── kernel.sxs
    │   │   └── libkernel_io.dylib
    │   ├── kv
    │   │   ├── kernel.sxs
    │   │   └── libkernel_kv.dylib
    │   └── random
    │       ├── kernel.sxs
    │       └── libkernel_random.dylib
    └── libpkg_slp.a

```

A Project structure is similar:
```

    project_name/
        |- init.sxs
        |- kernels
        |- modules
            mod_name/
                mod_name.sxs ; required
                some_mod_file.sxs
        |-  


```

Modules are directories of sxs files that logically group functionality
kernels are c++ extensions developed specifically for this project, and will
be auto loaded from project first, then installed kernels (include order) to ensure
that proiject level kernels overwrite system installed kernels at runtime

Modules are for import command

