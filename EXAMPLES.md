# MLAI Usage Examples

## Model Save/Load

### Basic Save/Load

```c
#include "model_io.h"

// After training...
matrix* params[6] = {W0, b0, W1, b1, W2, b2};
if (model_save("trained_model.mlai", params, 6)) {
    printf("Model saved successfully!\n");
}

// Later, to load:
matrix* loaded_params[6] = {NULL, NULL, NULL, NULL, NULL, NULL};
if (model_load(arena, "trained_model.mlai", loaded_params, 6)) {
    printf("Model loaded successfully!\n");
    // Use loaded_params[0] as W0, loaded_params[1] as b0, etc.
}
```

### Validate Before Loading

```c
if (!model_validate("model.mlai")) {
    fprintf(stderr, "Model file is corrupted or invalid!\n");
    return 1;
}

matrix* params[3];
model_load(arena, "model.mlai", params, 3);
```

### Pre-allocate Matrices

```c
// If you know the dimensions, pre-allocate:
matrix* params[3];
params[0] = mat_create(arena, 16, 784);
params[1] = mat_create(arena, 16, 16);
params[2] = mat_create(arena, 10, 16);

// Load will fill these matrices and validate dimensions
if (!model_load(arena, "model.mlai", params, 3)) {
    fprintf(stderr, "Dimension mismatch!\n");
}
```

## Model File Format

The `.mlai` format includes:

### Header (16 bytes)
- Magic number: `0x4D4C4149` ("MLAI")
- Version: `1`
- Number of parameters: `uint32`
- Reserved: `uint32`

### Per-Parameter (12 bytes + data)
- Rows: `uint32`
- Cols: `uint32`
- Checksum: `uint32`
- Data: `rows × cols × 4` bytes (float32)

### Example Structure

```
Offset  Size    Content
------  ------  -------
0x00    4       Magic (0x4D4C4149)
0x04    4       Version (1)
0x08    4       Num params (3)
0x0C    4       Reserved (0)

0x10    4       Param 0 rows (16)
0x14    4       Param 0 cols (784)
0x18    4       Param 0 checksum
0x1C    50176   Param 0 data (16 × 784 × 4 bytes)

0xC42C  4       Param 1 rows (16)
0xC430  4       Param 1 cols (16)
0xC434  4       Param 1 checksum
0xC438  1024    Param 1 data (16 × 16 × 4 bytes)

...
```

## Testing Examples

### Run All Tests

```bash
$ make test
Running unit tests...
========================================
  MLAI Unit Test Suite
========================================

[ RUN  ] mat_create_basic
[ PASS ] mat_create_basic
...
Total:  12
Passed: 12 ✅
Failed: 0 ❌

Running corrupted data tests...
========================================
  MLAI Corrupted Data Test Suite
========================================

[ RUN  ] load_nonexistent_file
[ PASS ] load_nonexistent_file
...
Total:  9
Passed: 9 ✅
Failed: 0 ❌
```

### Run Specific Test Suite

```bash
# Unit tests only
$ make test-unit

# Corrupted data tests only
$ make test-corrupted
```

### Add Custom Tests

Edit `test_simple.c` or `test_corrupted.c`:

```c
TEST_START("my_custom_test");
{
    matrix* m = mat_create(test_arena, 5, 5);
    ASSERT(m != NULL);

    mat_fill(m, 42.0f);
    ASSERT_NEAR(m->data[0], 42.0f, 1e-6f);
}
TEST_END("my_custom_test");
```

## Integration Example

### Complete Training and Save Workflow

```c
#include "base.h"
#include "arena.h"
#include "matrix.h"
#include "model_io.h"

int main(void) {
    mem_arena* arena = arena_create(GiB(1), MiB(1));

    // Load data
    matrix* train_images = mat_load(arena, 60000, 784, "train_images.mat");
    matrix* train_labels = mat_load(arena, 60000, 10, "train_labels.mat");

    // Create and train model
    model_context* model = model_create(arena);
    create_mnist_model(arena, model);
    model_compile(arena, model);

    model_training_desc training = {
        .train_images = train_images,
        .train_labels = train_labels,
        .epochs = 10,
        .batch_size = 50,
        .learning_rate = 0.01f
    };

    model_train(model, &training);

    // Save model parameters
    matrix* params[6] = {W0, b0, W1, b1, W2, b2};
    model_save("mnist_model.mlai", params, 6);

    // Test loading
    matrix* loaded[6] = {NULL, NULL, NULL, NULL, NULL, NULL};
    if (model_load(arena, "mnist_model.mlai", loaded, 6)) {
        printf("Model reloaded successfully!\n");
    }

    arena_destroy(arena);
    return 0;
}
```

## Error Handling Best Practices

### Always Check Return Values

```c
// ❌ Bad
model_save("model.mlai", params, 3);

// ✅ Good
if (!model_save("model.mlai", params, 3)) {
    fprintf(stderr, "Failed to save model\n");
    return 1;
}
```

### Validate Before Critical Operations

```c
// Check file exists and is valid before loading
if (!model_validate("important_model.mlai")) {
    fprintf(stderr, "Model file is corrupted!\n");
    // Fallback to default weights or retrain
}
```

### Handle Dimension Mismatches

```c
// Pre-allocate with expected dimensions
matrix* params[1];
params[0] = mat_create(arena, 10, 10);

if (!model_load(arena, "model.mlai", params, 1)) {
    fprintf(stderr, "Dimension mismatch or file error\n");
    // Either recreate with correct size or fail gracefully
}
```

## Performance Tips

### Arena Usage

```c
// Use temporary arenas for transient data
mem_arena_temp temp = arena_temp_begin(arena);

matrix* tmp = mat_create(arena, 1000, 1000);
// ... use tmp ...

arena_temp_end(temp);  // Automatically frees tmp
```

### Batch Processing

```c
// Process in batches to reduce memory usage
for (u32 batch = 0; batch < num_batches; batch++) {
    mem_arena_temp temp = arena_temp_begin(arena);

    // Process batch
    matrix* batch_data = mat_create(arena, batch_size, 784);
    // ... process ...

    arena_temp_end(temp);  // Free batch memory
}
```

## Security Considerations

### Model File Validation

```c
// Always validate untrusted model files
if (!model_validate(user_provided_file)) {
    fprintf(stderr, "Untrusted model file failed validation\n");
    return 1;
}

// Check file size is reasonable
struct stat st;
if (stat(filename, &st) == 0) {
    if (st.st_size > MAX_MODEL_SIZE) {
        fprintf(stderr, "Model file too large\n");
        return 1;
    }
}
```

### Checksum Verification

The model_io system automatically verifies checksums:

```c
// If checksum fails, a warning is printed:
// "Warning: Checksum mismatch for parameter 0 (file may be corrupted)"

// You can catch this by checking stderr or implementing custom logging
```

## Debugging

### Enable Debug Builds

```bash
make debug        # Includes debug symbols
make asan         # Address sanitizer for memory errors
```

### Print Matrix Contents

```c
void print_matrix(matrix* m, const char* name) {
    printf("%s (%u×%u):\n", name, m->rows, m->cols);
    for (u32 i = 0; i < m->rows && i < 5; i++) {  // First 5 rows
        for (u32 j = 0; j < m->cols && j < 10; j++) {  // First 10 cols
            printf("%.4f ", m->data[i * m->cols + j]);
        }
        printf("...\n");
    }
}
```

### Check Memory Leaks

```bash
valgrind --leak-check=full ./mlai
```

## Common Issues

### Issue: "Cannot open file"

**Cause**: File doesn't exist or no permissions

**Solution**:
```c
// Check file exists before loading
#include <sys/stat.h>

struct stat buffer;
if (stat("model.mlai", &buffer) != 0) {
    fprintf(stderr, "File does not exist\n");
}
```

### Issue: "Dimension mismatch"

**Cause**: Loaded model has different dimensions than expected

**Solution**:
```c
// Let model_load create matrices with correct sizes
matrix* params[3] = {NULL, NULL, NULL};
model_load(arena, "model.mlai", params, 3);
// Now params[0]->rows and params[0]->cols are set correctly
```

### Issue: "Checksum mismatch"

**Cause**: File corrupted or modified

**Solution**:
```c
// Validate before loading
if (!model_validate("model.mlai")) {
    fprintf(stderr, "File is corrupted. Retraining...\n");
    // Retrain model
}
```
