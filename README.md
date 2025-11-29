# SXS

A dynamic extensible runtime based on the "simple list programs" slp.

## Prerequisites

- CMake 3.15+
- C++20 compatible compiler
- Git

### System Dependencies

Install the following dependencies on your system:

**macOS (Homebrew):**
```bash
brew install rocksdb spdlog fmt zstd
```

**Ubuntu/Debian:**
```bash
sudo apt-get install librocksdb-dev libspdlog-dev libfmt-dev libzstd-dev
```

**Fedora/RHEL:**
```bash
sudo dnf install rocksdb-devel spdlog-devel fmt-devel libzstd-devel
```

## Installation

### 1. Clone and Initialize

```bash
git clone <repository-url> sxs
cd sxs
git submodule update --init --recursive
```

### 2. Install Using Wizard

```bash
./wizard.sh --install
```

This will:
- Build and install the standard library (`libs/`) to `~/.sxs`
- Build the applications (`apps/`)
- Install the `sxs` binary to `~/.sxs/bin/sxs`
- Set up kernels and libraries in `~/.sxs/lib/kernels/`

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

We install `sxs` into the same dir, and key off of `SXS_HOME` to find `kernels` at runtime so things like `#(load "io" "kv")` will work. 

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
./wizard.sh --install      # Build libs, install to ~/.sxs, build apps, install binary
./wizard.sh --reinstall    # Force reinstall (removes existing ~/.sxs)
./wizard.sh --uninstall    # Remove ~/.sxs installation
./wizard.sh --check        # Check installation status
./wizard.sh --build        # Build the apps project (requires libs installed)
./wizard.sh --test         # Build and run all tests (libs + apps)
```

## Usage

After installation:

```bash
sxs script.sxs
```

### Example Scripts

See the `apps/scripts/` directory for example SXS programs:

```bash
sxs apps/scripts/example.sxs
sxs apps/scripts/test_kv_comprehensive.sxs
```

## SXS Language Features

- S-expression syntax
- Import system with circular dependency detection
- Dynamic scope management
- Lambda functions
- Type system (integers, reals, strings, symbols, lists, brace expressions)
- Kernel loading for native extensions

## Development

### VSCode Configuration

For VSCode users, create `.vscode/c_cpp_properties.json` with the following configuration to support IntelliSense for both `apps/` and `libs/` directories:

```json
{
    "configurations": [
        {
            "name": "Mac",
            "includePath": [
                "${workspaceFolder}/apps/**",
                "${workspaceFolder}/apps/pkg/**",
                "${workspaceFolder}/apps/extern/snitch/include/**",
                "${workspaceFolder}/apps/extern/json/include/**",
                "${workspaceFolder}/apps/extern/cpp-httplib/**",
                "${workspaceFolder}/libs/**",
                "${workspaceFolder}/libs/pkg/**",
                "/opt/homebrew/include/**",
                "/usr/local/include/**",
                "${HOME}/.sxs/include/**"
            ],
            "defines": [],
            "compilerPath": "/usr/bin/clang++",
            "cStandard": "c17",
            "cppStandard": "c++20",
            "intelliSenseMode": "macos-clang-arm64",
            "compileCommands": "${workspaceFolder}/apps/build/compile_commands.json"
        }
    ],
    "version": 4
}
```

Adjust paths for your platform (Linux/Windows) as needed. The key include paths are:
- `${workspaceFolder}/apps/**` - Apps source code
- `${workspaceFolder}/libs/**` - Libs (standard library) source code
- `${HOME}/.sxs/include/**` - Installed headers (kernel_api.h, slp headers)

### Manual Build

If you need to build without the wizard:

**1. Build and install libs:**
```bash
cd libs
mkdir -p build && cd build
cmake ..
make -j8
make install  # Installs to ~/.sxs
```

**2. Build apps:**
```bash
cd ../../apps
mkdir -p build && cd build
cmake ..
make -j8
```

The main binary will be at `apps/build/cmd/sxs/sxs`

### Running Tests

The recommended way to run all tests:

```bash
./wizard.sh --test
```

This runs:
1. Libs unit tests (ctest in `libs/build/`)
2. Libs kernel tests (`libs/tests/kernel/`)
3. Apps unit tests (ctest in `apps/build/`)
4. Apps kernel integration tests (`apps/tests/integration/kernel/`)
5. Apps integration tests (`apps/tests/integration/app/`)

Or manually run specific test suites:

```bash
# Libs unit tests
cd libs/build
ctest --output-on-failure

# Apps unit tests
cd apps/build
ctest --output-on-failure

# Kernel tests
cd apps/tests/integration/kernel
./test.sh
```

## Project Structure

This is a monorepo with two main directories:

```
sxs/
├── apps/                # Main application code
│   ├── cmd/             # Entry points (sxs, sxs-rt binaries)
│   ├── pkg/             # Core runtime libraries
│   │   ├── core/        # Interpreter, VM, datum types
│   │   └── bytes/       # Byte utilities
│   ├── tests/           # Application tests
│   │   ├── unit/        # Unit tests
│   │   └── integration/ # Integration tests (kernel, app)
│   ├── scripts/         # Example SXS scripts
│   └── extern/          # External dependencies (submodules)
│
├── libs/                # Standard library (installed to ~/.sxs)
│   ├── pkg/             # Library packages
│   │   ├── kvds/        # Key-value datastore
│   │   ├── slp/         # Simple list parser
│   │   ├── random/      # Random number generation
│   │   └── kernel_api.h # C API for kernel development
│   ├── std/             # Standard kernels
│   │   ├── alu/         # Arithmetic operations kernel
│   │   ├── io/          # I/O operations kernel
│   │   ├── kv/          # Key-value store kernel
│   │   └── random/      # Random generation kernel
│   └── tests/           # Library tests
│
└── wizard.sh            # Build and installation script
```

## License

MIT

