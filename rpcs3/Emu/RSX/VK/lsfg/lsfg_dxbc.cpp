#include "lsfg_dxbc.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <dxbc_modinfo.h>
#include <dxbc_module.h>
#include <dxbc_reader.h>
#include <thirdparty/spirv.hpp>

namespace {

struct BindingOffsets {
    uint32_t bindingIndex{};
    uint32_t bindingOffset{};
    uint32_t setIndex{};
    uint32_t setOffset{};
};

}

bool lsfg_translate_dxbc(const uint8_t* bytecode, uint32_t size, uint32_t** out_words,
                         uint32_t* out_word_count) {
    if (!bytecode || size < 4 || !out_words || !out_word_count) return false;

    try {
        dxvk::DxbcReader reader(reinterpret_cast<const char*>(bytecode), size);
        dxvk::DxbcModule module(reader);
        const dxvk::DxbcModuleInfo info{};
        auto code = module.compile(info, "CS");

        std::vector<BindingOffsets> bindingOffsets;
        std::vector<uint32_t> varIds;
        for (auto ins : code) {
            if (ins.opCode() == spv::OpDecorate) {
                if (ins.arg(2) == spv::DecorationBinding) {
                    const uint32_t varId = ins.arg(1);
                    bindingOffsets.resize(std::max(bindingOffsets.size(),
                                                   static_cast<size_t>(varId + 1)));
                    bindingOffsets[varId].bindingIndex = ins.arg(3);
                    bindingOffsets[varId].bindingOffset = ins.offset() + 3;
                    varIds.push_back(varId);
                }

                if (ins.arg(2) == spv::DecorationDescriptorSet) {
                    const uint32_t varId = ins.arg(1);
                    bindingOffsets.resize(std::max(bindingOffsets.size(),
                                                   static_cast<size_t>(varId + 1)));
                    bindingOffsets[varId].setIndex = ins.arg(3);
                    bindingOffsets[varId].setOffset = ins.offset() + 3;
                }
            }

            if (ins.opCode() == spv::OpFunction) break;
        }

        std::vector<BindingOffsets> validBindings;
        for (const auto varId : varIds) {
            const auto slot = bindingOffsets[varId];
            if (slot.bindingOffset) validBindings.push_back(slot);
        }

        for (size_t i = 0; i < validBindings.size(); i++) {
            code.data()[validBindings.at(i).bindingOffset] = static_cast<uint32_t>(i);
        }

        const size_t byte_size = code.size();
        if (byte_size == 0 || byte_size % sizeof(uint32_t) != 0) return false;

        const size_t word_count = byte_size / sizeof(uint32_t);
        auto* words = static_cast<uint32_t*>(std::malloc(byte_size));
        if (!words) return false;
        std::memcpy(words, code.data(), byte_size);

        *out_words = words;
        *out_word_count = static_cast<uint32_t>(word_count);
        return true;
    } catch (...) {
        return false;
    }
}

void lsfg_free_translated(uint32_t* words) {
    std::free(words);
}
