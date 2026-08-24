#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool lsfg_translate_dxbc(const uint8_t* bytecode, uint32_t size, uint32_t** out_words,
                         uint32_t* out_word_count);

void lsfg_free_translated(uint32_t* words);

#ifdef __cplusplus
}
#endif
