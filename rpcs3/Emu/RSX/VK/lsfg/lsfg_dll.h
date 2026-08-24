#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum LsfgStatus {
    LSFG_OK = 0,
    LSFG_NOT_INSTALLED = 1,
    LSFG_UNREADABLE_FILE = 2,
    LSFG_NOT_PORTABLE_EXECUTABLE = 3,
    LSFG_MISSING_SHADERS = 4,
    LSFG_TRANSLATION_FAILED = 5,
    LSFG_CACHE_UNUSABLE = 6
} LsfgStatus;

typedef enum LsfgVariant {
    LSFG_VARIANT_NONE = 0,
    LSFG_VARIANT_FP16 = 1,
    LSFG_VARIANT_FP32 = 2,
    LSFG_VARIANT_DXBC = 3
} LsfgVariant;

#define LSFG_SHADER_MIPMAPS     255u
#define LSFG_SHADER_GENERATE    256u
#define LSFG_SHADER_PERF_FIRST  280u
#define LSFG_SHADER_PERF_LAST   302u
#define LSFG_SHADER_COUNT       25u

typedef struct LsfgModule {
    uint32_t  id;
    uint32_t* words;
    uint32_t  word_count;
} LsfgModule;

typedef struct LsfgModuleSet {
    LsfgModule  modules[LSFG_SHADER_COUNT];
    uint32_t    count;
    LsfgVariant variant;
} LsfgModuleSet;

typedef struct LsfgCacheInfo {
    uint64_t    source_size;
    uint64_t    source_hash;
    uint32_t    module_count;
    LsfgVariant variant;
} LsfgCacheInfo;

const uint32_t* lsfg_shader_ids(size_t* out_count);

LsfgStatus lsfg_validate_dll(const char* dll_path);

LsfgStatus lsfg_validate_dll_fd(int fd);

LsfgStatus lsfg_build_cache(const char* dll_path, const char* cache_path, bool prefer_fp16);

LsfgStatus lsfg_build_cache_fd(int fd, const char* cache_path, bool prefer_fp16);

LsfgStatus lsfg_load_modules(const char* cache_path, LsfgModuleSet* out_set);

LsfgStatus lsfg_cache_info(const char* cache_path, LsfgCacheInfo* out_info);

const char* lsfg_variant_name(LsfgVariant variant);

void lsfg_release_modules(LsfgModuleSet* set);

const uint32_t* lsfg_find_module(const LsfgModuleSet* set, uint32_t id, uint32_t* out_word_count);

#ifdef __cplusplus
}
#endif
