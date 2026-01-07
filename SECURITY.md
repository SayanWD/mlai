# Security Policy

## Overview

This document outlines security considerations and best practices for the MLAI project.

## Known Security Issues

### Critical Issues (Must Fix Before Production)

#### 1. Unchecked File Operations
**Location**: [main.c:291](main.c#L291) - `mat_load()`

**Issue**: No NULL check after `fopen()`, leading to potential NULL pointer dereference.

```c
FILE* f = fopen(filename, "rb");  // Can return NULL
fseek(f, 0, SEEK_END);             // Crash if f is NULL
```

**Impact**: Program crash, potential DoS
**Severity**: HIGH

**Fix**:
```c
FILE* f = fopen(filename, "rb");
if (!f) {
    fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
    return NULL;
}
```

#### 2. Array Index Validation
**Location**: [main.c:175-176](main.c#L175)

**Issue**: Label values used as array indices without validation.

```c
u32 num = train_labels_file->data[i];
train_labels->data[i * 10 + num] = 1.0f;  // num could be >= 10
```

**Impact**: Out-of-bounds write, potential code execution
**Severity**: CRITICAL

**Fix**:
```c
u32 num = train_labels_file->data[i];
if (num >= 10) {
    fprintf(stderr, "Error: Invalid label %u at index %u\n", num, i);
    return 1;
}
train_labels->data[i * 10 + num] = 1.0f;
```

#### 3. Integer Overflow in Size Calculations
**Location**: Multiple locations in matrix operations

**Issue**: Large matrix dimensions could cause integer overflow.

```c
mat->data = PUSH_ARRAY(arena, f32, (u64)rows * cols);
```

**Impact**: Buffer overflow, heap corruption
**Severity**: HIGH

**Fix**:
```c
if (rows > UINT32_MAX / cols) {
    fprintf(stderr, "Error: Matrix dimensions too large\n");
    return NULL;
}
u64 size = (u64)rows * cols;
if (size > SIZE_MAX / sizeof(f32)) {
    fprintf(stderr, "Error: Matrix size exceeds memory limits\n");
    return NULL;
}
```

#### 4. Return Value Bugs
**Location**: [main.c:382](main.c#L382), [main.c:398](main.c#L398)

**Issue**: `mat_add()` and `mat_sub()` return `false` on success.

```c
// Should be:
return true;  // Currently returns false
```

**Impact**: Logic errors in calling code
**Severity**: MEDIUM

### Medium Priority Issues

#### 5. Fixed Buffer Sizes
**Location**: [main.c:195](main.c#L195)

**Issue**: Hardcoded memcpy size without validation.

```c
memcpy(model->input->val->data, test_images->data, sizeof(f32) * 784);
```

**Recommendation**: Verify source and destination sizes match.

#### 6. No Memory Allocation Failure Handling
**Location**: Throughout code

**Issue**: Arena allocations can return NULL but are rarely checked.

**Recommendation**: Add consistent NULL checks after all allocations.

## Build Security

### Compiler Flags
The Makefile includes security-hardening flags:

```makefile
-D_FORTIFY_SOURCE=2        # Buffer overflow detection
-fstack-protector-strong   # Stack canary
-fPIE -pie                 # ASLR support
-Wl,-z,relro,-z,now       # Immediate binding (Linux)
```

### Recommended Build Process

1. **Development**: Always use sanitizers
   ```bash
   make asan
   ```

2. **Testing**: Run with address and undefined behavior sanitizers
   ```bash
   make test-asan
   ```

3. **Release**: Use optimizations but keep security flags
   ```bash
   make release
   ```

## Input Validation

### File Input
- Always validate file existence before reading
- Check file sizes before loading
- Verify data format matches expected structure
- Handle corrupted/truncated files gracefully

### Data Validation
- Verify matrix dimensions are reasonable
- Check for NaN/Inf in loaded data
- Validate label ranges (0-9 for MNIST)
- Ensure consistency between data and labels

## Memory Safety

### Arena Allocator
- Arena can run out of memory (returns NULL)
- No automatic bounds checking within arena
- Relies on correct size calculations

**Best Practices**:
- Check return values from `arena_push()`
- Use `PUSH_STRUCT` and `PUSH_ARRAY` macros
- Avoid manual pointer arithmetic
- Clear arenas after use with `arena_clear()`

### Buffer Overflows
Current protections:
- Compiler flags (`-D_FORTIFY_SOURCE=2`)
- Stack canaries (`-fstack-protector-strong`)

Needed improvements:
- Add bounds checking to matrix operations
- Validate all array indices
- Check sizes before memcpy/memset

## Random Number Generation

The PRNG (PCG) is cryptographically weak:
- **Do NOT use for**: Cryptographic keys, tokens, security-sensitive random values
- **Safe for**: Weight initialization, data shuffling, training augmentation

For security-sensitive randomness, use:
- Linux: `/dev/urandom`
- Windows: `BCryptGenRandom`
- OpenSSL: `RAND_bytes`

## Dependency Security

### Python Dependencies
Check for vulnerabilities:
```bash
pip install safety
safety check -r requirements.txt
```

Update regularly:
```bash
pip install --upgrade -r requirements.txt
```

### System Dependencies
- gcc/clang: Keep updated
- glibc: Security patches important
- TensorFlow Datasets: Monitor for CVEs

## Data Security

### MNIST Dataset
- Public dataset, no privacy concerns
- Verify checksums if downloading manually
- Store in `.gitignore` (done)

### Model Weights
- If saving weights, ensure secure storage
- Do not commit trained models to git
- Consider encryption for sensitive models

## Static Analysis

Run security checks:
```bash
make security-check
make analyze  # Requires cppcheck
```

Additional tools:
```bash
# Clang static analyzer
scan-build make

# Valgrind for memory errors
valgrind --leak-check=full ./mlai

# Address sanitizer
make asan && ./mlai
```

## Secure Coding Guidelines

1. **Always validate input**: Files, user data, network input
2. **Check return values**: Especially for allocation, I/O
3. **Avoid fixed buffers**: Use dynamic allocation with size checks
4. **Initialize memory**: Use `memset` or zero-initializing allocators
5. **No deprecated functions**: Avoid `gets`, `strcpy`, `sprintf`
6. **Minimize unsafe operations**: Pointer arithmetic, casts
7. **Use const**: Mark read-only data as const
8. **Bounds check arrays**: Validate indices before access

## Reporting Security Issues

If you find a security vulnerability:

1. **Do NOT** open a public issue
2. Contact maintainers privately
3. Provide:
   - Description of vulnerability
   - Steps to reproduce
   - Potential impact
   - Suggested fix (if available)

## Security Checklist

Before release:
- [ ] All file operations check return values
- [ ] Array indices are bounds-checked
- [ ] Integer overflow protection in size calculations
- [ ] Memory allocations checked for NULL
- [ ] Built with security flags enabled
- [ ] Tested with AddressSanitizer
- [ ] Tested with UndefinedBehaviorSanitizer
- [ ] Static analysis run (cppcheck/scan-build)
- [ ] No hardcoded secrets or credentials
- [ ] Dependencies updated and scanned
- [ ] Input validation on all external data
- [ ] Error messages don't leak sensitive info

## References

- [OWASP C Coding Practices](https://owasp.org/www-project-secure-coding-practices-quick-reference-guide/)
- [CERT C Coding Standard](https://wiki.sei.cmu.edu/confluence/display/c/SEI+CERT+C+Coding+Standard)
- [CWE Top 25](https://cwe.mitre.org/top25/)
- [Address Sanitizer](https://github.com/google/sanitizers/wiki/AddressSanitizer)
