#ifndef MODEL_IO_H
#define MODEL_IO_H

#include "base.h"
#include "arena.h"
#include "matrix.h"

// Model file format version
#define MODEL_VERSION 1
#define MODEL_MAGIC 0x4D4C4149  // "MLAI" in hex

// Model file header
typedef struct {
    u32 magic;           // Magic number for validation
    u32 version;         // File format version
    u32 num_params;      // Number of parameter matrices
    u32 reserved;        // Reserved for future use
} model_header;

// Parameter metadata
typedef struct {
    u32 rows;
    u32 cols;
    u32 checksum;        // Simple checksum for validation
} param_metadata;

// Save model parameters to file
b32 model_save(const char* filename, matrix** params, u32 num_params);

// Load model parameters from file
b32 model_load(mem_arena* arena, const char* filename, matrix** params, u32 num_params);

// Validate model file
b32 model_validate(const char* filename);

#endif // MODEL_IO_H
