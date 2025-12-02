# Testing

Tests are divided into **build-time** (`unit`) and **run-time** (`integration`) categories.

## Unit Tests

Build-time tests that run during compilation. These include:
- Direct C++ evaluative testing
- Tests for integrants backing kernels
- Tests for root structures
- Some tests execute sxs code copied to the build directory

Located in `tests/unit/`

## Integration Tests

Run-time tests requiring a built AND installed system. Two categories:

### Kernel
Full-system tests validating each kernel's runtime behavior within the language environment. Each kernel has its own sxs test suite ensuring proper integration with the runtime.

### Standalone
Holistic tests validating complex interactions across the entire system. These leverage multiple kernels and the kernel API to test system-wide integration scenarios (e.g., working directory propagation through system info API to kernel path resolution).

Located in `tests/integration/`, orchestrated by `run_all.sh` which runs kernel tests followed by auto-discovered standalone tests.

