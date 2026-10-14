/*
 * PS5_FrameGen - the generator's device parts: the context and the passes it
 * records (framegen.h states the contract).
 *
 * One descriptor set layout serves every method (shaders/common.glsl): five
 * sampled inputs and the storage output. Each method is one compute pipeline
 * built from SPIR-V compiled into the library (compile-shaders.sh).
 *
 * Copyright (C) 2026 Mihawk
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <cstdlib>
#include "framegen.h"
#include "fg_neural.h"
#include "fg_splat.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <new>

#include "shaders/blend.comp.h"
#include "shaders/mvwarp.comp.h"
#include "shaders/repeat.comp.h"

namespace {

constexpr uint32_t kMaxSlots = 8;
constexpr uint32_t kInputBindings = 5; // prev/next colour, prev/next depth, next motion
constexpr uint32_t kOutputBinding = 5;
constexpr uint32_t kGroupSize = 8;     // local_size in shaders/common.glsl

struct PushConstants {
	uint32_t width;
	uint32_t height;
	float t;
	uint32_t source_width;   // FG_METHOD_REPEAT: the frame's part of the image, 0 for all of it
	uint32_t source_height;
};

struct MethodCode {
	const uint32_t *words;
	size_t bytes;
};

// The SPIR-V of each method, in fg_method order
const MethodCode kMethodCode[FG_METHOD_COUNT] = {
	{ fg_spv_repeat, sizeof(fg_spv_repeat) },
	{ fg_spv_blend, sizeof(fg_spv_blend) },
	{ fg_spv_mvwarp, sizeof(fg_spv_mvwarp) },
	{ nullptr, 0 }, // neural: its own passes (fg_neural.cpp)
	{ nullptr, 0 }, // mvsplat: its own passes (fg_splat.cpp)
};

void say_error(char *error, size_t size, const char *format, ...)
{
	if (!error || size == 0) {
		return;
	}
	va_list args;
	va_start(args, format);
	vsnprintf(error, size, format, args);
	va_end(args);
}

} // namespace

struct fg_context {
	VkDevice device{ VK_NULL_HANDLE };
	uint32_t width{ 0 };
	uint32_t height{ 0 };
	uint32_t slots{ 0 };
	uint32_t source_width{ 0 };
	uint32_t source_height{ 0 };
	VkSampler linearSampler{ VK_NULL_HANDLE };  // colour and motion
	VkSampler nearestSampler{ VK_NULL_HANDLE }; // depth
	VkDescriptorSetLayout setLayout{ VK_NULL_HANDLE };
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
	VkPipeline pipelines[FG_METHOD_COUNT]{};
	VkDescriptorPool pool{ VK_NULL_HANDLE };
	// A call's bindings go in the next set of a ring, never one the GPU may still be reading (the PS5's driver
	// makes such an update wait for the GPU)
	static constexpr uint32_t kRing = 8;
	VkDescriptorSet sets[kMaxSlots][kRing][FG_METHOD_COUNT]{};
	uint32_t ring[kMaxSlots][FG_METHOD_COUNT]{};
	VkPhysicalDevice physicalDevice{ VK_NULL_HANDLE };
	fg_neural *neural{ nullptr };
	fg_splat *splat{ nullptr };
};

void fg_context_destroy(fg_context *ctx)
{
	if (!ctx) {
		return;
	}
	fg_neural_destroy(ctx->neural);
	fg_splat_destroy(ctx->splat);
	// Sets go with their pool; every handle may still be null after a failed create
	if (ctx->pool) {
		vkDestroyDescriptorPool(ctx->device, ctx->pool, nullptr);
	}
	for (VkPipeline pipeline : ctx->pipelines) {
		if (pipeline) {
			vkDestroyPipeline(ctx->device, pipeline, nullptr);
		}
	}
	if (ctx->pipelineLayout) {
		vkDestroyPipelineLayout(ctx->device, ctx->pipelineLayout, nullptr);
	}
	if (ctx->setLayout) {
		vkDestroyDescriptorSetLayout(ctx->device, ctx->setLayout, nullptr);
	}
	if (ctx->linearSampler) {
		vkDestroySampler(ctx->device, ctx->linearSampler, nullptr);
	}
	if (ctx->nearestSampler) {
		vkDestroySampler(ctx->device, ctx->nearestSampler, nullptr);
	}
	delete ctx;
}

VkResult fg_context_create(const fg_create_info *info, fg_context **out, char *error, size_t error_size)
{
	if (out) {
		*out = nullptr;
	}
	if (const char *problem = fg_check_create_info(info)) {
		say_error(error, error_size, "fg_context_create: %s", problem);
		return VK_ERROR_INITIALIZATION_FAILED;
	}
	if (!out) {
		say_error(error, error_size, "fg_context_create: no out pointer");
		return VK_ERROR_INITIALIZATION_FAILED;
	}
	fg_context *ctx = new (std::nothrow) fg_context;
	if (!ctx) {
		say_error(error, error_size, "fg_context_create: out of host memory");
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}
	ctx->device = info->device;
	ctx->physicalDevice = info->physical_device;
	ctx->width = info->width;
	ctx->source_width = info->source_width;
	ctx->source_height = info->source_height;
	ctx->height = info->height;
	ctx->slots = info->slots;
	VkResult result = VK_SUCCESS;
#define FG_CALL(call)                                                                \
	do {                                                                             \
		result = (call);                                                             \
		if (result != VK_SUCCESS) {                                                  \
			say_error(error, error_size, "fg_context_create: %s returned %d", #call, \
				static_cast<int>(result));                                           \
			fg_context_destroy(ctx);                                                 \
			return result;                                                           \
		}                                                                            \
	} while (0)

	// Colour and motion: bilinear and clamped, for methods that sample at warped
	// positions. Depth: nearest, since depth formats need not support linear
	// filtering (and blending depths across an edge invents surfaces).
	VkSamplerCreateInfo samplerInfo{
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.maxLod = 0.0f,
	};
	FG_CALL(vkCreateSampler(ctx->device, &samplerInfo, nullptr, &ctx->linearSampler));
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	FG_CALL(vkCreateSampler(ctx->device, &samplerInfo, nullptr, &ctx->nearestSampler));

	VkDescriptorSetLayoutBinding bindings[kInputBindings + 1]{};
	for (uint32_t i = 0; i < kInputBindings; i++) {
		bindings[i] = { i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };
	}
	bindings[kOutputBinding] = { kOutputBinding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };
	const VkDescriptorSetLayoutCreateInfo setLayoutInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = kInputBindings + 1,
		.pBindings = bindings,
	};
	FG_CALL(vkCreateDescriptorSetLayout(ctx->device, &setLayoutInfo, nullptr, &ctx->setLayout));

	const VkPushConstantRange pushRange{ VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstants) };
	const VkPipelineLayoutCreateInfo pipelineLayoutInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &ctx->setLayout,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &pushRange,
	};
	FG_CALL(vkCreatePipelineLayout(ctx->device, &pipelineLayoutInfo, nullptr, &ctx->pipelineLayout));

	for (uint32_t method = 0; method < FG_METHOD_COUNT; method++) {
		if (!kMethodCode[method].words) {
			continue;
		}
		const VkShaderModuleCreateInfo moduleInfo{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = kMethodCode[method].bytes,
			.pCode = kMethodCode[method].words,
		};
		VkShaderModule module = VK_NULL_HANDLE;
		FG_CALL(vkCreateShaderModule(ctx->device, &moduleInfo, nullptr, &module));
		const VkComputePipelineCreateInfo pipelineInfo{
			.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			.stage = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_COMPUTE_BIT,
				.module = module,
				.pName = "main",
			},
			.layout = ctx->pipelineLayout,
		};
		result = vkCreateComputePipelines(ctx->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &ctx->pipelines[method]);
		vkDestroyShaderModule(ctx->device, module, nullptr);
		FG_CALL(result);
	}

	const uint32_t setCount = ctx->slots * FG_METHOD_COUNT * fg_context::kRing;
	const VkDescriptorPoolSize poolSizes[] = {
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, setCount * kInputBindings },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, setCount },
	};
	const VkDescriptorPoolCreateInfo poolInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = setCount,
		.poolSizeCount = 2,
		.pPoolSizes = poolSizes,
	};
	FG_CALL(vkCreateDescriptorPool(ctx->device, &poolInfo, nullptr, &ctx->pool));
	for (uint32_t slot = 0; slot < ctx->slots; slot++)
	for (uint32_t r = 0; r < fg_context::kRing; r++) {
		VkDescriptorSetLayout layouts[FG_METHOD_COUNT];
		for (VkDescriptorSetLayout &layout : layouts) {
			layout = ctx->setLayout;
		}
		const VkDescriptorSetAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = ctx->pool,
			.descriptorSetCount = FG_METHOD_COUNT,
			.pSetLayouts = layouts,
		};
		FG_CALL(vkAllocateDescriptorSets(ctx->device, &allocInfo, ctx->sets[slot][r]));
	}
#undef FG_CALL
	*out = ctx;
	return VK_SUCCESS;
}

void fg_record_generate(fg_context *ctx, VkCommandBuffer cmd, uint32_t slot, fg_method method,
	const fg_frame_pair *pair, VkImageView output)
{
	// A wrong call is the program's bug: record nothing rather than fault the GPU
	if (!ctx || !pair || slot >= ctx->slots || method >= FG_METHOD_COUNT || output == VK_NULL_HANDLE) {
		return;
	}
	if (method == FG_METHOD_NEURAL) {
		fg_neural_record(ctx->neural, cmd, slot, pair, output);
		return;
	}
	if (method == FG_METHOD_MVSPLAT) {
		fg_splat_record(ctx->splat, cmd, slot, pair, output);
		return;
	}
	const VkImageView inputs[kInputBindings] = { pair->prev_color, pair->next_color, pair->prev_depth,
		pair->next_depth, pair->next_motion };
	VkDescriptorImageInfo imageInfos[kInputBindings + 1]{};
	VkWriteDescriptorSet writes[kInputBindings + 1]{};
	const uint32_t r = ctx->ring[slot][method];
	ctx->ring[slot][method] = (r + 1) % fg_context::kRing;
	const VkDescriptorSet set = ctx->sets[slot][r][method];
	for (uint32_t i = 0; i < kInputBindings; i++) {
		const bool depth = i == 2 || i == 3;
		imageInfos[i] = { depth ? ctx->nearestSampler : ctx->linearSampler, inputs[i],
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		writes[i] = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = set, .dstBinding = i,
			.descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &imageInfos[i] };
	}
	imageInfos[kOutputBinding] = { VK_NULL_HANDLE, output, VK_IMAGE_LAYOUT_GENERAL };
	writes[kOutputBinding] = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = set,
		.dstBinding = kOutputBinding, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.pImageInfo = &imageInfos[kOutputBinding] };
	vkUpdateDescriptorSets(ctx->device, kInputBindings + 1, writes, 0, nullptr);

	const PushConstants push{ ctx->width, ctx->height, pair->t, pair->source_width, pair->source_height };
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, ctx->pipelines[method]);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, ctx->pipelineLayout, 0, 1, &set, 0, nullptr);
	vkCmdPushConstants(cmd, ctx->pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), &push);
	vkCmdDispatch(cmd, (ctx->width + kGroupSize - 1) / kGroupSize, (ctx->height + kGroupSize - 1) / kGroupSize, 1);
}

VkResult fg_context_load_network(fg_context *ctx, const void *blob, size_t size, char *error, size_t error_size)
{
	if (!ctx || !blob || ctx->physicalDevice == VK_NULL_HANDLE) {
		say_error(error, error_size, "fg_context_load_network: needs a context made with a physical_device, and weights");
		return VK_ERROR_INITIALIZATION_FAILED;
	}
	const fg_neural_info info{ ctx->device, ctx->physicalDevice, ctx->width, ctx->height, ctx->slots, ctx->source_width, ctx->source_height };
	fg_neural *neural = fg_neural_create(&info, blob, size, error, error_size);
	if (!neural) {
		return VK_ERROR_INITIALIZATION_FAILED;
	}
	fg_neural_destroy(ctx->neural);
	ctx->neural = neural;
	return VK_SUCCESS;
}

bool fg_method_available(const fg_context *ctx, fg_method method)
{
	if (!ctx || method >= FG_METHOD_COUNT) {
		return false;
	}
	if (method == FG_METHOD_MVSPLAT) {
		return ctx->splat != nullptr;
	}
	return method != FG_METHOD_NEURAL || ctx->neural != nullptr;
}

VkResult fg_context_enable_mvsplat(fg_context *ctx, char *error, size_t error_size)
{
	if (!ctx || ctx->physicalDevice == VK_NULL_HANDLE) {
		say_error(error, error_size, "fg_context_enable_mvsplat: needs a context made with a physical_device");
		return VK_ERROR_INITIALIZATION_FAILED;
	}
	if (ctx->splat) {
		return VK_SUCCESS;
	}
	const fg_splat_info info{ ctx->device, ctx->physicalDevice, ctx->width, ctx->height, ctx->slots };
	ctx->splat = fg_splat_create(&info, error, error_size);
	return ctx->splat ? VK_SUCCESS : VK_ERROR_INITIALIZATION_FAILED;
}

uint32_t fg_mvsplat_pass_count(const fg_context *ctx)
{
	return ctx ? fg_splat_pass_count(ctx->splat) : 0;
}

const char *fg_mvsplat_pass_label(const fg_context *ctx, uint32_t pass)
{
	return ctx ? fg_splat_pass_label(ctx->splat, pass) : "";
}

void fg_context_set_profile(fg_context *ctx, VkQueryPool pool, uint32_t base)
{
	if (ctx) {
		fg_neural_set_profile(ctx->neural, pool, base);
		fg_splat_set_profile(ctx->splat, pool, base);
	}
}

uint32_t fg_neural_pass_count(const fg_context *ctx)
{
	return ctx ? fg_neural_pass_count(ctx->neural) : 0;
}

const char *fg_neural_pass_label(const fg_context *ctx, uint32_t pass)
{
	return ctx ? fg_neural_pass_label(ctx->neural, pass) : "";
}
