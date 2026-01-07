#define _CRT_SECURE_NO_WARNINGS

#include "base.h"
#include "arena.h"
#include "matrix.h"
#include "model_io.h"

#include <string.h>

// Test framework
typedef struct {
    u32 total;
    u32 passed;
    u32 failed;
} test_results;

#define TEST_START(name) do { \
    printf("[ RUN  ] %s\n", name); \
    mem_arena_temp _test_temp = arena_temp_begin(test_arena); \
    b32 _test_passed = true;

#define TEST_END(name) \
    arena_temp_end(_test_temp); \
    results.total++; \
    if (_test_passed) { \
        results.passed++; \
        printf("[ PASS ] %s\n\n", name); \
    } else { \
        results.failed++; \
        printf("[ FAIL ] %s\n\n", name); \
    } \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "  ❌ ASSERT FAILED: %s at line %d\n", #cond, __LINE__); \
        _test_passed = false; \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        fprintf(stderr, "  ❌ ASSERT_EQ FAILED: %s != %s at line %d\n", #a, #b, __LINE__); \
        _test_passed = false; \
    } \
} while(0)

// Create corrupted test file
static void create_corrupted_file(const char* filename, u32 corruption_type) {
    FILE* f = fopen(filename, "wb");
    if (!f) return;

    switch (corruption_type) {
        case 0: {
            // Empty file
            break;
        }
        case 1: {
            // Wrong magic number
            u32 bad_magic = 0xDEADBEEF;
            fwrite(&bad_magic, sizeof(u32), 1, f);
            break;
        }
        case 2: {
            // Truncated header
            u32 magic = 0x4D4C4149;
            fwrite(&magic, sizeof(u32), 1, f);
            // Missing rest of header
            break;
        }
        case 3: {
            // Valid header but missing data
            model_header header = {
                .magic = 0x4D4C4149,
                .version = 1,
                .num_params = 1,
                .reserved = 0
            };
            fwrite(&header, sizeof(model_header), 1, f);
            // Missing param data
            break;
        }
        case 4: {
            // Header + metadata but truncated data
            model_header header = {
                .magic = 0x4D4C4149,
                .version = 1,
                .num_params = 1,
                .reserved = 0
            };
            fwrite(&header, sizeof(model_header), 1, f);

            param_metadata meta = {.rows = 10, .cols = 10, .checksum = 0};
            fwrite(&meta, sizeof(param_metadata), 1, f);

            // Write only half the data
            f32 data[50];
            memset(data, 0, sizeof(data));
            fwrite(data, sizeof(f32), 50, f);
            break;
        }
    }

    fclose(f);
}

// External functions
extern matrix* mat_create(mem_arena* arena, u32 rows, u32 cols);
extern void mat_fill(matrix* mat, f32 x);

int main(void) {
    printf("========================================\n");
    printf("  MLAI Corrupted Data Test Suite\n");
    printf("========================================\n\n");

    mem_arena* test_arena = arena_create(MiB(100), MiB(1));
    if (!test_arena) {
        fprintf(stderr, "Failed to create test arena\n");
        return 1;
    }

    test_results results = {0};

    // Test: Load non-existent file
    TEST_START("load_nonexistent_file");
    {
        matrix* params[1] = {NULL};
        b32 result = model_load(test_arena, "nonexistent_model.mlai", params, 1);
        ASSERT(result == false); // Should fail gracefully
    }
    TEST_END("load_nonexistent_file");

    // Test: Empty file
    TEST_START("load_empty_file");
    {
        create_corrupted_file("corrupted_empty.mlai", 0);
        matrix* params[1] = {NULL};
        b32 result = model_load(test_arena, "corrupted_empty.mlai", params, 1);
        ASSERT(result == false);
        remove("corrupted_empty.mlai");
    }
    TEST_END("load_empty_file");

    // Test: Wrong magic number
    TEST_START("load_wrong_magic");
    {
        create_corrupted_file("corrupted_magic.mlai", 1);
        matrix* params[1] = {NULL};
        b32 result = model_load(test_arena, "corrupted_magic.mlai", params, 1);
        ASSERT(result == false);
        remove("corrupted_magic.mlai");
    }
    TEST_END("load_wrong_magic");

    // Test: Truncated header
    TEST_START("load_truncated_header");
    {
        create_corrupted_file("corrupted_header.mlai", 2);
        matrix* params[1] = {NULL};
        b32 result = model_load(test_arena, "corrupted_header.mlai", params, 1);
        ASSERT(result == false);
        remove("corrupted_header.mlai");
    }
    TEST_END("load_truncated_header");

    // Test: Missing parameter data
    TEST_START("load_missing_data");
    {
        create_corrupted_file("corrupted_nodata.mlai", 3);
        matrix* params[1] = {NULL};
        b32 result = model_load(test_arena, "corrupted_nodata.mlai", params, 1);
        ASSERT(result == false);
        remove("corrupted_nodata.mlai");
    }
    TEST_END("load_missing_data");

    // Test: Truncated parameter data
    TEST_START("load_truncated_data");
    {
        create_corrupted_file("corrupted_truncated.mlai", 4);
        matrix* params[1] = {NULL};
        b32 result = model_load(test_arena, "corrupted_truncated.mlai", params, 1);
        ASSERT(result == false);
        remove("corrupted_truncated.mlai");
    }
    TEST_END("load_truncated_data");

    // Test: Save and load valid model
    TEST_START("save_load_valid_model");
    {
        // Create test parameters
        matrix* save_params[3];
        save_params[0] = mat_create(test_arena, 10, 10);
        save_params[1] = mat_create(test_arena, 5, 10);
        save_params[2] = mat_create(test_arena, 2, 5);

        ASSERT(save_params[0] != NULL && save_params[1] != NULL && save_params[2] != NULL);

        mat_fill(save_params[0], 1.5f);
        mat_fill(save_params[1], 2.5f);
        mat_fill(save_params[2], 3.5f);

        // Save
        b32 save_result = model_save("test_model.mlai", save_params, 3);
        ASSERT(save_result == true);

        // Load
        matrix* load_params[3] = {NULL, NULL, NULL};
        b32 load_result = model_load(test_arena, "test_model.mlai", load_params, 3);
        ASSERT(load_result == true);

        // Verify data
        ASSERT(load_params[0]->rows == 10 && load_params[0]->cols == 10);
        ASSERT(load_params[1]->rows == 5 && load_params[1]->cols == 10);
        ASSERT(load_params[2]->rows == 2 && load_params[2]->cols == 5);

        // Check values (spot check)
        ASSERT(fabsf(load_params[0]->data[0] - 1.5f) < 1e-6f);
        ASSERT(fabsf(load_params[1]->data[0] - 2.5f) < 1e-6f);
        ASSERT(fabsf(load_params[2]->data[0] - 3.5f) < 1e-6f);

        remove("test_model.mlai");
    }
    TEST_END("save_load_valid_model");

    // Test: Validate model file
    TEST_START("validate_model_file");
    {
        // Create valid model
        matrix* params[1];
        params[0] = mat_create(test_arena, 5, 5);
        ASSERT(params[0] != NULL);
        mat_fill(params[0], 1.0f);

        b32 save_result = model_save("valid_model.mlai", params, 1);
        ASSERT(save_result == true);

        // Validate
        b32 valid = model_validate("valid_model.mlai");
        ASSERT(valid == true);

        // Test invalid file
        create_corrupted_file("invalid_model.mlai", 1);
        b32 invalid = model_validate("invalid_model.mlai");
        ASSERT(invalid == false);

        remove("valid_model.mlai");
        remove("invalid_model.mlai");
    }
    TEST_END("validate_model_file");

    // Test: Dimension mismatch
    TEST_START("load_dimension_mismatch");
    {
        // Save with one size
        matrix* save_params[1];
        save_params[0] = mat_create(test_arena, 10, 10);
        ASSERT(save_params[0] != NULL);
        mat_fill(save_params[0], 1.0f);

        b32 save_result = model_save("mismatch_model.mlai", save_params, 1);
        ASSERT(save_result == true);

        // Try to load with wrong size
        matrix* load_params[1];
        load_params[0] = mat_create(test_arena, 5, 5); // Wrong size
        ASSERT(load_params[0] != NULL);

        b32 load_result = model_load(test_arena, "mismatch_model.mlai", load_params, 1);
        ASSERT(load_result == false); // Should fail due to size mismatch

        remove("mismatch_model.mlai");
    }
    TEST_END("load_dimension_mismatch");

    printf("========================================\n");
    printf("  Test Results\n");
    printf("========================================\n");
    printf("Total:  %u\n", results.total);
    printf("Passed: %u ✅\n", results.passed);
    printf("Failed: %u ❌\n", results.failed);
    printf("========================================\n");

    arena_destroy(test_arena);

    return results.failed == 0 ? 0 : 1;
}
