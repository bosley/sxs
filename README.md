<div align="center">
  <img src="syntax/icons/sxs-icon-dark.svg" alt="SXS Logo" width="128"/>
  <br/>
  <h1><em>SXS (Side by side)</em></h1>
</div>

A dynamic extensible runtime based on the "simple list programs" slp. 

Right now this is basically a "build a bear" of diy lisp projects. With the main libs installed to build the apps you'll have the ability
to add any set of keywords to the sxs interpreter environment and extend the instruction set.

Im getting this ironed out with the goal of making a particular system, not a language, but its in such a meta state right now that its basically
just a primordial lisp.

## Prerequisites

- CMake 3.15+
- C++20 compatible compiler
- Git

### System Dependencies

Install the following dependencies on your system:

**macOS (Homebrew):**
```bash
brew install rocksdb spdlog fmt zstd snitch
```

**Ubuntu/Debian:**
```bash
sudo apt-get install librocksdb-dev libspdlog-dev libfmt-dev libzstd-dev libsnitch-dev
```

**Fedora/RHEL:**
```bash
sudo dnf install rocksdb-devel spdlog-devel fmt-devel libzstd-devel snitch-devel
```

## Installation

### 1. Clone the Repository

```bash
git clone <repository-url> sxs
cd sxs
```

### 2. Install Using Wizard

```bash
./wizard.sh --install
```

This will:
- Build the entire project (core runtime, kernels, and binary) from source
- Install the `sxs` binary to `~/.sxs/bin/`
- Install the SLP parser library to `~/.sxs/lib/`
- Install headers (kernel API and SLP) to `~/.sxs/include/`
- Set up standard kernels in `~/.sxs/lib/kernels/`
- Run kernel tests to verify the installation

The base language handled is extraordinarily minimal. Kernels are language extensions written in c++ that extend the language.
These are linked at runtime and you can swap out versions as you see fit. 

The install directory will look something like: 

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

The slp package is the "simple list parser" that parses our sources, the kernel_api.h is the C header for interop with the runtime from a dylib (see kernels/.)

We install `sxs` into `$SXS_HOME/bin`, and key off of `SXS_HOME` to find `kernels` at runtime so things like `#(load "io" "kv")` will work.

### Unified Runtime: `sxs`

The `sxs` binary provides a unified interface for both script execution and project management:
- **Script Mode**: Run standalone `.sxs` files directly: `sxs script.sxs`
- **Project Mode**: Manage structured applications with custom kernels: `sxs project <command>`
- **Additional Commands**: Type checking, testing, compilation (stubs), and more

See `sxs help` for full command reference or consult `SXS_MANUAL.md` for detailed documentation. 

### 3. Configure Environment

Add these lines to your shell configuration file (`~/.bashrc`, `~/.zshrc`, etc.):

```bash
export SXS_HOME=~/.sxs
export PATH=$SXS_HOME/bin:$PATH
```

Then reload your shell:
```bash
source ~/.bashrc
```

## Wizard Commands

The `wizard.sh` script provides several commands:

```bash
./wizard.sh --install      # Build and install to ~/.sxs
./wizard.sh --reinstall    # Force reinstall (removes existing ~/.sxs)
./wizard.sh --uninstall    # Remove ~/.sxs installation
./wizard.sh --check        # Check installation status
./wizard.sh --build        # Build the project
./wizard.sh --test         # Build and run all tests
```

## Usage

### Running Scripts

For standalone scripts:

```bash
sxs script.sxs
```

Example test scripts are available in `tests/`:

```bash
sxs tests/integration/kernel/kv/kv_full.sxs
sxs tests/integration/kernel/forge/concat.sxs
```

With options:

```bash
sxs -i ./custom-kernels script.sxs     # Add custom kernel path
sxs -v script.sxs                       # Verbose logging
sxs -w /tmp script.sxs                  # Set working directory
```

### Managing Projects

The `sxs` command provides project management for structured applications with custom kernels and modules.

#### Project Structure

```
my_project/
├── init.sxs              # Entry point
├── kernels/              # Custom C++ kernels
│   └── my_kernel/
│       ├── kernel.cpp
│       ├── kernel.hpp
│       └── Makefile
└── modules/              # SXS module libraries
    └── my_module/
        ├── my_module.sxs
        └── helper.sxs
```

#### Project Commands

```bash
sxs project new <name> [dir]    # Create a new project
sxs project build [dir]         # Build project kernels
sxs project run [dir]           # Build (if needed) and run project
sxs project clean [dir]         # Clean project cache
sxs deps [dir]                  # Show project dependencies and cache status
```

#### Workflow Example

```bash
sxs project new my_app
cd my_app

sxs project build

sxs project run

sxs deps

sxs project clean
```

Projects automatically manage kernel building and caching. Custom kernels in `kernels/` override system kernels, allowing project-specific language extensions.

### Additional Commands

```bash
sxs version                     # Show version information
sxs help                        # Show help message
sxs check <file>                # Type check code (stub)
sxs test [dir]                  # Run tests (stub)
sxs compile <file> -o <out>     # Compile program (stub)
```

For complete command reference, see `SXS_MANUAL.md` or run `sxs help`.

## SXS Language Features

- S-expression syntax
- Import system with circular dependency detection
- Dynamic scope management
- Lambda functions
- Type system (integers, reals, strings, symbols, lists, brace expressions)
- Kernel loading for native extensions

## Development

### VSCode Configuration

For VSCode users, create `.vscode/c_cpp_properties.json` with the following configuration to support IntelliSense:

```json
{
    "configurations": [
        {
            "name": "Mac",
            "includePath": [
                "${workspaceFolder}/**",
                "${workspaceFolder}/root/**",
                "${workspaceFolder}/core/**",
                "${workspaceFolder}/integrants/**",
                "${workspaceFolder}/kernels/**",
                "/opt/homebrew/include/**",
                "/usr/local/include/**",
                "${HOME}/.sxs/include/**"
            ],
            "defines": [],
            "compilerPath": "/usr/bin/clang++",
            "cStandard": "c17",
            "cppStandard": "c++20",
            "intelliSenseMode": "macos-clang-arm64",
            "compileCommands": "${workspaceFolder}/build/compile_commands.json"
        }
    ],
    "version": 4
}
```

Adjust paths for your platform (Linux/Windows) as needed. The key include paths are:
- `${workspaceFolder}/root/**` - SLP parser and kernel API
- `${workspaceFolder}/core/**` - Core runtime and interpreter
- `${workspaceFolder}/integrants/**` - Utility packages (kvds, random, etc.)
- `${workspaceFolder}/kernels/**` - Standard kernel implementations
- `${HOME}/.sxs/include/**` - Installed headers (kernel_api.h, slp headers)

### Manual Build

If you need to build without the wizard:

```bash
mkdir -p build && cd build
cmake ..
make -j8
make install
```

This single build process:
- Builds all components (root, core, integrants, kernels, binaries)
- Installs libraries and headers to `~/.sxs`
- Installs kernels to `~/.sxs/lib/kernels/`
- Installs binaries to `~/.sxs/bin/`

The binaries will be at:
- `build/sxs/sxs`
- `build/sup/sup`

### Running Tests

The recommended way to run all tests:

```bash
./wizard.sh --test
```

This runs:
1. Unit tests (ctest in `build/`)
2. Kernel integration tests (`tests/integration/kernel/`)

Or manually run specific test suites:

```bash
cd build
ctest --output-on-failure

cd tests/integration/kernel
./run.sh
```

## Project Structure

This is a unified monorepo with the following structure:

```
sxs/
├── root/                # Simple list parser (SLP) and kernel API
│   ├── slp/             # Parser for .sxs source files
│   │   ├── slp.hpp      # Main parser interface
│   │   └── buffer.hpp   # Buffer management
│   └── kernel_api.hpp   # C API for kernel development
│
├── core/                # Core interpreter and runtime
│   ├── core.hpp         # Main runtime interface
│   ├── interpreter.*    # Interpreter execution loop
│   ├── context.*        # Execution context management
│   ├── instructions/    # Instruction generation, interpretation, typechecking
│   ├── imports/         # Import system with circular dependency detection
│   ├── kernels/         # Kernel loading and management
│   └── type_checker/    # Type checking system
│
├── integrants/          # Utility packages
│   ├── kvds/            # Key-value datastore (memory and disk backends)
│   ├── random/          # Random number generation
│   ├── bytes/           # Byte manipulation utilities
│   ├── cache/           # Caching utilities
│   └── types/           # Type utilities (lifetime management)
│
├── kernels/             # Standard language extension kernels
│   ├── alu/             # Arithmetic and logic operations
│   ├── forge/           # List manipulation operations
│   ├── io/              # I/O operations (print, read, file operations)
│   ├── kv/              # Key-value store operations
│   └── random/          # Random generation operations
│
├── sxs/                 # sxs binary (script runner)
│   └── main.cpp         # Entry point for standalone script execution
│
├── sup/                 # sup binary (project manager)
│   ├── main.cpp         # Entry point for project management
│   ├── project.*        # Project creation and structure
│   ├── runtime.*        # Project runtime execution
│   ├── dep.*            # Dependency management
│   └── clean.*          # Cache cleaning
│
├── tests/               # All tests (unit and integration)
│   ├── unit/            # Unit tests for core components
│   └── integration/     # Integration tests
│       └── kernel/      # Kernel integration tests
│
└── wizard.sh            # Build and installation script
```

## AI Usage

Warning: The documentation in this code base is almost entirely just me chatting with a claude and spinning up markdown docs.
I do NOT give a shit about the _synthetic feel_ that happens when ai generates text when its about docs. all i care about is
correctness. i review all the docs it makes as i'm making it with the thing but of course errors will always be present. this way
though i guarantee there will be fewer errors than if i, a single dude, maintained all the docs myself without the assist. 

you can see this in the docs of software i've written before that are similar to this software in nature (sauros, nibi, nabla, umbra, etc.)

Warning: Shell scripts are mostly maintained by ai now. after i got the ball rolling on testing i started having a claude generate the `wizard`
script and a few of the test shell scripts. i am actively using and reviewing these and i wouldn't consider them vibe coded at all.

Claude and other ai are notoriously bad at c++. It's also horrendous at sxs (slp) generation.
If you do decide to contribute to this application please don't use AI on code other than to refactor or
tab complete. Don't make sweeping changes with agents.

Please just use your head and leverage the tools you have in a considerate and thoughtful manner. 

## License

MIT

