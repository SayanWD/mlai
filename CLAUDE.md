# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a machine learning library built from scratch in C for handwritten digit recognition using the MNIST dataset. The project implements a complete ML framework including matrix operations, automatic differentiation, and neural network training. Based on https://github.com/Magicalbat/videos/tree/main/machine-learning

## Build Commands

### Development
```bash
make              # Build with debug symbols (default)
make debug        # Same as above
make release      # Optimized release build
make run          # Build and run
```

### Data Preparation
```bash
python3 -m pip install -r requirements.txt
make data         # Download and prepare MNIST dataset
```

### Testing and Security
```bash
make asan              # Build with AddressSanitizer
make test-asan         # Run with AddressSanitizer
make analyze           # Static analysis with cppcheck
make security-check    # Run security checks
```

### Cleanup
```bash
make clean      # Remove build artifacts
make cleanall   # Remove build artifacts and data files
```

## Project Structure

```
mlai/
├── main.c          # Main program, model implementation, training loop
├── arena.c         # Arena memory allocator (platform-specific)
├── arena.h         # Arena allocator interface
├── prng.c          # PCG random number generator
├── prng.h          # PRNG interface
├── base.h          # Type definitions and macros
├── mnist.py        # Python script for MNIST data preparation
├── Makefile        # Build system with security flags
├── requirements.txt # Python dependencies
└── SECURITY.md     # Security guidelines
```

## Core Architecture

### 1. Mathematical Layer (Matrix Operations)
Located in [main.c](main.c) starting around line 278:
- `matrix` struct: row-major storage with u32 dimensions
- Basic operations: create, load, copy, clear, fill, fill_rand
- Arithmetic: add, sub, mul (with transpose options), scale
- Activation functions: ReLU, softmax
- Loss: cross-entropy
- Gradient operations for each forward operation

### 2. Computational Graph (Automatic Differentiation)
Located in [main.c](main.c) starting around line 44:
- `model_var` struct: graph nodes containing value, gradient, operation, inputs
- Operations: CREATE, RELU, SOFTMAX, ADD, SUB, MATMUL, CROSS_ENTROPY
- Flags: REQUIRES_GRAD, PARAMETER, INPUT, OUTPUT, DESIRED_OUTPUT, COST
- Two execution programs: forward (compute values) and cost (includes gradients)
- Topological sort for execution order

### 3. Training Infrastructure
Located in [main.c](main.c):
- `model_context`: holds all variables and compiled programs
- `model_training_desc`: training configuration
- SGD optimizer with mini-batch support
- Epoch-based training with shuffle

## Memory Management

Uses arena-based allocation ([arena.c](arena.c), [arena.h](arena.h)):
- Platform-specific (Windows via VirtualAlloc, Linux via mmap)
- Reserve/commit pattern for efficient memory usage
- Thread-local scratch arenas for temporary allocations
- Automatic cleanup on arena destruction

## Random Number Generation

PCG-based PRNG ([prng.c](prng.c), [prng.h](prng.h)):
- Apache License 2.0
- Thread-safe with explicit state (`prng_state`)
- Global state for convenience functions
- Float generation in [0, 1) range

## Model Architecture

Implemented in `create_mnist_model()` in [main.c](main.c:242):
- Input: 784 features (28×28 pixels)
- Layer 1: W0 (16×784) + b0 (16×1) → ReLU
- Layer 2: W1 (16×16) + b1 (16×1) → ReLU + residual connection from Layer 1
- Output: W2 (10×16) + b2 (10×1) → Softmax
- Xavier initialization for weights
- Cross-entropy loss

Target performance: ~86-89% test accuracy after 10 epochs

## Security Considerations

### Build Security
The Makefile includes hardened compiler flags:
- `-D_FORTIFY_SOURCE=2`: Buffer overflow detection
- `-fstack-protector-strong`: Stack canary protection
- `-fPIE -pie`: Position-independent executable
- `-Wl,-z,relro,-z,now`: GOT/PLT hardening (Linux)
- Warning flags to catch common bugs

### Code Security Issues Identified

**CRITICAL - Must fix before production:**

1. **No NULL checks in mat_load()** ([main.c:291](main.c#L291))
   - `fopen()` can return NULL, causing crash
   - `fseek()`, `ftell()`, `fread()` used without validation

2. **Integer overflow in matrix operations**
   - Cast to `u64` is present but not consistently checked
   - Large dimensions could overflow in size calculations

3. **No bounds checking on array indexing**
   - [main.c:175-176](main.c#L175): `train_labels_file->data[i]` used as index without validation
   - Could access out-of-bounds if label ≥ 10

4. **Unsafe `memcpy` usage** ([main.c:195](main.c#L195))
   - Size hardcoded as `sizeof(f32) * 784`
   - Should verify source buffer size

5. **mat_add/mat_sub return wrong value** ([main.c:382](main.c#L382), [main.c:398](main.c#L398))
   - Both return `false` instead of `true` on success
   - Logic bug that could cause issues

### Recommended Fixes

Add validation functions:
```c
// Add to main.c
FILE* safe_fopen(const char* filename, const char* mode) {
    FILE* f = fopen(filename, mode);
    if (!f) {
        fprintf(stderr, "Error: Failed to open file '%s'\n", filename);
        exit(1);
    }
    return f;
}

// Use in mat_load and add size validation
```

Add bounds checking:
```c
// In one-hot encoding loop
u32 num = train_labels_file->data[i];
if (num >= 10) {
    fprintf(stderr, "Error: Invalid label %u at index %u\n", num, i);
    exit(1);
}
```

Fix return values:
```c
// In mat_add and mat_sub, change return false to return true
return true;  // line 382 and 398
```

## Data Pipeline

Python script ([mnist.py](mnist.py)):
- Uses TensorFlow Datasets to load MNIST
- Normalizes images: [0, 255] → [0, 1]
- Keeps labels as integers (not one-hot, converted in C)
- Outputs binary `.mat` files

Binary format:
- Raw float32 arrays (little-endian)
- No headers or metadata
- Files: train_images.mat, train_labels.mat, test_images.mat, test_labels.mat

## Development Workflow

1. Install dependencies: `pip install -r requirements.txt`
2. Prepare data: `make data`
3. Build with sanitizers during development: `make asan`
4. Test: `make test-asan`
5. Run security checks: `make security-check`
6. Build release: `make release`

## Known Limitations

- Single-threaded execution
- No GPU acceleration
- Fixed architecture (hardcoded in create_mnist_model)
- No model serialization/checkpoint saving
- Limited to MNIST (28×28 grayscale images)
- Platform-specific code (Windows/Linux only, no macOS mmap support added)

## Implementation Notes

- Gradients accumulate with `_add_grad` functions
- Topological sort uses DFS with visited/temp markers
- Matrix multiplication supports transpose flags to avoid copies
- ReLU gradient: 1 if x > 0, else 0
- Softmax backward requires full Jacobian computation
- Cross-entropy gradient simplified for one-hot labels
- Arena position must stay aligned to `sizeof(void*)`

## Testing Recommendations

1. Test with various batch sizes (1, 32, 50, 100)
2. Verify gradient computation with numerical gradients
3. Test with artificially corrupted data files
4. Memory leak detection with valgrind
5. Bounds checking with sanitizers
6. Test on different platforms (Linux/Windows)
