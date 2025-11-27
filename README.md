# SXS - Runtime & SLP Script Executor

A runtime system for executing SLP (S-expression Like Programming) scripts with built-in event system, key-value storage, and session management.

## Building the Project

### Prerequisites

- CMake 3.15+
- C++20 compatible compiler
- fmt library
- spdlog library
- RocksDB
- zstd (via pkg-config)

### Build Instructions

```bash
# Clone the repository
cd sxs

# Create build directory
mkdir -p build
cd build

# Configure and build (Debug with ASAN off by default)
cmake ..
make -j8

# Or build release version (optimized, no ASAN)
make release
```

The binaries will be in `build/bin/`:
- `sxsd` - SLP script executor daemon
- Example programs in `build/bin/examples/`

## Running SLP Scripts

### Basic Usage

```bash
./build/bin/sxsd <script.slp> [options]
```

### Options

- `--help, -h` - Print help message
- `--runtime-root-path, -r PATH` - Set runtime root path (default: ~/.sxdd)
- `--include-path, -i PATH` - Add include path (repeatable)
- `--event-system-max-threads, -t NUM` - Max event system threads (default: 4)
- `--event-system-max-queue-size, -q NUM` - Max event queue size (default: 1000)
- `--max-sessions-per-entity, -s NUM` - Max sessions per entity (default: 10)

### Example Scripts

```bash
# Run KV storage demo
./build/bin/sxsd sxs_slp/kv.slp -r /tmp/sxs_demo

# Run event system demo
./build/bin/sxsd sxs_slp/events.slp -r /tmp/sxs_demo
```

### Example SLP Script

```lisp
{
  (core/util/log "Hello from SLP!")
  
  (core/kv/set username "alice")
  (core/kv/set age 30)
  
  (core/util/log "User:" (core/kv/get username))
  (core/util/log "Age:" (core/kv/get age))
}
```

## SLP Language Features

- **KV Operations**: `core/kv/set`, `core/kv/get`, `core/kv/del`, `core/kv/exists`
- **Event System**: `core/event/pub`, `core/event/sub` with type-safe handlers
- **Utilities**: `core/util/log`, `core/expr/eval`
- **Sequential Execution**: Brace lists `{}` execute statements sequentially
- **Type System**: Strong typing with `:int`, `:real`, `:str`, `:symbol`, etc.

See `pkg/runtime/fns/COMMANDS.md` for complete function reference.

---

## Development Notes

### ASAN Debugging

If you need to enable AddressSanitizer for debugging:

```bash
cd build
cmake -DWITH_ASAN=ON ..
make -j8
```

#### Linux ASLR Issue

If you get `AddressSanitizer:DEADLYSIGNAL` errors on Linux, you may need to disable ASLR:

```bash
echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
```
