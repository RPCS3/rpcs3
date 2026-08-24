#include "stdafx.h"
#include "lsfg_dll.h"
#include "lsfg_dxbc.h"

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define DOS_MAGIC          0x5A4Du
#define PE_SIGNATURE       0x00004550u
#define PE32_MAGIC         0x010Bu
#define PE32_PLUS_MAGIC    0x020Bu

#define DOS_LFANEW_OFFSET              0x3Cu
#define COFF_HEADER_SIZE               20u
#define OPTIONAL_HEADER_SIZE_OFFSET    16u
#define SECTION_HEADER_SIZE            40u
#define DATA_DIRECTORY_ENTRY_SIZE      8u
#define DATA_DIRECTORY_OFFSET_PE32     96u
#define DATA_DIRECTORY_OFFSET_PE32P    112u
#define RESOURCE_DATA_DIRECTORY_INDEX  2u

#define RESOURCE_DIRECTORY_SIZE     16u
#define RESOURCE_NAMED_COUNT_OFFSET 12u
#define RESOURCE_ID_COUNT_OFFSET    14u
#define RESOURCE_ENTRY_SIZE         8u
#define RESOURCE_SUBDIRECTORY_FLAG  0x80000000u
#define RESOURCE_TYPE_RCDATA        10u

#define SPIRV_MAGIC                  0x07230203u
#define SPIRV_HEADER_WORDS           5u
#define SPIRV_WORD_COUNT_SHIFT       16u
#define SPIRV_OPCODE_MASK            0xFFFFu
#define SPIRV_OP_FUNCTION            54u
#define SPIRV_OP_DECORATE            71u
#define SPIRV_DECORATION_BINDING     33u
#define SPIRV_DECORATION_DESCRIPTOR_SET 34u
#define DECORATION_LITERAL_WORD      3u

#define VARIANT_FP16_OFFSET 49u
#define VARIANT_FP32_OFFSET 98u

#define MAX_RESOURCE_ID 512u

#define CACHE_MAGIC   0x4746534Cu
#define CACHE_VERSION 1u

#define MAX_SPIRV_WORDS (16u * 1024u * 1024u)

typedef struct PeSection {
    uint32_t virtual_address;
    uint32_t virtual_size;
    uint32_t raw_address;
    uint32_t raw_size;
} PeSection;

typedef struct PeImage {
    const uint8_t* data;
    size_t         size;
    PeSection*     sections;
    uint32_t       section_count;
    bool           heap_backed;
    int            owned_fd;
} PeImage;

typedef struct ResourceTable {
    const uint8_t* data[MAX_RESOURCE_ID];
    uint32_t       size[MAX_RESOURCE_ID];
} ResourceTable;

typedef struct CacheHeader {
    uint32_t magic;
    uint32_t version;
    uint64_t source_size;
    uint64_t source_hash;
    uint32_t module_count;
    uint32_t variant;
} CacheHeader;

typedef struct BindingSlot {
    uint32_t set;
    uint32_t binding;
    size_t   literal_offset;
} BindingSlot;

static const uint32_t* build_shader_ids(size_t* out_count) {
    static uint32_t ids[LSFG_SHADER_COUNT];
    static size_t count = 0;
    if (count == 0) {
        ids[count++] = LSFG_SHADER_MIPMAPS;
        ids[count++] = LSFG_SHADER_GENERATE;
        for (uint32_t id = LSFG_SHADER_PERF_FIRST; id <= LSFG_SHADER_PERF_LAST; id++) {
            ids[count++] = id;
        }
    }
    if (out_count) *out_count = count;
    return ids;
}

const uint32_t* lsfg_shader_ids(size_t* out_count) {
    return build_shader_ids(out_count);
}

static uint64_t fnv1a64(const uint8_t* data, size_t size) {
    uint64_t hash = 1469598103934665603ULL;
    for (size_t i = 0; i < size; i++) {
        hash ^= static_cast<uint64_t>(data[i]);
        hash *= 1099511628211ULL;
    }
    return hash;
}

static bool pe_read_u16(const PeImage* image, size_t offset, uint16_t* out_value) {
    if (offset > image->size || image->size - offset < sizeof(uint16_t)) return false;
    memcpy(out_value, image->data + offset, sizeof(uint16_t));
    return true;
}

static bool pe_read_u32(const PeImage* image, size_t offset, uint32_t* out_value) {
    if (offset > image->size || image->size - offset < sizeof(uint32_t)) return false;
    memcpy(out_value, image->data + offset, sizeof(uint32_t));
    return true;
}

static bool pe_find_header(const PeImage* image, size_t* out_offset) {
    uint16_t dos_magic = 0;
    if (!pe_read_u16(image, 0, &dos_magic) || dos_magic != DOS_MAGIC) return false;

    uint32_t pe_offset = 0;
    if (!pe_read_u32(image, DOS_LFANEW_OFFSET, &pe_offset)) return false;

    uint32_t signature = 0;
    if (!pe_read_u32(image, pe_offset, &signature) || signature != PE_SIGNATURE) return false;

    *out_offset = static_cast<size_t>(pe_offset);
    return true;
}

static bool pe_find_data_directory(const PeImage* image, size_t optional_header_offset,
                                   size_t* out_offset) {
    uint16_t optional_magic = 0;
    if (!pe_read_u16(image, optional_header_offset, &optional_magic)) return false;

    switch (optional_magic) {
        case PE32_MAGIC:
            *out_offset = optional_header_offset + DATA_DIRECTORY_OFFSET_PE32;
            return true;
        case PE32_PLUS_MAGIC:
            *out_offset = optional_header_offset + DATA_DIRECTORY_OFFSET_PE32P;
            return true;
        default:
            return false;
    }
}

static bool pe_read_sections(PeImage* image, size_t pe_offset) {
    uint16_t section_count = 0;
    uint16_t optional_header_size = 0;
    if (!pe_read_u16(image, pe_offset + 4 + 2, &section_count) ||
        !pe_read_u16(image, pe_offset + 4 + OPTIONAL_HEADER_SIZE_OFFSET, &optional_header_size)) {
        return false;
    }
    if (section_count == 0) return false;

    image->sections = static_cast<PeSection*>(calloc(section_count, sizeof(PeSection)));
    if (!image->sections) return false;

    const size_t table_offset = pe_offset + 4 + COFF_HEADER_SIZE + optional_header_size;
    for (uint16_t i = 0; i < section_count; i++) {
        const size_t offset = table_offset + static_cast<size_t>(i) * SECTION_HEADER_SIZE;
        PeSection* section = &image->sections[i];
        if (!pe_read_u32(image, offset + 8, &section->virtual_size) ||
            !pe_read_u32(image, offset + 12, &section->virtual_address) ||
            !pe_read_u32(image, offset + 16, &section->raw_size) ||
            !pe_read_u32(image, offset + 20, &section->raw_address)) {
            free(image->sections);
            image->sections = NULL;
            return false;
        }
    }
    image->section_count = section_count;
    return true;
}

static bool pe_rva_to_offset(const PeImage* image, uint32_t rva, size_t* out_offset) {
    for (uint32_t i = 0; i < image->section_count; i++) {
        const PeSection* section = &image->sections[i];
        const uint32_t span = section->virtual_size > section->raw_size ? section->virtual_size
                                                                       : section->raw_size;
        if (span == 0 || rva < section->virtual_address) continue;
        const uint32_t relative = rva - section->virtual_address;
        if (relative < span) {
            *out_offset = static_cast<size_t>(section->raw_address) + relative;
            return true;
        }
    }
    return false;
}

static bool resource_entry_count(const PeImage* image, size_t directory_offset, size_t* out_total) {
    uint16_t named_count = 0;
    uint16_t id_count = 0;
    if (!pe_read_u16(image, directory_offset + RESOURCE_NAMED_COUNT_OFFSET, &named_count) ||
        !pe_read_u16(image, directory_offset + RESOURCE_ID_COUNT_OFFSET, &id_count)) {
        return false;
    }
    *out_total = static_cast<size_t>(named_count) + static_cast<size_t>(id_count);
    return true;
}

static bool resource_entry_at(const PeImage* image, size_t directory_offset, size_t index,
                              uint32_t* out_id, uint32_t* out_offset, bool* out_is_directory,
                              bool* out_is_named) {
    const size_t offset =
        directory_offset + RESOURCE_DIRECTORY_SIZE + index * RESOURCE_ENTRY_SIZE;
    uint32_t name = 0;
    uint32_t data = 0;
    if (!pe_read_u32(image, offset, &name) || !pe_read_u32(image, offset + 4, &data)) return false;

    *out_id = name & ~RESOURCE_SUBDIRECTORY_FLAG;
    *out_offset = data & ~RESOURCE_SUBDIRECTORY_FLAG;
    *out_is_directory = (data & RESOURCE_SUBDIRECTORY_FLAG) != 0;
    *out_is_named = (name & RESOURCE_SUBDIRECTORY_FLAG) != 0;
    return true;
}

static bool resource_read_leaf(const PeImage* image, size_t leaf_offset,
                               const uint8_t** out_data, uint32_t* out_size) {
    uint32_t data_rva = 0;
    uint32_t data_size = 0;
    if (!pe_read_u32(image, leaf_offset, &data_rva) ||
        !pe_read_u32(image, leaf_offset + 4, &data_size) || data_size == 0) {
        return false;
    }

    size_t data_offset = 0;
    if (!pe_rva_to_offset(image, data_rva, &data_offset)) return false;
    if (data_offset > image->size || image->size - data_offset < data_size) return false;

    *out_data = image->data + data_offset;
    *out_size = data_size;
    return true;
}

static bool collect_rcdata(const PeImage* image, size_t resource_base, ResourceTable* out_table) {
    size_t type_total = 0;
    if (!resource_entry_count(image, resource_base, &type_total)) return false;

    for (size_t t = 0; t < type_total; t++) {
        uint32_t type_id = 0;
        uint32_t type_offset = 0;
        bool type_is_directory = false;
        bool type_is_named = false;
        if (!resource_entry_at(image, resource_base, t, &type_id, &type_offset,
                               &type_is_directory, &type_is_named)) {
            return false;
        }
        if (type_is_named || type_id != RESOURCE_TYPE_RCDATA || !type_is_directory) continue;

        const size_t name_base = resource_base + type_offset;
        size_t name_total = 0;
        if (!resource_entry_count(image, name_base, &name_total)) return false;

        for (size_t n = 0; n < name_total; n++) {
            uint32_t name_id = 0;
            uint32_t name_offset = 0;
            bool name_is_directory = false;
            bool name_is_named = false;
            if (!resource_entry_at(image, name_base, n, &name_id, &name_offset,
                                   &name_is_directory, &name_is_named)) {
                return false;
            }
            if (name_is_named || !name_is_directory || name_id >= MAX_RESOURCE_ID) continue;

            const size_t language_base = resource_base + name_offset;
            size_t language_total = 0;
            if (!resource_entry_count(image, language_base, &language_total)) return false;

            for (size_t l = 0; l < language_total; l++) {
                uint32_t language_id = 0;
                uint32_t language_offset = 0;
                bool language_is_directory = false;
                bool language_is_named = false;
                if (!resource_entry_at(image, language_base, l, &language_id, &language_offset,
                                       &language_is_directory, &language_is_named)) {
                    return false;
                }
                if (language_is_directory) continue;

                const uint8_t* data = NULL;
                uint32_t size = 0;
                if (!resource_read_leaf(image, resource_base + language_offset, &data, &size)) {
                    continue;
                }
                out_table->data[name_id] = data;
                out_table->size[name_id] = size;
                break;
            }
        }
    }
    return true;
}

static bool pe_slurp(int fd, PeImage* out_image, size_t size) {
    uint8_t* buffer = static_cast<uint8_t*>(malloc(size));
    if (!buffer) return false;

    size_t filled = 0;
    while (filled < size) {
        const ssize_t got = pread(fd, buffer + filled, size - filled, static_cast<off_t>(filled));
        if (got <= 0) {
            free(buffer);
            return false;
        }
        filled += static_cast<size_t>(got);
    }

    out_image->data = buffer;
    out_image->size = size;
    out_image->heap_backed = true;
    return true;
}

static bool pe_adopt(int fd, PeImage* out_image, int owned_fd) {
    memset(out_image, 0, sizeof(*out_image));
    out_image->owned_fd = owned_fd;

    struct stat info;
    if (fstat(fd, &info) != 0 || info.st_size <= 0) return false;

    const size_t size = static_cast<size_t>(info.st_size);
    void* mapped = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) return pe_slurp(fd, out_image, size);

    out_image->data = static_cast<const uint8_t*>(mapped);
    out_image->size = size;
    return true;
}

static bool pe_open_fd(int fd, PeImage* out_image) {
    if (fd < 0) {
        memset(out_image, 0, sizeof(*out_image));
        out_image->owned_fd = -1;
        return false;
    }
    return pe_adopt(fd, out_image, -1);
}

static bool pe_open(const char* path, PeImage* out_image) {
    const int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        memset(out_image, 0, sizeof(*out_image));
        out_image->owned_fd = -1;
        return false;
    }

    if (!pe_adopt(fd, out_image, fd)) {
        close(fd);
        out_image->owned_fd = -1;
        return false;
    }
    return true;
}

static void pe_close(PeImage* image) {
    if (image->sections) {
        free(image->sections);
        image->sections = NULL;
    }
    if (image->data) {
        if (image->heap_backed) {
            free(const_cast<uint8_t*>(image->data));
        } else {
            munmap(const_cast<uint8_t*>(image->data), image->size);
        }
    }
    if (image->owned_fd >= 0) close(image->owned_fd);
    image->data = NULL;
    image->size = 0;
    image->owned_fd = -1;
}

static LsfgStatus parse_resources(PeImage* image, ResourceTable* table) {
    size_t pe_offset = 0;
    if (!pe_find_header(image, &pe_offset)) return LSFG_NOT_PORTABLE_EXECUTABLE;
    if (!pe_read_sections(image, pe_offset)) return LSFG_NOT_PORTABLE_EXECUTABLE;

    size_t data_directory = 0;
    if (!pe_find_data_directory(image, pe_offset + 4 + COFF_HEADER_SIZE, &data_directory)) {
        return LSFG_NOT_PORTABLE_EXECUTABLE;
    }

    uint32_t resource_rva = 0;
    if (!pe_read_u32(image,
                     data_directory + RESOURCE_DATA_DIRECTORY_INDEX * DATA_DIRECTORY_ENTRY_SIZE,
                     &resource_rva) ||
        resource_rva == 0) {
        return LSFG_MISSING_SHADERS;
    }

    size_t resource_base = 0;
    if (!pe_rva_to_offset(image, resource_rva, &resource_base)) return LSFG_MISSING_SHADERS;
    if (!collect_rcdata(image, resource_base, table)) return LSFG_MISSING_SHADERS;
    return LSFG_OK;
}

static bool is_spirv_module(const uint8_t* blob, uint32_t size) {
    if (blob == NULL || size < SPIRV_HEADER_WORDS * sizeof(uint32_t)) return false;
    if (size % sizeof(uint32_t) != 0) return false;
    uint32_t magic = 0;
    memcpy(&magic, blob, sizeof(magic));
    return magic == SPIRV_MAGIC;
}

static uint32_t lookup_descriptor_set(const uint32_t* words, size_t word_count, uint32_t target) {
    size_t offset = SPIRV_HEADER_WORDS;
    while (offset < word_count) {
        const uint32_t length = words[offset] >> SPIRV_WORD_COUNT_SHIFT;
        const uint32_t opcode = words[offset] & SPIRV_OPCODE_MASK;
        if (length == 0 || offset + length > word_count) break;
        if (opcode == SPIRV_OP_FUNCTION) break;
        if (opcode == SPIRV_OP_DECORATE && length >= 4 &&
            words[offset + 2] == SPIRV_DECORATION_DESCRIPTOR_SET && words[offset + 1] == target) {
            return words[offset + 3];
        }
        offset += length;
    }
    return 0;
}

static bool renumber_bindings(uint32_t* words, size_t word_count) {
    size_t slot_count = 0;
    size_t offset = SPIRV_HEADER_WORDS;
    while (offset < word_count) {
        const uint32_t length = words[offset] >> SPIRV_WORD_COUNT_SHIFT;
        const uint32_t opcode = words[offset] & SPIRV_OPCODE_MASK;
        if (length == 0 || offset + length > word_count) return false;
        if (opcode == SPIRV_OP_FUNCTION) break;
        if (opcode == SPIRV_OP_DECORATE && length >= 4 &&
            words[offset + 2] == SPIRV_DECORATION_BINDING) {
            slot_count++;
        }
        offset += length;
    }
    if (slot_count == 0) return true;

    BindingSlot* slots = static_cast<BindingSlot*>(calloc(slot_count, sizeof(BindingSlot)));
    if (!slots) return false;

    size_t index = 0;
    offset = SPIRV_HEADER_WORDS;
    while (offset < word_count && index < slot_count) {
        const uint32_t length = words[offset] >> SPIRV_WORD_COUNT_SHIFT;
        const uint32_t opcode = words[offset] & SPIRV_OPCODE_MASK;
        if (length == 0 || offset + length > word_count) break;
        if (opcode == SPIRV_OP_FUNCTION) break;
        if (opcode == SPIRV_OP_DECORATE && length >= 4 &&
            words[offset + 2] == SPIRV_DECORATION_BINDING) {
            slots[index].binding = words[offset + 3];
            slots[index].literal_offset = offset + DECORATION_LITERAL_WORD;
            slots[index].set = lookup_descriptor_set(words, word_count, words[offset + 1]);
            index++;
        }
        offset += length;
    }

    if (index != slot_count) {
        free(slots);
        return false;
    }

    for (size_t i = 1; i < slot_count; i++) {
        const BindingSlot key = slots[i];
        size_t j = i;
        while (j > 0 && (slots[j - 1].set > key.set ||
                         (slots[j - 1].set == key.set && slots[j - 1].binding > key.binding))) {
            slots[j] = slots[j - 1];
            j--;
        }
        slots[j] = key;
    }

    for (size_t i = 0; i < slot_count; i++) {
        words[slots[i].literal_offset] = static_cast<uint32_t>(i);
    }

    free(slots);
    return true;
}

static bool adopt_spirv(const uint8_t* blob, uint32_t size, uint32_t** out_words,
                        uint32_t* out_word_count) {
    if (!is_spirv_module(blob, size)) return false;

    const size_t word_count = size / sizeof(uint32_t);
    if (word_count > MAX_SPIRV_WORDS) return false;

    uint32_t* words = static_cast<uint32_t*>(malloc(word_count * sizeof(uint32_t)));
    if (!words) return false;
    memcpy(words, blob, word_count * sizeof(uint32_t));

    if (!renumber_bindings(words, word_count)) {
        free(words);
        return false;
    }

    *out_words = words;
    *out_word_count = static_cast<uint32_t>(word_count);
    return true;
}

static bool has_native_variant(const ResourceTable* table, uint32_t variant_offset) {
    size_t count = 0;
    const uint32_t* ids = build_shader_ids(&count);
    for (size_t i = 0; i < count; i++) {
        const uint32_t id = ids[i] + variant_offset;
        if (id >= MAX_RESOURCE_ID) return false;
        if (!is_spirv_module(table->data[id], table->size[id])) return false;
    }
    return true;
}

static LsfgVariant select_variant(const ResourceTable* table, bool prefer_fp16) {
    if (prefer_fp16 && has_native_variant(table, VARIANT_FP16_OFFSET)) return LSFG_VARIANT_FP16;
    if (has_native_variant(table, VARIANT_FP32_OFFSET)) return LSFG_VARIANT_FP32;
    if (has_native_variant(table, VARIANT_FP16_OFFSET)) return LSFG_VARIANT_FP16;
    return LSFG_VARIANT_NONE;
}

static bool translate_base_shader(const ResourceTable* table, uint32_t id, uint32_t** out_words,
                                  uint32_t* out_word_count) {
    if (id >= MAX_RESOURCE_ID || table->data[id] == NULL) return false;
    return lsfg_translate_dxbc(table->data[id], table->size[id], out_words, out_word_count);
}

static bool has_base_chain(const ResourceTable* table) {
    size_t count = 0;
    const uint32_t* ids = build_shader_ids(&count);
    for (size_t i = 0; i < count; i++) {
        if (ids[i] >= MAX_RESOURCE_ID || table->data[ids[i]] == NULL) return false;
    }
    return true;
}

static const char* variant_name(LsfgVariant variant) {
    switch (variant) {
        case LSFG_VARIANT_FP16: return "spirv-fp16";
        case LSFG_VARIANT_FP32: return "spirv-fp32";
        case LSFG_VARIANT_DXBC: return "dxbc-translated";
        default: return "none";
    }
}

static uint32_t variant_offset(LsfgVariant variant) {
    return variant == LSFG_VARIANT_FP16 ? VARIANT_FP16_OFFSET : VARIANT_FP32_OFFSET;
}

static bool write_cache(const char* cache_path, const CacheHeader* header,
                        const LsfgModuleSet* set) {
    char temp_path[PATH_MAX];
    const int written = snprintf(temp_path, sizeof(temp_path), "%s.tmp", cache_path);
    if (written <= 0 || static_cast<size_t>(written) >= sizeof(temp_path)) return false;

    FILE* file = fopen(temp_path, "wb");
    if (!file) return false;

    bool ok = fwrite(header, sizeof(*header), 1, file) == 1;
    for (uint32_t i = 0; ok && i < set->count; i++) {
        const LsfgModule* module = &set->modules[i];
        ok = fwrite(&module->id, sizeof(module->id), 1, file) == 1 &&
             fwrite(&module->word_count, sizeof(module->word_count), 1, file) == 1 &&
             fwrite(module->words, sizeof(uint32_t), module->word_count, file) ==
                 module->word_count;
    }

    if (ok) ok = fflush(file) == 0;
    if (ok) ok = fsync(fileno(file)) == 0;
    fclose(file);

    if (!ok || rename(temp_path, cache_path) != 0) {
        unlink(temp_path);
        return false;
    }
    return true;
}

static LsfgStatus validate_image(PeImage* image) {
    ResourceTable* table = static_cast<ResourceTable*>(calloc(1, sizeof(ResourceTable)));
    if (!table) return LSFG_UNREADABLE_FILE;

    LsfgStatus status = parse_resources(image, table);
    if (status == LSFG_OK && !has_base_chain(table)) status = LSFG_MISSING_SHADERS;

    free(table);
    return status;
}

static LsfgStatus build_cache_from_image(PeImage* image, const char* cache_path,
                                         bool prefer_fp16);

LsfgStatus lsfg_validate_dll(const char* dll_path) {
    if (!dll_path) return LSFG_NOT_INSTALLED;

    PeImage image;
    if (!pe_open(dll_path, &image)) return LSFG_NOT_INSTALLED;

    const LsfgStatus status = validate_image(&image);
    pe_close(&image);
    return status;
}

LsfgStatus lsfg_validate_dll_fd(int fd) {
    PeImage image;
    if (!pe_open_fd(fd, &image)) return LSFG_NOT_INSTALLED;

    const LsfgStatus status = validate_image(&image);
    pe_close(&image);
    return status;
}

LsfgStatus lsfg_build_cache(const char* dll_path, const char* cache_path, bool prefer_fp16) {
    if (!dll_path || !cache_path) return LSFG_NOT_INSTALLED;

    PeImage image;
    if (!pe_open(dll_path, &image)) return LSFG_NOT_INSTALLED;

    const LsfgStatus status = build_cache_from_image(&image, cache_path, prefer_fp16);
    pe_close(&image);
    return status;
}

LsfgStatus lsfg_build_cache_fd(int fd, const char* cache_path, bool prefer_fp16) {
    if (!cache_path) return LSFG_NOT_INSTALLED;

    PeImage image;
    if (!pe_open_fd(fd, &image)) return LSFG_NOT_INSTALLED;

    const LsfgStatus status = build_cache_from_image(&image, cache_path, prefer_fp16);
    pe_close(&image);
    return status;
}

static LsfgStatus build_cache_from_image(PeImage* image_ptr, const char* cache_path,
                                         bool prefer_fp16) {
    PeImage& image = *image_ptr;

    ResourceTable* table = static_cast<ResourceTable*>(calloc(1, sizeof(ResourceTable)));
    if (!table) return LSFG_UNREADABLE_FILE;

    LsfgStatus status = parse_resources(&image, table);
    if (status != LSFG_OK) {
        free(table);
        return status;
    }

    const LsfgVariant variant = select_variant(table, prefer_fp16);
    const bool translate = variant == LSFG_VARIANT_NONE;
    if (translate && !has_base_chain(table)) {
        free(table);
        return LSFG_MISSING_SHADERS;
    }

    LsfgModuleSet set;
    memset(&set, 0, sizeof(set));
    set.variant = translate ? LSFG_VARIANT_DXBC : variant;

    const uint32_t offset = translate ? 0u : variant_offset(variant);
    size_t id_count = 0;
    const uint32_t* ids = build_shader_ids(&id_count);
    for (size_t i = 0; i < id_count; i++) {
        const uint32_t resource_id = ids[i] + offset;
        uint32_t* words = NULL;
        uint32_t word_count = 0;
        const bool ok = translate
            ? translate_base_shader(table, resource_id, &words, &word_count)
            : adopt_spirv(table->data[resource_id], table->size[resource_id], &words,
                          &word_count);
        if (!ok) {
            rsx_log.error("Frame generation: shader %u (%s) failed", ids[i],
                          translate ? "dxbc" : "spirv");
            status = LSFG_TRANSLATION_FAILED;
            break;
        }
        set.modules[set.count].id = ids[i];
        set.modules[set.count].words = words;
        set.modules[set.count].word_count = word_count;
        set.count++;
    }

    if (status == LSFG_OK) {
        CacheHeader header;
        memset(&header, 0, sizeof(header));
        header.magic = CACHE_MAGIC;
        header.version = CACHE_VERSION;
        header.source_size = static_cast<uint64_t>(image.size);
        header.source_hash = fnv1a64(image.data, image.size);
        header.module_count = set.count;
        header.variant = static_cast<uint32_t>(set.variant);

        if (!write_cache(cache_path, &header, &set)) status = LSFG_CACHE_UNUSABLE;
    }

    if (status == LSFG_OK) {
        rsx_log.notice("Frame generation: cached %u shader modules (source=%s)", set.count,
                       variant_name(set.variant));
    } else {
        rsx_log.error("Frame generation: shader cache build failed with status %d", static_cast<int>(status));
    }

    lsfg_release_modules(&set);
    free(table);
    return status;
}

LsfgStatus lsfg_cache_info(const char* cache_path, LsfgCacheInfo* out_info) {
    if (!cache_path || !out_info) return LSFG_CACHE_UNUSABLE;
    memset(out_info, 0, sizeof(*out_info));

    FILE* file = fopen(cache_path, "rb");
    if (!file) return LSFG_NOT_INSTALLED;

    CacheHeader header;
    const bool header_read = fread(&header, sizeof(header), 1, file) == 1;
    fclose(file);
    if (!header_read || header.magic != CACHE_MAGIC || header.version != CACHE_VERSION ||
        header.module_count != LSFG_SHADER_COUNT) {
        return LSFG_CACHE_UNUSABLE;
    }

    out_info->source_size = header.source_size;
    out_info->source_hash = header.source_hash;
    out_info->module_count = header.module_count;
    out_info->variant = static_cast<LsfgVariant>(header.variant);
    return LSFG_OK;
}

const char* lsfg_variant_name(LsfgVariant variant) {
    return variant_name(variant);
}

LsfgStatus lsfg_load_modules(const char* cache_path, LsfgModuleSet* out_set) {
    if (!cache_path || !out_set) return LSFG_CACHE_UNUSABLE;
    memset(out_set, 0, sizeof(*out_set));

    FILE* file = fopen(cache_path, "rb");
    if (!file) return LSFG_NOT_INSTALLED;

    CacheHeader header;
    if (fread(&header, sizeof(header), 1, file) != 1 || header.magic != CACHE_MAGIC ||
        header.version != CACHE_VERSION || header.module_count != LSFG_SHADER_COUNT) {
        fclose(file);
        return LSFG_CACHE_UNUSABLE;
    }

    out_set->variant = static_cast<LsfgVariant>(header.variant);

    LsfgStatus status = LSFG_OK;
    for (uint32_t i = 0; i < header.module_count; i++) {
        uint32_t id = 0;
        uint32_t word_count = 0;
        if (fread(&id, sizeof(id), 1, file) != 1 ||
            fread(&word_count, sizeof(word_count), 1, file) != 1 || word_count == 0 ||
            word_count > MAX_SPIRV_WORDS) {
            status = LSFG_CACHE_UNUSABLE;
            break;
        }

        uint32_t* words = static_cast<uint32_t*>(malloc(static_cast<size_t>(word_count) * sizeof(uint32_t)));
        if (!words) {
            status = LSFG_CACHE_UNUSABLE;
            break;
        }
        if (fread(words, sizeof(uint32_t), word_count, file) != word_count) {
            free(words);
            status = LSFG_CACHE_UNUSABLE;
            break;
        }

        out_set->modules[out_set->count].id = id;
        out_set->modules[out_set->count].words = words;
        out_set->modules[out_set->count].word_count = word_count;
        out_set->count++;
    }
    fclose(file);

    if (status == LSFG_OK) {
        size_t id_count = 0;
        const uint32_t* ids = build_shader_ids(&id_count);
        for (size_t i = 0; i < id_count; i++) {
            if (!lsfg_find_module(out_set, ids[i], NULL)) {
                status = LSFG_MISSING_SHADERS;
                break;
            }
        }
    }

    if (status != LSFG_OK) lsfg_release_modules(out_set);
    return status;
}

void lsfg_release_modules(LsfgModuleSet* set) {
    if (!set) return;
    for (uint32_t i = 0; i < set->count; i++) {
        free(set->modules[i].words);
        set->modules[i].words = NULL;
    }
    set->count = 0;
    set->variant = LSFG_VARIANT_NONE;
}

const uint32_t* lsfg_find_module(const LsfgModuleSet* set, uint32_t id,
                                 uint32_t* out_word_count) {
    if (!set) return NULL;
    for (uint32_t i = 0; i < set->count; i++) {
        if (set->modules[i].id != id) continue;
        if (out_word_count) *out_word_count = set->modules[i].word_count;
        return set->modules[i].words;
    }
    return NULL;
}
