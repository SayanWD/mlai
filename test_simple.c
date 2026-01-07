#define _CRT_SECURE_NO_WARNINGS

#include "base.h"
#include "arena.h"
#include "prng.h"
#include "matrix.h"

// Simple test framework
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
        fprintf(stderr, "  ❌ ASSERT_EQ FAILED: %s != %s (%u != %u) at line %d\n", \
                #a, #b, (u32)(a), (u32)(b), __LINE__); \
        _test_passed = false; \
    } \
} while(0)

#define ASSERT_NEAR(a, b, eps) do { \
    f32 _a = (a), _b = (b), _diff = fabsf(_a - _b); \
    if (_diff > (eps)) { \
        fprintf(stderr, "  ❌ ASSERT_NEAR FAILED: %s != %s (%.6f != %.6f, diff=%.6f) at line %d\n", \
                #a, #b, _a, _b, _diff, __LINE__); \
        _test_passed = false; \
    } \
} while(0)

// Include implementations - will be linked
extern matrix* mat_create(mem_arena* arena, u32 rows, u32 cols);
extern b32 mat_copy(matrix* dst, matrix* src);
extern void mat_clear(matrix* mat);
extern void mat_fill(matrix* mat, f32 x);
extern void mat_fill_rand(matrix* mat, f32 lower, f32 upper);
extern void mat_scale(matrix* mat, f32 scale);
extern f32 mat_sum(matrix* mat);
extern u64 mat_argmax(matrix* mat);
extern b32 mat_add(matrix* out, const matrix* a, const matrix* b);
extern b32 mat_sub(matrix* out, const matrix* a, const matrix* b);
extern b32 mat_mul(matrix* out, const matrix* a, const matrix* b, b8 zero_out, b8 transpose_a, b8 transpose_b);
extern b32 mat_relu(matrix* out, const matrix* in);
extern b32 mat_softmax(matrix* out, const matrix* in);

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

    // Test: mat_create_basic
    TEST_START("mat_create_basic");
    {
        matrix* m = mat_create(test_arena, 3, 4);
        ASSERT(m != NULL);
        ASSERT_EQ(m->rows, 3);
        ASSERT_EQ(m->cols, 4);
        ASSERT(m->data != NULL);

        // Should be zero-initialized
        for (u32 i = 0; i < 12; i++) {
            ASSERT_NEAR(m->data[i], 0.0f, 1e-6f);
        }
    }
    TEST_END("mat_create_basic");

    // Test: mat_create_overflow
    TEST_START("mat_create_overflow");
    {
        matrix* m = mat_create(test_arena, UINT32_MAX, UINT32_MAX);
        ASSERT(m == NULL); // Should fail
    }
    TEST_END("mat_create_overflow");

    // Test: mat_fill_and_sum
    TEST_START("mat_fill_and_sum");
    {
        matrix* m = mat_create(test_arena, 10, 10);
        ASSERT(m != NULL);

        mat_fill(m, 3.14f);

        for (u32 i = 0; i < 100; i++) {
            ASSERT_NEAR(m->data[i], 3.14f, 1e-6f);
        }

        f32 sum = mat_sum(m);
        ASSERT_NEAR(sum, 314.0f, 1e-3f); // Floating point tolerance
    }
    TEST_END("mat_fill_and_sum");

    // Test: mat_scale
    TEST_START("mat_scale");
    {
        matrix* m = mat_create(test_arena, 5, 5);
        ASSERT(m != NULL);

        mat_fill(m, 2.0f);
        mat_scale(m, 3.0f);

        for (u32 i = 0; i < 25; i++) {
            ASSERT_NEAR(m->data[i], 6.0f, 1e-6f);
        }
    }
    TEST_END("mat_scale");

    // Test: mat_argmax
    TEST_START("mat_argmax");
    {
        matrix* m = mat_create(test_arena, 1, 10);
        ASSERT(m != NULL);

        for (u32 i = 0; i < 10; i++) {
            m->data[i] = (f32)i;
        }
        m->data[7] = 100.0f;

        u64 max_idx = mat_argmax(m);
        ASSERT_EQ(max_idx, 7);
    }
    TEST_END("mat_argmax");

    // Test: mat_add_basic
    TEST_START("mat_add_basic");
    {
        matrix* a = mat_create(test_arena, 2, 3);
        matrix* b = mat_create(test_arena, 2, 3);
        matrix* out = mat_create(test_arena, 2, 3);

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
    }
    TEST_END("mat_add_basic");

    // Test: mat_add_dimension_mismatch
    TEST_START("mat_add_dimension_mismatch");
    {
        matrix* a = mat_create(test_arena, 2, 3);
        matrix* b = mat_create(test_arena, 3, 2);
        matrix* out = mat_create(test_arena, 2, 3);

        ASSERT(a != NULL && b != NULL && out != NULL);

        b32 result = mat_add(out, a, b);
        ASSERT(result == false); // Should fail
    }
    TEST_END("mat_add_dimension_mismatch");

    // Test: mat_sub_basic
    TEST_START("mat_sub_basic");
    {
        matrix* a = mat_create(test_arena, 2, 2);
        matrix* b = mat_create(test_arena, 2, 2);
        matrix* out = mat_create(test_arena, 2, 2);

        ASSERT(a != NULL && b != NULL && out != NULL);

        mat_fill(a, 10.0f);
        mat_fill(b, 3.0f);

        b32 result = mat_sub(out, a, b);
        ASSERT(result == true);

        for (u32 i = 0; i < 4; i++) {
            ASSERT_NEAR(out->data[i], 7.0f, 1e-6f);
        }
    }
    TEST_END("mat_sub_basic");

    // Test: mat_relu
    TEST_START("mat_relu");
    {
        matrix* in = mat_create(test_arena, 1, 6);
        matrix* out = mat_create(test_arena, 1, 6);

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
    }
    TEST_END("mat_relu");

    // Test: mat_softmax
    TEST_START("mat_softmax");
    {
        matrix* in = mat_create(test_arena, 1, 3);
        matrix* out = mat_create(test_arena, 1, 3);

        ASSERT(in != NULL && out != NULL);

        in->data[0] = 1.0f;
        in->data[1] = 2.0f;
        in->data[2] = 3.0f;

        b32 result = mat_softmax(out, in);
        ASSERT(result == true);

        // Check sum = 1
        f32 sum = out->data[0] + out->data[1] + out->data[2];
        ASSERT_NEAR(sum, 1.0f, 1e-6f);

        // Check monotonicity
        ASSERT(out->data[2] > out->data[1]);
        ASSERT(out->data[1] > out->data[0]);

        // All positive
        ASSERT(out->data[0] > 0.0f);
        ASSERT(out->data[1] > 0.0f);
        ASSERT(out->data[2] > 0.0f);
    }
    TEST_END("mat_softmax");

    // Test: prng_deterministic
    TEST_START("prng_deterministic");
    {
        prng_state rng1, rng2;
        prng_seed_r(&rng1, 12345, 67890);
        prng_seed_r(&rng2, 12345, 67890);

        for (u32 i = 0; i < 100; i++) {
            u32 r1 = prng_rand_r(&rng1);
            u32 r2 = prng_rand_r(&rng2);
            ASSERT_EQ(r1, r2);
        }
    }
    TEST_END("prng_deterministic");

    // Test: prng_range
    TEST_START("prng_range");
    {
        prng_state rng;
        prng_seed_r(&rng, 42, 42);

        for (u32 i = 0; i < 1000; i++) {
            f32 r = prng_randf_r(&rng);
            ASSERT(r >= 0.0f && r <= 1.0f);
        }
    }
    TEST_END("prng_range");

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
