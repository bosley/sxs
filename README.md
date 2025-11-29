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
- Clone and build `sxs-std` to `~/.sxs`
- Build the main SXS project
- Install the `sxs` binary to `~/.sxs/bin/sxs`
- Set up kernels and libraries

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
./wizard.sh --install      # Install sxs-std and build project
./wizard.sh --reinstall    # Force reinstall (removes existing)
./wizard.sh --uninstall    # Remove sxs-std installation
./wizard.sh --check        # Check installation status
./wizard.sh --build        # Build the main sxs project
./wizard.sh --test         # Build and run all tests
```

## Usage

After installation:

```bash
sxs script.sxs
```

### Example Scripts

See the `scripts/` directory for example SXS programs:

```bash
sxs scripts/example.sxs
sxs scripts/test_kv_comprehensive.sxs
```

## SXS Language Features

- S-expression syntax
- Import system with circular dependency detection
- Dynamic scope management
- Lambda functions
- Type system (integers, reals, strings, symbols, lists, brace expressions)
- Kernel loading for native extensions

## Development

### Manual Build

If you need to build without the wizard:

```bash
mkdir -p build
cd build
cmake ..
make -j8
```

The main binary will be at `build/cmd/sxs/sxs`

### Running Tests

```bash
./wizard.sh --test
```

Or manually:

```bash
cd build
ctest --output-on-failure
```

### Debugging with ASAN

```bash
cd build
cmake -DWITH_ASAN=ON ..
make -j8
```

## Project Structure

```
sxs/
├── cmd/              # Entry points (sxs binary)
├── pkg/              # Core runtime libraries
│   ├── core/         # Interpreter, VM, datum types
│   ├── bytes/        # Byte utilities
│   └── types/        # Type system helpers
├── tests/            # Unit tests
├── scripts/          # Example SXS scripts
├── kernels/          # Native kernel extensions
└── extern/           # External dependencies (submodules)
```

## License

MIT

