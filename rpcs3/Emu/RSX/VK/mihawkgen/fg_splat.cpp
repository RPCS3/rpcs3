/*
 * PS5_FrameGen - method "mvsplat": motion-vector interpolation by a forward-splatted vector field (shaders/sp_*.comp).
 *
 * The next frame's vectors are carried to the instant t with a depth test (the nearest surface wins), what no surface
 * reached takes the farthest surface around, both frames are read through the field with a depth check for what is
 * covered or uncovered, and what neither frame shows is filled from the pixels around. It owns images (one set for each
 * slot) kept in VK_IMAGE_LAYOUT_GENERAL and records the barriers between its passes; the caller's contract for its inputs and
 * the output is framegen.h's. docs/design.md says where each idea comes from.
 *
 * Copyright (C) 2026 Mihawk
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "fg_splat.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <new>
#include <string>
#include <vector>

#include "shaders/sp_clear.comp.h"
#include "shaders/sp_depth.comp.h"
#include "shaders/sp_down.comp.h"
#include "shaders/sp_fill.comp.h"
#include "shaders/sp_final.comp.h"
#include "shaders/sp_gather.comp.h"
#include "shaders/sp_up.comp.h"
#include "shaders/sp_vec.comp.h"

namespace {

constexpr uint32_t kGroup = 8;
constexpr uint32_t kRing = 8;       // sets for a call's own bindings, rotated: one the GPU may still read is never rewritten
constexpr uint32_t kLevels = 6;     // the fill's pyramid: 6 levels above the frame, 64 pixels
constexpr uint32_t kFillSteps = 7;  // 64, 32, ... 1
constexpr float kDefaultTolerance = 0.05f;
constexpr uint32_t kSamplers = 5;   // bindings 0 to 4
constexpr uint32_t kBindings = 7;   // 5 and 6 are storage images

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

uint32_t ceil_div(uint32_t a, uint32_t b) { return (a + b - 1) / b; }

struct Image {
	VkImage image{ VK_NULL_HANDLE };
	VkDeviceMemory memory{ VK_NULL_HANDLE };
	VkImageView view{ VK_NULL_HANDLE };
	uint32_t w{ 0 }, h{ 0 };
};

enum Kind { kClear, kDepth, kVec, kFill, kGather, kDown, kUp, kFinal, kKinds };

struct PushBlock {
	int32_t w, h;
	float t;
	int32_t step, first, top;
	float tol;
	int32_t inverted;
	float near_z, far_z;
};

struct Pass {
	Kind kind;
	VkDescriptorSet set[kRing]{};  // a pass with no binding of the call's own has the same set in every entry
	PushBlock push;
	uint32_t w, h;
	std::string label;
};

struct SlotImages {
	Image winning;               // R32_UINT: the nearest depth that arrived at each pixel
	Image field[2];              // RGBA16F: displacement (x, y), depth, "reached"
	Image result;                // RGBA16F: the readings' colour, and whether any was trusted
	Image sums[kLevels + 1];     // [1..]: the pyramid's sums, premultiplied, alpha the count
	Image filled[kLevels + 1];   // [1..]: its fill
	std::vector<Pass> passes;
	uint32_t ring{ 0 };
	bool initialised{ false };
};

uint32_t find_memory(const VkPhysicalDeviceMemoryProperties &mem, uint32_t bits, VkMemoryPropertyFlags want)
{
	for (uint32_t i = 0; i < mem.memoryTypeCount; i++) {
		if ((bits & (1u << i)) && (mem.memoryTypes[i].propertyFlags & want) == want) {
			return i;
		}
	}
	return UINT32_MAX;
}

void destroy_image(VkDevice device, Image &i)
{
	if (i.view) vkDestroyImageView(device, i.view, nullptr);
	if (i.image) vkDestroyImage(device, i.image, nullptr);
	if (i.memory) vkFreeMemory(device, i.memory, nullptr);
	i = {};
}

} // namespace

struct fg_splat {
	VkDevice device{ VK_NULL_HANDLE };
	VkPhysicalDeviceMemoryProperties memory{};
	uint32_t width{ 0 }, height{ 0 }, slots{ 0 };
	VkSampler linear{ VK_NULL_HANDLE };   // colour
	VkSampler nearest{ VK_NULL_HANDLE };  // depth, motion, and the method's own images
	VkDescriptorSetLayout set_layout{ VK_NULL_HANDLE };
	VkPipelineLayout pipeline_layout{ VK_NULL_HANDLE };
	VkPipeline pipelines[kKinds]{};
	VkDescriptorPool pool{ VK_NULL_HANDLE };
	SlotImages slot[8];
	VkQueryPool profile_pool{ VK_NULL_HANDLE };
	uint32_t profile_base{ 0 };
};

void fg_splat_destroy(fg_splat *s)
{
	if (!s) {
		return;
	}
	VkDevice d = s->device;
	if (s->pool) vkDestroyDescriptorPool(d, s->pool, nullptr);
	for (SlotImages &si : s->slot) {
		destroy_image(d, si.winning);
		for (Image &i : si.field) destroy_image(d, i);
		destroy_image(d, si.result);
		for (Image &i : si.sums) destroy_image(d, i);
		for (Image &i : si.filled) destroy_image(d, i);
	}
	for (VkPipeline p : s->pipelines) if (p) vkDestroyPipeline(d, p, nullptr);
	if (s->pipeline_layout) vkDestroyPipelineLayout(d, s->pipeline_layout, nullptr);
	if (s->set_layout) vkDestroyDescriptorSetLayout(d, s->set_layout, nullptr);
	if (s->linear) vkDestroySampler(d, s->linear, nullptr);
	if (s->nearest) vkDestroySampler(d, s->nearest, nullptr);
	delete s;
}

fg_splat *fg_splat_create(const fg_splat_info *info, char *error, size_t error_size)
{
	if (!info || info->physical_device == VK_NULL_HANDLE || info->slots == 0 || info->slots > 8) {
		say_error(error, error_size, "fg_splat: needs a physical device and 1 to 8 slots");
		return nullptr;
	}
	fg_splat *s = new (std::nothrow) fg_splat;
	if (!s) {
		say_error(error, error_size, "fg_splat: out of host memory");
		return nullptr;
	}
	s->device = info->device;
	s->width = info->width;
	s->height = info->height;
	s->slots = info->slots;
	vkGetPhysicalDeviceMemoryProperties(info->physical_device, &s->memory);
	VkDevice d = s->device;
	VkResult result = VK_SUCCESS;
#define SP_CALL(call)                                                                                     \
	do {                                                                                                  \
		result = (call);                                                                                  \
		if (result != VK_SUCCESS) {                                                                       \
			say_error(error, error_size, "fg_splat: %s returned %d", #call, static_cast<int>(result));    \
			fg_splat_destroy(s);                                                                          \
			return nullptr;                                                                               \
		}                                                                                                 \
	} while (0)

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
	SP_CALL(vkCreateSampler(d, &samplerInfo, nullptr, &s->linear));
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	SP_CALL(vkCreateSampler(d, &samplerInfo, nullptr, &s->nearest));

	VkDescriptorSetLayoutBinding bindings[kBindings]{};
	for (uint32_t i = 0; i < kBindings; i++) {
		bindings[i] = { i, i < kSamplers ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };
	}
	const VkDescriptorSetLayoutCreateInfo layoutInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .bindingCount = kBindings, .pBindings = bindings };
	SP_CALL(vkCreateDescriptorSetLayout(d, &layoutInfo, nullptr, &s->set_layout));
	const VkPushConstantRange push{ VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushBlock) };
	const VkPipelineLayoutCreateInfo plInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, .setLayoutCount = 1, .pSetLayouts = &s->set_layout,
		.pushConstantRangeCount = 1, .pPushConstantRanges = &push };
	SP_CALL(vkCreatePipelineLayout(d, &plInfo, nullptr, &s->pipeline_layout));

	struct Code { const uint32_t *words; size_t bytes; };
	const Code code[kKinds] = { { fg_spv_sp_clear, sizeof(fg_spv_sp_clear) }, { fg_spv_sp_depth, sizeof(fg_spv_sp_depth) },
		{ fg_spv_sp_vec, sizeof(fg_spv_sp_vec) }, { fg_spv_sp_fill, sizeof(fg_spv_sp_fill) }, { fg_spv_sp_gather, sizeof(fg_spv_sp_gather) },
		{ fg_spv_sp_down, sizeof(fg_spv_sp_down) }, { fg_spv_sp_up, sizeof(fg_spv_sp_up) }, { fg_spv_sp_final, sizeof(fg_spv_sp_final) } };
	for (uint32_t k = 0; k < kKinds; k++) {
		const VkShaderModuleCreateInfo moduleInfo{ .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, .codeSize = code[k].bytes, .pCode = code[k].words };
		VkShaderModule module = VK_NULL_HANDLE;
		SP_CALL(vkCreateShaderModule(d, &moduleInfo, nullptr, &module));
		const VkComputePipelineCreateInfo pipelineInfo{ .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			.stage = { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_COMPUTE_BIT, .module = module, .pName = "main" },
			.layout = s->pipeline_layout };
		result = vkCreateComputePipelines(d, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &s->pipelines[k]);
		vkDestroyShaderModule(d, module, nullptr);
		SP_CALL(result);
	}

	auto makeImage = [&](Image &img, uint32_t w, uint32_t h, VkFormat format) -> VkResult {
		img.w = w;
		img.h = h;
		const VkImageCreateInfo imageInfo{ .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, .imageType = VK_IMAGE_TYPE_2D, .format = format,
			.extent = { w, h, 1 }, .mipLevels = 1, .arrayLayers = 1, .samples = VK_SAMPLE_COUNT_1_BIT, .tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED };
		VkResult r = vkCreateImage(d, &imageInfo, nullptr, &img.image);
		if (r != VK_SUCCESS) return r;
		VkMemoryRequirements mr;
		vkGetImageMemoryRequirements(d, img.image, &mr);
		const uint32_t mt = find_memory(s->memory, mr.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		if (mt == UINT32_MAX) return VK_ERROR_OUT_OF_DEVICE_MEMORY;
		const VkMemoryAllocateInfo ai{ .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .allocationSize = mr.size, .memoryTypeIndex = mt };
		r = vkAllocateMemory(d, &ai, nullptr, &img.memory);
		if (r != VK_SUCCESS) return r;
		r = vkBindImageMemory(d, img.image, img.memory, 0);
		if (r != VK_SUCCESS) return r;
		const VkImageViewCreateInfo viewInfo{ .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, .image = img.image, .viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = format, .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 } };
		return vkCreateImageView(d, &viewInfo, nullptr, &img.view);
	};

	// The sets: a pass of the call's own bindings has one for each entry of the ring, the others one
	const uint32_t per_slot = 1 + 1 + 1 + kFillSteps + 1 + kLevels + kLevels + 1;
	const uint32_t set_count = (per_slot + 4 * (kRing - 1)) * s->slots;
	const VkDescriptorPoolSize sizes[] = { { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, set_count * kSamplers }, { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, set_count * 2 } };
	const VkDescriptorPoolCreateInfo poolInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, .maxSets = set_count, .poolSizeCount = 2, .pPoolSizes = sizes };
	SP_CALL(vkCreateDescriptorPool(d, &poolInfo, nullptr, &s->pool));

	const uint32_t W = info->width, H = info->height;
	for (uint32_t sl = 0; sl < s->slots; sl++) {
		SlotImages &si = s->slot[sl];
		SP_CALL(makeImage(si.winning, W, H, VK_FORMAT_R32_UINT));
		for (Image &i : si.field) SP_CALL(makeImage(i, W, H, VK_FORMAT_R16G16B16A16_SFLOAT));
		SP_CALL(makeImage(si.result, W, H, VK_FORMAT_R16G16B16A16_SFLOAT));
		uint32_t lw = W, lh = H;
		for (uint32_t l = 1; l <= kLevels; l++) {
			lw = (lw + 1) / 2;
			lh = (lh + 1) / 2;
			SP_CALL(makeImage(si.sums[l], lw, lh, VK_FORMAT_R16G16B16A16_SFLOAT));
			SP_CALL(makeImage(si.filled[l], lw, lh, VK_FORMAT_R16G16B16A16_SFLOAT));
		}

		// Writes the bindings a pass has of its own images (sampled: 0 to 4; storage: 5, 6); a null view leaves the binding alone
		auto write = [&](VkDescriptorSet set, const VkImageView sampled[kSamplers], VkImageView store5, VkImageView store6) {
			VkDescriptorImageInfo infos[kBindings]{};
			VkWriteDescriptorSet writes[kBindings]{};
			uint32_t n = 0;
			for (uint32_t i = 0; i < kSamplers; i++) {
				if (!sampled[i]) continue;
				infos[n] = { s->nearest, sampled[i], VK_IMAGE_LAYOUT_GENERAL };
				writes[n] = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = set, .dstBinding = i, .descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = &infos[n] };
				n++;
			}
			const VkImageView stores[2] = { store5, store6 };
			for (uint32_t i = 0; i < 2; i++) {
				if (!stores[i]) continue;
				infos[n] = { VK_NULL_HANDLE, stores[i], VK_IMAGE_LAYOUT_GENERAL };
				writes[n] = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = set, .dstBinding = 5 + i, .descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .pImageInfo = &infos[n] };
				n++;
			}
			if (n) vkUpdateDescriptorSets(d, n, writes, 0, nullptr);
		};
		auto add = [&](Kind kind, uint32_t w, uint32_t h, const char *label, bool ringed, const VkImageView sampled[kSamplers], VkImageView store5,
			VkImageView store6, PushBlock push) -> VkResult {
			Pass p{};
			p.kind = kind;
			p.w = w;
			p.h = h;
			p.label = std::string(label) + " " + std::to_string(w) + "x" + std::to_string(h);
			push.w = int32_t(w);
			push.h = int32_t(h);
			p.push = push;
			for (uint32_t r = 0; r < (ringed ? kRing : 1); r++) {
				const VkDescriptorSetAllocateInfo ai{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .descriptorPool = s->pool,
					.descriptorSetCount = 1, .pSetLayouts = &s->set_layout };
				VkResult res = vkAllocateDescriptorSets(d, &ai, &p.set[r]);
				if (res != VK_SUCCESS) return res;
				write(p.set[r], sampled, store5, store6);
			}
			if (!ringed) for (uint32_t r = 1; r < kRing; r++) p.set[r] = p.set[0];
			si.passes.push_back(p);
			return VK_SUCCESS;
		};
		// (the call's own bindings, written on every call: 0 to 4 of a pass that reads the frames, 5 of the last)
		const VkImageView none[kSamplers]{};
		VkImageView in[kSamplers]{};
		const PushBlock blank{};
		SP_CALL(add(kClear, W, H, "clear", false, none, VK_NULL_HANDLE, si.winning.view, blank));
		SP_CALL(add(kDepth, W, H, "splat depth", true, none, VK_NULL_HANDLE, si.winning.view, blank));
		SP_CALL(add(kVec, W, H, "splat vector", true, none, si.field[0].view, si.winning.view, blank));
		for (uint32_t i = 0; i < kFillSteps; i++) {
			in[0] = si.field[i & 1].view;
			PushBlock fp = blank;
			fp.step = 64 >> i;
			fp.first = i == 0;
			SP_CALL(add(kFill, W, H, "fill", false, in, si.field[(i + 1) & 1].view, si.winning.view, fp));
		}
		const Image &fieldFinal = si.field[kFillSteps & 1];
		in[0] = in[1] = in[2] = in[3] = VK_NULL_HANDLE;
		in[4] = fieldFinal.view;
		SP_CALL(add(kGather, W, H, "gather", true, in, si.result.view, VK_NULL_HANDLE, blank));
		in[4] = VK_NULL_HANDLE;
		for (uint32_t l = 1; l <= kLevels; l++) {
			in[0] = l == 1 ? si.result.view : si.sums[l - 1].view;
			SP_CALL(add(kDown, si.sums[l].w, si.sums[l].h, "down", false, in, si.sums[l].view, VK_NULL_HANDLE, blank));
		}
		for (uint32_t l = kLevels; l >= 1; l--) {
			in[0] = si.sums[l].view;
			in[1] = l == kLevels ? si.sums[l].view : si.filled[l + 1].view;   // (the top level reads nothing coarser; the binding must still be valid)
			PushBlock up = blank;
			up.top = l == kLevels;
			SP_CALL(add(kUp, si.sums[l].w, si.sums[l].h, "up", false, in, si.filled[l].view, VK_NULL_HANDLE, up));
		}
		in[0] = si.result.view;
		in[1] = si.filled[1].view;
		SP_CALL(add(kFinal, W, H, "final", true, in, VK_NULL_HANDLE, VK_NULL_HANDLE, blank));
		for (uint32_t i = 0; i < kSamplers; i++) in[i] = VK_NULL_HANDLE;
	}
#undef SP_CALL
	return s;
}

void fg_splat_record(fg_splat *s, VkCommandBuffer cmd, uint32_t slot, const fg_frame_pair *pair, VkImageView output)
{
	if (!s || slot >= s->slots) {
		return;
	}
	VkDevice d = s->device;
	SlotImages &si = s->slot[slot];

	if (!si.initialised) {
		// The first call: every intermediate image to GENERAL, contents undefined
		std::vector<VkImageMemoryBarrier> barriers;
		auto add = [&](const Image &img) {
			barriers.push_back({ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, .srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_GENERAL, .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = img.image, .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 } });
		};
		add(si.winning);
		for (const Image &i : si.field) add(i);
		add(si.result);
		for (uint32_t l = 1; l <= kLevels; l++) {
			add(si.sums[l]);
			add(si.filled[l]);
		}
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
			static_cast<uint32_t>(barriers.size()), barriers.data());
		si.initialised = true;
	}

	// The call's own bindings go in the next set of the ring (see kRing)
	const uint32_t ring = si.ring;
	si.ring = (ring + 1) % kRing;
	std::vector<VkDescriptorImageInfo> infos;
	std::vector<VkWriteDescriptorSet> writes;
	infos.reserve(32);
	writes.reserve(32);
	auto sampled = [&](VkDescriptorSet set, uint32_t binding, VkImageView view, VkSampler sampler) {
		infos.push_back({ sampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
		writes.push_back({ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = set, .dstBinding = binding, .descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = &infos.back() });
	};
	auto stored = [&](VkDescriptorSet set, uint32_t binding, VkImageView view) {
		infos.push_back({ VK_NULL_HANDLE, view, VK_IMAGE_LAYOUT_GENERAL });
		writes.push_back({ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = set, .dstBinding = binding, .descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .pImageInfo = &infos.back() });
	};
	for (Pass &p : si.passes) {
		const VkDescriptorSet set = p.set[ring];
		if (p.kind == kDepth || p.kind == kVec) {
			sampled(set, 0, pair->next_motion, s->nearest);
			sampled(set, 1, pair->next_depth, s->nearest);
		} else if (p.kind == kGather) {
			sampled(set, 0, pair->prev_color, s->linear);
			sampled(set, 1, pair->next_color, s->linear);
			sampled(set, 2, pair->prev_depth, s->nearest);
			sampled(set, 3, pair->next_depth, s->nearest);
		} else if (p.kind == kFinal) {
			stored(set, 5, output);
		}
	}
	// (the vectors of infos are never reallocated: reserved above)
	// Each descriptor info pointer was taken from infos.back() at its push; the vector did not move
	vkUpdateDescriptorSets(d, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

	const VkMemoryBarrier barrier{ .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER, .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT };
	if (s->profile_pool) {
		vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, s->profile_pool, s->profile_base);
	}
	uint32_t index = 0;
	bool first = true;
	for (Pass &p : si.passes) {
		if (!first) {
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
		}
		first = false;
		PushBlock push = p.push;
		push.t = pair->t;
		push.tol = pair->depth_tolerance > 0.0f ? pair->depth_tolerance : kDefaultTolerance;
		push.inverted = int32_t(pair->depth_inverted);
		push.near_z = pair->depth_near;
		push.far_z = pair->depth_far;
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, s->pipelines[p.kind]);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, s->pipeline_layout, 0, 1, &p.set[ring], 0, nullptr);
		vkCmdPushConstants(cmd, s->pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), &push);
		vkCmdDispatch(cmd, ceil_div(p.w, kGroup), ceil_div(p.h, kGroup), 1);
		index++;
		if (s->profile_pool) {
			vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, s->profile_pool, s->profile_base + index);
		}
	}
}

void fg_splat_set_profile(fg_splat *s, VkQueryPool pool, uint32_t base)
{
	if (s) {
		s->profile_pool = pool;
		s->profile_base = base;
	}
}

uint32_t fg_splat_pass_count(const fg_splat *s)
{
	return s ? static_cast<uint32_t>(s->slot[0].passes.size()) : 0;
}

const char *fg_splat_pass_label(const fg_splat *s, uint32_t pass)
{
	return s && pass < s->slot[0].passes.size() ? s->slot[0].passes[pass].label.c_str() : "";
}
