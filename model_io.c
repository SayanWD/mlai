#include "model_io.h"
#include <stdlib.h>

// Simple checksum calculation
static u32 calculate_checksum(const f32* data, u64 size) {
    u32 checksum = 0;
    const u8* bytes = (const u8*)data;
    u64 total_bytes = size * sizeof(f32);

    for (u64 i = 0; i < total_bytes; i++) {
        checksum = (checksum << 1) ^ bytes[i];
    }

    return checksum;
}

b32 model_save(const char* filename, matrix** params, u32 num_params) {
    if (!filename || !params || num_params == 0) {
        fprintf(stderr, "Error: Invalid arguments to model_save\n");
        return false;
    }

    FILE* f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Error: Cannot create file '%s'\n", filename);
        return false;
    }

    // Write header
    model_header header = {
        .magic = MODEL_MAGIC,
        .version = MODEL_VERSION,
        .num_params = num_params,
        .reserved = 0
    };

    if (fwrite(&header, sizeof(model_header), 1, f) != 1) {
        fprintf(stderr, "Error: Failed to write header\n");
        fclose(f);
        return false;
    }

    // Write each parameter
    for (u32 i = 0; i < num_params; i++) {
        matrix* m = params[i];
        if (!m) {
            fprintf(stderr, "Error: Parameter %u is NULL\n", i);
            fclose(f);
            return false;
        }

        u64 size = (u64)m->rows * m->cols;
        u32 checksum = calculate_checksum(m->data, size);

        // Write metadata
        param_metadata meta = {
            .rows = m->rows,
            .cols = m->cols,
            .checksum = checksum
        };

        if (fwrite(&meta, sizeof(param_metadata), 1, f) != 1) {
            fprintf(stderr, "Error: Failed to write metadata for parameter %u\n", i);
            fclose(f);
            return false;
        }

        // Write data
        u64 bytes_to_write = size * sizeof(f32);
        if (fwrite(m->data, 1, bytes_to_write, f) != bytes_to_write) {
            fprintf(stderr, "Error: Failed to write data for parameter %u\n", i);
            fclose(f);
            return false;
        }
    }

    fclose(f);
    printf("Model saved to '%s' (%u parameters)\n", filename, num_params);
    return true;
}

b32 model_load(mem_arena* arena, const char* filename, matrix** params, u32 num_params) {
    if (!filename || !params || num_params == 0) {
        fprintf(stderr, "Error: Invalid arguments to model_load\n");
        return false;
    }

    FILE* f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
        return false;
    }

    // Read header
    model_header header;
    if (fread(&header, sizeof(model_header), 1, f) != 1) {
        fprintf(stderr, "Error: Failed to read header\n");
        fclose(f);
        return false;
    }

    // Validate header
    if (header.magic != MODEL_MAGIC) {
        fprintf(stderr, "Error: Invalid magic number (expected 0x%08X, got 0x%08X)\n",
                MODEL_MAGIC, header.magic);
        fclose(f);
        return false;
    }

    if (header.version != MODEL_VERSION) {
        fprintf(stderr, "Error: Unsupported version (expected %u, got %u)\n",
                MODEL_VERSION, header.version);
        fclose(f);
        return false;
    }

    if (header.num_params != num_params) {
        fprintf(stderr, "Error: Parameter count mismatch (expected %u, got %u)\n",
                num_params, header.num_params);
        fclose(f);
        return false;
    }

    // Load each parameter
    for (u32 i = 0; i < num_params; i++) {
        // Read metadata
        param_metadata meta;
        if (fread(&meta, sizeof(param_metadata), 1, f) != 1) {
            fprintf(stderr, "Error: Failed to read metadata for parameter %u\n", i);
            fclose(f);
            return false;
        }

        // Create or verify matrix
        if (!params[i]) {
            params[i] = mat_create(arena, meta.rows, meta.cols);
            if (!params[i]) {
                fprintf(stderr, "Error: Failed to create matrix for parameter %u\n", i);
                fclose(f);
                return false;
            }
        } else {
            // Verify dimensions
            if (params[i]->rows != meta.rows || params[i]->cols != meta.cols) {
                fprintf(stderr, "Error: Dimension mismatch for parameter %u\n", i);
                fclose(f);
                return false;
            }
        }

        // Read data
        u64 size = (u64)meta.rows * meta.cols;
        u64 bytes_to_read = size * sizeof(f32);
        if (fread(params[i]->data, 1, bytes_to_read, f) != bytes_to_read) {
            fprintf(stderr, "Error: Failed to read data for parameter %u\n", i);
            fclose(f);
            return false;
        }

        // Verify checksum
        u32 checksum = calculate_checksum(params[i]->data, size);
        if (checksum != meta.checksum) {
            fprintf(stderr, "Warning: Checksum mismatch for parameter %u (file may be corrupted)\n", i);
            // Don't fail, but warn
        }
    }

    fclose(f);
    printf("Model loaded from '%s' (%u parameters)\n", filename, num_params);
    return true;
}

b32 model_validate(const char* filename) {
    if (!filename) {
        return false;
    }

    FILE* f = fopen(filename, "rb");
    if (!f) {
        return false;
    }

    // Read and validate header
    model_header header;
    if (fread(&header, sizeof(model_header), 1, f) != 1) {
        fclose(f);
        return false;
    }

    if (header.magic != MODEL_MAGIC || header.version != MODEL_VERSION) {
        fclose(f);
        return false;
    }

    // Quick validation of structure
    for (u32 i = 0; i < header.num_params; i++) {
        param_metadata meta;
        if (fread(&meta, sizeof(param_metadata), 1, f) != 1) {
            fclose(f);
            return false;
        }

        // Skip data
        u64 size = (u64)meta.rows * meta.cols * sizeof(f32);
        if (fseek(f, (long)size, SEEK_CUR) != 0) {
            fclose(f);
            return false;
        }
    }

    fclose(f);
    return true;
}
