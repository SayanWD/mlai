#define _CRT_SECURE_NO_WARNINGS

#include "base.h"
#include "arena.h"
#include "prng.h"

#include "arena.c"
#include "prng.c"

// Include matrix operations from main.c
typedef struct {
    u32 rows, cols;
    f32* data;
} matrix;

matrix* mat_create(mem_arena* arena, u32 rows, u32 cols);
b32 mat_copy(matrix* dst, matrix* src);
void mat_clear(matrix* mat);
void mat_fill(matrix* mat, f32 x);
void mat_scale(matrix* mat, f32 scale);
f32 mat_sum(matrix* mat);
u64 mat_argmax(matrix* mat);
b32 mat_add(matrix* out, const matrix* a, const matrix* b);
b32 mat_sub(matrix* out, const matrix* a, const matrix* b);
b32 mat_mul(matrix* out, const matrix* a, const matrix* b, b8 zero_out, b8 transpose_a, b8 transpose_b);
b32 mat_relu(matrix* out, const matrix* in);
b32 mat_softmax(matrix* out, const matrix* in);

// Test framework
typedef struct {
    const char* name;
    b32 (*func)(mem_arena*);
} test_case;

typedef struct {
    u32 total;
    u32 passed;
    u32 failed;
} test_results;

#define TEST(name) b32 test_##name(mem_arena* arena)
#define ASSERT(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "  ❌ ASSERT FAILED: %s at line %d\n", #cond, __LINE__); \
        return false; \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        fprintf(stderr, "  ❌ ASSERT_EQ FAILED: %s != %s (%u != %u) at line %d\n", \
                #a, #b, (u32)(a), (u32)(b), __LINE__); \
        return false; \
    } \
} while(0)

#define ASSERT_NEAR(a, b, eps) do { \
    f32 _a = (a), _b = (b), _diff = fabsf(_a - _b); \
    if (_diff > (eps)) { \
        fprintf(stderr, "  ❌ ASSERT_NEAR FAILED: %s != %s (%.6f != %.6f, diff=%.6f) at line %d\n", \
                #a, #b, _a, _b, _diff, __LINE__); \
        return false; \
    } \
} while(0)

// Matrix creation and basic operations
TEST(mat_create_basic) {
    matrix* m = mat_create(arena, 3, 4);
    ASSERT(m != NULL);
    ASSERT_EQ(m->rows, 3);
    ASSERT_EQ(m->cols, 4);
    ASSERT(m->data != NULL);

    // Should be zero-initialized
    for (u32 i = 0; i < 12; i++) {
        ASSERT_NEAR(m->data[i], 0.0f, 1e-6f);
    }

    return true;
}

TEST(mat_create_overflow) {
    // Test integer overflow protection
    matrix* m = mat_create(arena, UINT32_MAX, UINT32_MAX);
    ASSERT(m == NULL); // Should fail
    return true;
}

TEST(mat_fill_and_sum) {
    matrix* m = mat_create(arena, 10, 10);
    ASSERT(m != NULL);

    mat_fill(m, 3.14f);

    for (u32 i = 0; i < 100; i++) {
        ASSERT_NEAR(m->data[i], 3.14f, 1e-6f);
    }

    f32 sum = mat_sum(m);
    ASSERT_NEAR(sum, 314.0f, 1e-4f);

    return true;
}

TEST(mat_scale) {
    matrix* m = mat_create(arena, 5, 5);
    ASSERT(m != NULL);

    mat_fill(m, 2.0f);
    mat_scale(m, 3.0f);

    for (u32 i = 0; i < 25; i++) {
        ASSERT_NEAR(m->data[i], 6.0f, 1e-6f);
    }

    return true;
}

TEST(mat_argmax) {
    matrix* m = mat_create(arena, 1, 10);
    ASSERT(m != NULL);

    for (u32 i = 0; i < 10; i++) {
        m->data[i] = (f32)i;
    }
    m->data[7] = 100.0f; // Max value

    u64 max_idx = mat_argmax(m);
    ASSERT_EQ(max_idx, 7);

    return true;
}

// Matrix arithmetic
TEST(mat_add_basic) {
    matrix* a = mat_create(arena, 2, 3);
    matrix* b = mat_create(arena, 2, 3);
    matrix* out = mat_create(arena, 2, 3);

    ASSERT(a != NULL && b != NULL && out != NULL);

    for (u32 i = 0; i < 6; i++) {
        a->data[i] = (f32)i;
        b->data[i] = (f32)(i * 2);
    }

    b32 result = mat_add(out, a, b);
    ASSERT(result == true);

    for (u32 i = 0; i < 6; i++) {
        ASSERT_NEAR(out->data[i], (f32)(i * 3), 1e-6f);
    }

    return true;
}

TEST(mat_add_dimension_mismatch) {
    matrix* a = mat_create(arena, 2, 3);
    matrix* b = mat_create(arena, 3, 2);
    matrix* out = mat_create(arena, 2, 3);

    ASSERT(a != NULL && b != NULL && out != NULL);

    b32 result = mat_add(out, a, b);
    ASSERT(result == false); // Should fail

    return true;
}

TEST(mat_sub_basic) {
    matrix* a = mat_create(arena, 2, 2);
    matrix* b = mat_create(arena, 2, 2);
    matrix* out = mat_create(arena, 2, 2);

    ASSERT(a != NULL && b != NULL && out != NULL);

    mat_fill(a, 10.0f);
    mat_fill(b, 3.0f);

    b32 result = mat_sub(out, a, b);
    ASSERT(result == true);

    for (u32 i = 0; i < 4; i++) {
        ASSERT_NEAR(out->data[i], 7.0f, 1e-6f);
    }

    return true;
}

// Matrix multiplication
TEST(mat_mul_basic) {
    matrix* a = mat_create(arena, 2, 3);
    matrix* b = mat_create(arena, 3, 2);
    matrix* out = mat_create(arena, 2, 2);

    ASSERT(a != NULL && b != NULL && out != NULL);

    // A = [1 2 3]
    //     [4 5 6]
    a->data[0] = 1.0f; a->data[1] = 2.0f; a->data[2] = 3.0f;
    a->data[3] = 4.0f; a->data[4] = 5.0f; a->data[5] = 6.0f;

    // B = [7  8]
    //     [9  10]
    //     [11 12]
    b->data[0] = 7.0f;  b->data[1] = 8.0f;
    b->data[2] = 9.0f;  b->data[3] = 10.0f;
    b->data[4] = 11.0f; b->data[5] = 12.0f;

    // Result should be:
    // [58  64]
    // [139 154]
    b32 result = mat_mul(out, a, b, false, false, false);
    ASSERT(result == true);

    ASSERT_NEAR(out->data[0], 58.0f, 1e-4f);
    ASSERT_NEAR(out->data[1], 64.0f, 1e-4f);
    ASSERT_NEAR(out->data[2], 139.0f, 1e-4f);
    ASSERT_NEAR(out->data[3], 154.0f, 1e-4f);

    return true;
}

TEST(mat_mul_identity) {
    matrix* a = mat_create(arena, 3, 3);
    matrix* identity = mat_create(arena, 3, 3);
    matrix* out = mat_create(arena, 3, 3);

    ASSERT(a != NULL && identity != NULL && out != NULL);

    // Fill A with sequential values
    for (u32 i = 0; i < 9; i++) {
        a->data[i] = (f32)(i + 1);
    }

    // Create identity matrix
    mat_clear(identity);
    identity->data[0] = 1.0f; // [0,0]
    identity->data[4] = 1.0f; // [1,1]
    identity->data[8] = 1.0f; // [2,2]

    b32 result = mat_mul(out, a, identity, false, false, false);
    ASSERT(result == true);

    // A * I = A
    for (u32 i = 0; i < 9; i++) {
        ASSERT_NEAR(out->data[i], a->data[i], 1e-6f);
    }

    return true;
}

// Activation functions
TEST(mat_relu) {
    matrix* in = mat_create(arena, 1, 6);
    matrix* out = mat_create(arena, 1, 6);

    ASSERT(in != NULL && out != NULL);

    in->data[0] = -2.0f;
    in->data[1] = -1.0f;
    in->data[2] = 0.0f;
    in->data[3] = 1.0f;
    in->data[4] = 2.0f;
    in->data[5] = 3.0f;

    b32 result = mat_relu(out, in);
    ASSERT(result == true);

    ASSERT_NEAR(out->data[0], 0.0f, 1e-6f);
    ASSERT_NEAR(out->data[1], 0.0f, 1e-6f);
    ASSERT_NEAR(out->data[2], 0.0f, 1e-6f);
    ASSERT_NEAR(out->data[3], 1.0f, 1e-6f);
    ASSERT_NEAR(out->data[4], 2.0f, 1e-6f);
    ASSERT_NEAR(out->data[5], 3.0f, 1e-6f);

    return true;
}

TEST(mat_softmax) {
    matrix* in = mat_create(arena, 1, 3);
    matrix* out = mat_create(arena, 1, 3);

    ASSERT(in != NULL && out != NULL);

    in->data[0] = 1.0f;
    in->data[1] = 2.0f;
    in->data[2] = 3.0f;

    b32 result = mat_softmax(out, in);
    ASSERT(result == true);

    // Check sum = 1
    f32 sum = out->data[0] + out->data[1] + out->data[2];
    ASSERT_NEAR(sum, 1.0f, 1e-6f);

    // Check monotonicity (larger input -> larger output)
    ASSERT(out->data[2] > out->data[1]);
    ASSERT(out->data[1] > out->data[0]);

    // All values should be positive
    ASSERT(out->data[0] > 0.0f);
    ASSERT(out->data[1] > 0.0f);
    ASSERT(out->data[2] > 0.0f);

    return true;
}

// PRNG tests
TEST(prng_deterministic) {
    prng_state rng1, rng2;
    prng_seed_r(&rng1, 12345, 67890);
    prng_seed_r(&rng2, 12345, 67890);

    for (u32 i = 0; i < 100; i++) {
        u32 r1 = prng_rand_r(&rng1);
        u32 r2 = prng_rand_r(&rng2);
        ASSERT_EQ(r1, r2);
    }

    return true;
}

TEST(prng_range) {
    prng_state rng;
    prng_seed_r(&rng, 42, 42);

    for (u32 i = 0; i < 1000; i++) {
        f32 r = prng_randf_r(&rng);
        ASSERT(r >= 0.0f && r <= 1.0f);
    }

    return true;
}

// Include matrix implementation
#include "main.c"

// Test runner
void run_test(test_case* tc, test_results* results, mem_arena* arena) {
    printf("[ RUN  ] %s\n", tc->name);

    mem_arena_temp temp = arena_temp_begin(arena);
    b32 passed = tc->func(arena);
    arena_temp_end(temp);

    results->total++;
    if (passed) {
        results->passed++;
        printf("[ PASS ] %s\n", tc->name);
    } else {
        results->failed++;
        printf("[ FAIL ] %s\n", tc->name);
    }
    printf("\n");
}

int main(void) {
    printf("========================================\n");
    printf("  MLAI Unit Test Suite\n");
    printf("========================================\n\n");

    mem_arena* test_arena = arena_create(MiB(100), MiB(1));
    if (!test_arena) {
        fprintf(stderr, "Failed to create test arena\n");
        return 1;
    }

    test_results results = {0};

    test_case tests[] = {
        {"mat_create_basic", test_mat_create_basic},
        {"mat_create_overflow", test_mat_create_overflow},
        {"mat_fill_and_sum", test_mat_fill_and_sum},
        {"mat_scale", test_mat_scale},
        {"mat_argmax", test_mat_argmax},
        {"mat_add_basic", test_mat_add_basic},
        {"mat_add_dimension_mismatch", test_mat_add_dimension_mismatch},
        {"mat_sub_basic", test_mat_sub_basic},
        {"mat_mul_basic", test_mat_mul_basic},
        {"mat_mul_identity", test_mat_mul_identity},
        {"mat_relu", test_mat_relu},
        {"mat_softmax", test_mat_softmax},
        {"prng_deterministic", test_prng_deterministic},
        {"prng_range", test_prng_range},
    };

    u32 num_tests = sizeof(tests) / sizeof(tests[0]);

    for (u32 i = 0; i < num_tests; i++) {
        run_test(&tests[i], &results, test_arena);
    }

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
