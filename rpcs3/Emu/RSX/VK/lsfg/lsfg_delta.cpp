// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2025 lsfg-vk
// SPDX-License-Identifier: GPL-3.0-or-later

#include "lsfg_delta.hpp"
#include "lsfg_shaders.hpp"

#include <vector>

namespace lsfg {

namespace {

constexpr uint32_t DISPATCH_TILE_SHIFT = 3;

[[nodiscard]] uint32_t GroupCount(uint32_t size) {
    return (size + (1u << DISPATCH_TILE_SHIFT) - 1) >> DISPATCH_TILE_SHIFT;
}

}

LsfgDelta::LsfgDelta(const Device& device, const LsfgShaders& shaders, LsfgResources& resources,
                     VkDescriptorPool descriptor_pool, LsfgImageHistory& inputs_,
                     LsfgImage& flow_input_, LsfgImage* previous_gamma_, LsfgImage* previous1_,
                     LsfgImage* previous2_)
    : inputs{&inputs_}, flow_input{&flow_input_}, previous_gamma{previous_gamma_},
      previous1{previous1_}, previous2{previous2_} {
    passes[0] = LsfgPass(device, shaders, LSFG_DELTA_SHADERS[0],
                         {{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
                          {2, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {5, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    passes[1] = LsfgPass(device, shaders, LSFG_DELTA_SHADERS[1],
                         {{1, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {3, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    passes[2] = LsfgPass(device, shaders, LSFG_DELTA_SHADERS[2],
                         {{1, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    passes[3] = LsfgPass(device, shaders, LSFG_DELTA_SHADERS[3],
                         {{1, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    passes[4] = LsfgPass(device, shaders, LSFG_DELTA_SHADERS[4],
                         {{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
                          {2, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {4, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    passes[5] = LsfgPass(device, shaders, LSFG_DELTA_SHADERS[5],
                         {{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
                          {2, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {6, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    for (size_t i = 6; i < LSFG_DELTA_STAGES - 1; ++i) {
        passes[i] = LsfgPass(device, shaders, LSFG_DELTA_SHADERS[i],
                             {{1, VK_DESCRIPTOR_TYPE_SAMPLER},
                              {1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                              {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    }
    passes[9] = LsfgPass(device, shaders, LSFG_DELTA_SHADERS[9],
                         {{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
                          {2, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    for (const auto& pass : passes) {
        if (!pass.Valid()) return;
    }

    const VkExtent2D extent = (*inputs)[0][0].Extent();
    for (auto& image : temp1) {
        image = LsfgImage(device, extent);
        if (!image.Valid()) return;
    }
    for (auto& image : temp2) {
        image = LsfgImage(device, extent);
        if (!image.Valid()) return;
    }
    out_image1 = LsfgImage(device, extent, LSFG_MOTION_FORMAT);
    out_image2 = LsfgImage(device, extent, LSFG_MOTION_FORMAT);
    if (!out_image1.Valid() || !out_image2.Valid()) return;

    std::vector<VkDescriptorSetLayout> layouts;
    for (size_t slot = 0; slot < LSFG_GENERATION_SLOTS; ++slot) {
        for (size_t i = 0; i < LSFG_HISTORY_SLOTS; ++i) {
            layouts.push_back(passes[0].SetLayout());
        }
        for (size_t i = 1; i <= 4; ++i) {
            layouts.push_back(passes[i].SetLayout());
        }
        for (size_t i = 0; i < LSFG_HISTORY_SLOTS; ++i) {
            layouts.push_back(passes[5].SetLayout());
        }
        for (size_t i = 6; i < LSFG_DELTA_STAGES; ++i) {
            layouts.push_back(passes[i].SetLayout());
        }
    }

    const std::vector<VkDescriptorSet> sets =
        AllocateLsfgDescriptorSets(device, descriptor_pool, layouts);
    if (sets.size() != layouts.size()) return;

    const LsfgImage& dummy = resources.GetDummy(LSFG_MOTION_FORMAT);
    const LsfgImage& previous_gamma_image = previous_gamma != nullptr ? *previous_gamma : dummy;
    const LsfgImage& previous1_image = previous1 != nullptr ? *previous1 : dummy;
    const LsfgImage& previous2_image = previous2 != nullptr ? *previous2 : dummy;
    if (!dummy.Valid()) return;

    const VkSampler sampler = resources.GetSampler();
    const VkSampler border_sampler =
        resources.GetSampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_COMPARE_OP_NEVER, true);
    const VkSampler edge_sampler =
        resources.GetSampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_COMPARE_OP_ALWAYS, false);

    size_t next = 0;
    for (size_t slot = 0; slot < LSFG_GENERATION_SLOTS; ++slot) {
        Generation& pass = generations[slot];
        const VkBuffer buffer =
            resources.GetBuffer(LsfgSlotTimestamp(slot), false, previous_gamma == nullptr);

        for (size_t i = 0; i < LSFG_HISTORY_SLOTS; ++i) {
            pass.first_descriptor_sets[i] = sets[next++];
        }
        for (size_t i = 0; i < 4; ++i) {
            pass.descriptor_sets[i] = sets[next++];
        }
        for (size_t i = 0; i < LSFG_HISTORY_SLOTS; ++i) {
            pass.sixth_descriptor_sets[i] = sets[next++];
        }
        for (size_t i = 4; i < LSFG_DELTA_STAGES - 2; ++i) {
            pass.descriptor_sets[i] = sets[next++];
        }

        for (size_t i = 0; i < LSFG_HISTORY_SLOTS; ++i) {
            LsfgDescriptorWriter(pass.first_descriptor_sets[i])
                .AddUniformBuffer(buffer, LsfgResources::BufferSize())
                .AddSampler(border_sampler)
                .AddSampler(edge_sampler)
                .AddSampledImages((*inputs)[(i + 2) % LSFG_HISTORY_SLOTS])
                .AddSampledImages((*inputs)[i % LSFG_HISTORY_SLOTS])
                .AddSampledImage(previous_gamma_image)
                .AddStorageImages(temp1)
                .Build(device);
            LsfgDescriptorWriter(pass.sixth_descriptor_sets[i])
                .AddUniformBuffer(buffer, LsfgResources::BufferSize())
                .AddSampler(border_sampler)
                .AddSampler(edge_sampler)
                .AddSampledImages((*inputs)[(i + 2) % LSFG_HISTORY_SLOTS])
                .AddSampledImages((*inputs)[i % LSFG_HISTORY_SLOTS])
                .AddSampledImage(previous_gamma_image)
                .AddSampledImage(previous1_image)
                .AddStorageImage(temp2[0])
                .Build(device);
        }
        LsfgDescriptorWriter(pass.descriptor_sets[0])
            .AddSampler(sampler)
            .AddSampledImages(temp1)
            .AddStorageImages(temp2)
            .Build(device);
        LsfgDescriptorWriter(pass.descriptor_sets[1])
            .AddSampler(sampler)
            .AddSampledImages(temp2)
            .AddStorageImage(temp1[0])
            .AddStorageImage(temp1[1])
            .Build(device);
        LsfgDescriptorWriter(pass.descriptor_sets[2])
            .AddSampler(sampler)
            .AddSampledImage(temp1[0])
            .AddSampledImage(temp1[1])
            .AddStorageImages(temp2)
            .Build(device);
        LsfgDescriptorWriter(pass.descriptor_sets[3])
            .AddUniformBuffer(buffer, LsfgResources::BufferSize())
            .AddSampler(sampler)
            .AddSampler(edge_sampler)
            .AddSampledImages(temp2)
            .AddSampledImage(previous_gamma_image)
            .AddSampledImage(*flow_input)
            .AddStorageImage(out_image1)
            .Build(device);
        LsfgDescriptorWriter(pass.descriptor_sets[4])
            .AddSampler(sampler)
            .AddSampledImage(temp2[0])
            .AddStorageImage(temp1[0])
            .Build(device);
        LsfgDescriptorWriter(pass.descriptor_sets[5])
            .AddSampler(sampler)
            .AddSampledImage(temp1[0])
            .AddStorageImage(temp2[0])
            .Build(device);
        LsfgDescriptorWriter(pass.descriptor_sets[6])
            .AddSampler(sampler)
            .AddSampledImage(temp2[0])
            .AddStorageImage(temp1[0])
            .Build(device);
        LsfgDescriptorWriter(pass.descriptor_sets[7])
            .AddUniformBuffer(buffer, LsfgResources::BufferSize())
            .AddSampler(sampler)
            .AddSampler(edge_sampler)
            .AddSampledImage(temp1[0])
            .AddSampledImage(previous2_image)
            .AddStorageImage(out_image2)
            .Build(device);
    }
    allocated = true;
}

void LsfgDelta::PushStepBarriers(LsfgBarriers& barriers, uint64_t frame_count, size_t step) {
    const size_t history = frame_count % LSFG_HISTORY_SLOTS;
    const size_t previous_history = (frame_count + 2) % LSFG_HISTORY_SLOTS;

    switch (step) {
    case 0:
        barriers.WriteToReadAll((*inputs)[previous_history])
            .WriteToReadAll((*inputs)[history])
            .WriteToRead(previous_gamma)
            .ReadToWriteAll(temp1);
        break;
    case 1:
        barriers.WriteToReadAll(temp1).ReadToWriteAll(temp2);
        break;
    case 2:
        barriers.WriteToReadAll(temp2).ReadToWriteAll(temp1);
        break;
    case 3:
        barriers.WriteToReadAll(temp1).ReadToWriteAll(temp2);
        break;
    case 4:
        barriers.WriteToReadAll(temp2)
            .WriteToRead(previous_gamma)
            .WriteToRead(*flow_input)
            .ReadToWrite(out_image1);
        break;
    case 5:
        barriers.WriteToReadAll((*inputs)[previous_history])
            .WriteToReadAll((*inputs)[history])
            .WriteToRead(previous_gamma)
            .WriteToRead(previous1)
            .ReadToWriteAll(temp2);
        break;
    case 6:
        barriers.WriteToReadAll(temp2).ReadToWrite(temp1[0]).ReadToWrite(temp1[1]);
        break;
    case 7:
        barriers.WriteToRead(temp1[0]).WriteToRead(temp1[1]).ReadToWriteAll(temp2);
        break;
    case 8:
        barriers.WriteToReadAll(temp2).ReadToWrite(temp1[0]).ReadToWrite(temp1[1]);
        break;
    default:
        barriers.WriteToRead(temp1[0])
            .WriteToRead(temp1[1])
            .WriteToRead(previous2)
            .ReadToWrite(out_image2);
        break;
    }
}

void LsfgDelta::DispatchStep(VkCommandBuffer cmdbuf, uint64_t frame_count, size_t slot,
                             size_t step) {
    const Generation& pass = generations[slot];
    const VkExtent2D extent = temp1[0].Extent();
    const size_t history = frame_count % LSFG_HISTORY_SLOTS;

    if (step == 0) {
        passes[0].Bind(cmdbuf, pass.first_descriptor_sets[history]);
    } else if (step == 5) {
        passes[5].Bind(cmdbuf, pass.sixth_descriptor_sets[history]);
    } else if (step < 5) {
        passes[step].Bind(cmdbuf, pass.descriptor_sets[step - 1]);
    } else {
        passes[step].Bind(cmdbuf, pass.descriptor_sets[step - 2]);
    }
    VK_GET_SYMBOL(vkCmdDispatch)(cmdbuf, GroupCount(extent.width), GroupCount(extent.height), 1);
}

void LsfgDelta::Dispatch(VkCommandBuffer cmdbuf, uint64_t frame_count, size_t slot) {
    for (size_t step = 0; step < LSFG_DELTA_STAGES; ++step) {
        LsfgBarriers barriers(cmdbuf);
        PushStepBarriers(barriers, frame_count, step);
        barriers.Build();
        DispatchStep(cmdbuf, frame_count, slot, step);
    }
}

}
