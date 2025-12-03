# Example SXS Project

This is an SXS project with local kernels and dependencies.

This was made with `sxs project new example`

## Running

Use the project commands:

```bash
sxs project build
sxs project run
sxs project clean
```

## Why Not Run Directly?

Running `sxs init.sxs` directly will fail because the type checker and runtime won't find the local kernels in `./kernels/`. The project commands set up the correct include paths to the `.sxs-cache/kernels/` directory where built kernels are cached.

## Structure

```
example/
├── init.sxs              # Entry point
├── kernels/              # Local kernel sources
│   └── example/
│       ├── kernel.sxs    # Kernel metadata & type definitions
│       ├── example.cpp   # Kernel implementation
│       └── Makefile      # Build configuration
└── .sxs-cache/           # Built artifacts (auto-generated)
    └── kernels/
        └── example/
            ├── kernel.sxs
            └── libkernel_example.dylib
```

