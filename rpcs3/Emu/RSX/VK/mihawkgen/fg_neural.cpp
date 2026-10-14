/*
 * PS5_FrameGen - the neural method: the trained interpolation network
 * (train/model.py) as compute shaders.
 *
 * The network is read from the weight file train/export.py writes. It runs at a
 * fixed working size (288 rows, the width of the frames' aspect ratio rounded to
 * 16), on the two frames reduced to that size, and takes its flows to the frames'
 * full size for the warp and the blend (shaders/nn_final.comp). Its intermediate
 * images belong to the generator, one set for each slot, and are kept in
 * VK_IMAGE_LAYOUT_GENERAL: between its own passes it records the barriers it
 * needs. The caller's contract for the inputs and the output is framegen.h's.
 *
 * Copyright (C) 2026 Mihawk
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "fg_neural.h"

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>
#include <vector>

#include "shaders/nn_conv.comp.h"
#include "shaders/nn_downsample.comp.h"
#include "shaders/nn_field.comp.h"
#include "shaders/nn_final.comp.h"
#include "shaders/nn_motion.comp.h"
#include "shaders/nn_pool.comp.h"
#include "shaders/nn_prep.comp.h"
#include "shaders/nn_static.comp.h"
#include "shaders/nn_tiles.comp.h"
#include "shaders/nn_update.comp.h"

namespace {

constexpr uint32_t kMagic = 0x31574746; // "FGW1"
constexpr uint32_t kWorkingHeight = 288;

// The height the network works at. 288 rows unless FG_WORK_HEIGHT (a test knob) names another, rounded to a multiple of 16 (the coarsest
// block works at 1/16 of it). A frame is averaged down to it, the flow and mask made there, and both taken to the frame's full size by the
// last pass; the cost of the network grows with its area.
static uint32_t working_height()
{
	if (const char *e = std::getenv("FG_WORK_HEIGHT")) {
		const long v = std::atol(e);
		if (v >= 64 && v <= 2160) {
			return static_cast<uint32_t>((v + 8) / 16 * 16);
		}
	}
	return kWorkingHeight;
}
constexpr uint32_t kGroup = 8;
constexpr uint32_t kMaxBlocks = 4;
constexpr uint32_t kMaxGroupsPerLayer = 16; // the conv shader's accumulators (64 channels out)
constexpr VkFormat kFeatureFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

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
};

struct Conv {
	uint32_t cin, cout, leaky;
	uint32_t weight_base; // in vec4s
	uint32_t bias_base;
};

struct Block {
	uint32_t scale, width, convs, radius;
	uint32_t w, h; // the block's size: working size / scale
	std::vector<Conv> layers;
};

enum PassKind { kDownsample, kPool, kPrep, kConv, kUpdate, kFinal, kTiles, kMotion, kStatic, kField, kKinds };

constexpr uint32_t kRing = 8;  // sets rotated for a call's own bindings: one the GPU may still read is never rewritten

struct Pass {
	int32_t which{ -1 };           // kDownsample: which frame's set
	PassKind kind;
	VkPipeline pipeline;
	VkDescriptorSet set;
	uint32_t groups_x, groups_y, groups_z;
	uint32_t push[16];
	uint32_t push_size;
	std::string label;
};

struct SlotImages {
	Image work[2];                    // the frames at working size
	Image pooled[kMaxBlocks][2];      // at each block's scale
	Image features[kMaxBlocks][2];    // ping-pong arrays
	Image flow[kMaxBlocks], mask[kMaxBlocks];
	Image motion;                     // pixel art: the motion of whole source pixels at t (nn_motion)
	std::vector<Pass> passes;
	// Where a call's frames and output go in the passes' descriptor sets
	VkDescriptorSet down_sets[kRing][2]{};
	VkDescriptorSet final_sets[kRing]{};
	VkDescriptorSet tiles_sets[kRing]{};
	VkDescriptorSet motion_sets[kRing]{};
	VkDescriptorSet static_sets[kRing]{};
	VkDescriptorSet field_sets[kRing]{};
	uint32_t ring{ 0 };
	bool initialised{ false };
};

} // namespace

struct fg_neural {
	VkDevice device{ VK_NULL_HANDLE };
	VkPhysicalDeviceMemoryProperties memory{};
	uint32_t width{ 0 }, height{ 0 }, slots{ 0 };
	uint32_t work_w{ 0 }, work_h{ 0 };
	uint32_t src_w{ 0 }, src_h{ 0 };      // pixel art: the frames in source pixels (0: none)
	uint32_t grid_w{ 0 }, grid_h{ 0 };    // the tiles of the motion search
	Block blocks[kMaxBlocks];
	uint32_t block_count{ 0 };

	VkSampler sampler{ VK_NULL_HANDLE };
	VkDescriptorSetLayout set_layouts[kKinds]{};
	VkPipelineLayout pipeline_layouts[kKinds]{};
	VkPipeline pipelines[kKinds]{};
	std::vector<VkPipeline> conv_pipelines;
	VkDescriptorPool pool{ VK_NULL_HANDLE };

	VkBuffer weights{ VK_NULL_HANDLE };
	VkDeviceMemory weights_memory{ VK_NULL_HANDLE };

	bool tiles_init{ false };
	Image field;                      // pixel art: the displacement of each pixel of the first frame (nn_field), shared by the slots
	Image stable;                     // pixel art: for each source pixel the pairs of frames in a row it has not changed in (nn_static)
	Image tiles;                      // pixel art: the displacement found for each tile of the frames (nn_tiles), shared by the slots
	SlotImages slot[8];
	VkQueryPool profile_pool{ VK_NULL_HANDLE };
	uint32_t profile_base{ 0 };
};

namespace {

uint32_t find_memory(const VkPhysicalDeviceMemoryProperties &mem, uint32_t bits, VkMemoryPropertyFlags want, VkMemoryPropertyFlags avoid = 0)
{
	for (uint32_t i = 0; i < mem.memoryTypeCount; i++) {
		const VkMemoryPropertyFlags flags = mem.memoryTypes[i].propertyFlags;
		if ((bits & (1u << i)) && (flags & want) == want && !(flags & avoid)) {
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

void fg_neural_destroy(fg_neural *n)
{
	if (!n) {
		return;
	}
	VkDevice d = n->device;
	if (n->pool) vkDestroyDescriptorPool(d, n->pool, nullptr);
	destroy_image(d, n->tiles);
	destroy_image(d, n->stable);
	destroy_image(d, n->field);
	for (SlotImages &s : n->slot) {
		for (Image &i : s.work) destroy_image(d, i);
		for (auto &row : s.pooled) for (Image &i : row) destroy_image(d, i);
		for (auto &row : s.features) for (Image &i : row) destroy_image(d, i);
		for (Image &i : s.flow) destroy_image(d, i);
		for (Image &i : s.mask) destroy_image(d, i);
		destroy_image(d, s.motion);
	}
	for (VkPipeline p : n->conv_pipelines) vkDestroyPipeline(d, p, nullptr);
	for (VkPipeline p : n->pipelines) if (p) vkDestroyPipeline(d, p, nullptr);
	for (VkPipelineLayout l : n->pipeline_layouts) if (l) vkDestroyPipelineLayout(d, l, nullptr);
	for (VkDescriptorSetLayout l : n->set_layouts) if (l) vkDestroyDescriptorSetLayout(d, l, nullptr);
	if (n->sampler) vkDestroySampler(d, n->sampler, nullptr);
	if (n->weights) vkDestroyBuffer(d, n->weights, nullptr);
	if (n->weights_memory) vkFreeMemory(d, n->weights_memory, nullptr);
	delete n;
}

namespace {

// Reads the weight file into the blocks and the float array the shader reads
bool parse(fg_neural *n, const uint8_t *blob, size_t size, std::vector<float> &weights, char *error, size_t error_size)
{
	size_t at = 0;
	auto u32 = [&](uint32_t &out) {
		if (at + 4 > size) return false;
		memcpy(&out, blob + at, 4);
		at += 4;
		return true;
	};
	uint32_t magic = 0, count = 0;
	if (!u32(magic) || magic != kMagic) {
		say_error(error, error_size, "fg_neural: not a weight file (magic)");
		return false;
	}
	if (!u32(count) || count == 0 || count > kMaxBlocks) {
		say_error(error, error_size, "fg_neural: %u blocks (1 to %u)", count, kMaxBlocks);
		return false;
	}
	n->block_count = count;
	for (uint32_t b = 0; b < count; b++) {
		Block &blk = n->blocks[b];
		if (!u32(blk.scale) || !u32(blk.width) || !u32(blk.convs) || !u32(blk.radius) || blk.radius > 4 || blk.scale == 0 || blk.scale > 16 ||
			blk.width == 0 || blk.width > kMaxGroupsPerLayer * 4 || blk.convs < 2 || blk.convs > 12) {
			say_error(error, error_size, "fg_neural: block %u's header is out of range", b);
			return false;
		}
		if (b > 0 && (n->blocks[b - 1].scale % blk.scale != 0 || blk.scale >= n->blocks[b - 1].scale)) {
			say_error(error, error_size, "fg_neural: block scales must fall, each dividing the one before");
			return false;
		}
	}
	for (uint32_t b = 0; b < count; b++) {
		Block &blk = n->blocks[b];
		for (uint32_t c = 0; c <= blk.convs; c++) {
			Conv conv{};
			if (!u32(conv.cin) || !u32(conv.cout) || !u32(conv.leaky)) {
				say_error(error, error_size, "fg_neural: truncated at block %u", b);
				return false;
			}
			const uint32_t want_in = c == 0 ? (b == 0 ? 7 : 18) + (2 * blk.radius + 1) * (2 * blk.radius + 1) : blk.width;
			const uint32_t want_out = c == blk.convs ? 5 : blk.width;
			if (conv.cin != want_in || conv.cout != want_out || conv.leaky != (c == blk.convs ? 0u : 1u)) {
				say_error(error, error_size, "fg_neural: block %u convolution %u is %u->%u, expected %u->%u", b, c, conv.cin,
					conv.cout, want_in, want_out);
				return false;
			}
			const uint32_t ig = ceil_div(conv.cin, 4), og = ceil_div(conv.cout, 4);
			if (ig > kMaxGroupsPerLayer || og > kMaxGroupsPerLayer) {
				say_error(error, error_size, "fg_neural: too many channels in block %u", b);
				return false;
			}
			const size_t weight_floats = size_t(og) * 9 * ig * 16, bias_floats = size_t(og) * 4;
			if (at + (weight_floats + bias_floats) * 4 > size) {
				say_error(error, error_size, "fg_neural: truncated weights in block %u", b);
				return false;
			}
			conv.weight_base = static_cast<uint32_t>(weights.size() / 4);
			weights.resize(weights.size() + weight_floats);
			memcpy(weights.data() + weights.size() - weight_floats, blob + at, weight_floats * 4);
			at += weight_floats * 4;
			conv.bias_base = static_cast<uint32_t>(weights.size() / 4);
			weights.resize(weights.size() + bias_floats);
			memcpy(weights.data() + weights.size() - bias_floats, blob + at, bias_floats * 4);
			at += bias_floats * 4;
			blk.layers.push_back(conv);
		}
	}
	if (at != size) {
		say_error(error, error_size, "fg_neural: %zu bytes after the last layer", size - at);
		return false;
	}
	return true;
}

} // namespace

fg_neural *fg_neural_create(const fg_neural_info *info, const void *blob, size_t size, char *error, size_t error_size)
{
	fg_neural *n = new (std::nothrow) fg_neural;
	if (!n) {
		say_error(error, error_size, "fg_neural: out of host memory");
		return nullptr;
	}
	n->device = info->device;
	n->width = info->width;
	n->height = info->height;
	n->slots = info->slots;
	if (info->source_width > 0 && info->source_height > 0 && info->width % info->source_width == 0 && info->height % info->source_height == 0
		&& info->width / info->source_width == info->height / info->source_height) {
		n->src_w = info->source_width;
		n->src_h = info->source_height;
		n->grid_w = (n->src_w + 7) / 8;
		n->grid_h = (n->src_h + 7) / 8;
	}
	vkGetPhysicalDeviceMemoryProperties(info->physical_device, &n->memory);
	std::vector<float> weights;
	if (!parse(n, static_cast<const uint8_t *>(blob), size, weights, error, error_size)) {
		fg_neural_destroy(n);
		return nullptr;
	}
	const uint32_t target_h = working_height();
	n->work_h = target_h;
	n->work_w = std::max(64u, static_cast<uint32_t>(std::lround(double(target_h) * info->width / info->height / 16.0)) * 16);
	for (uint32_t b = 0; b < n->block_count; b++) {
		Block &blk = n->blocks[b];
		blk.w = n->work_w / blk.scale;
		blk.h = n->work_h / blk.scale;
		if (blk.w * blk.scale != n->work_w || blk.h * blk.scale != n->work_h) {
			say_error(error, error_size, "fg_neural: the working size %ux%u is not a multiple of scale %u", n->work_w, n->work_h, blk.scale);
			fg_neural_destroy(n);
			return nullptr;
		}
	}

	VkDevice d = n->device;
	VkResult result = VK_SUCCESS;
#define NN_CALL(call)                                                                      \
	do {                                                                                   \
		result = (call);                                                                   \
		if (result != VK_SUCCESS) {                                                        \
			say_error(error, error_size, "fg_neural: %s returned %d", #call, static_cast<int>(result)); \
			fg_neural_destroy(n);                                                          \
			return nullptr;                                                                \
		}                                                                                  \
	} while (0)

	// The weights: host-visible, in device-local memory where the device has such
	const VkBufferCreateInfo bufferInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = weights.size() * sizeof(float),
		.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	};
	NN_CALL(vkCreateBuffer(d, &bufferInfo, nullptr, &n->weights));
	VkMemoryRequirements req;
	vkGetBufferMemoryRequirements(d, n->weights, &req);
	uint32_t type = find_memory(n->memory, req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (type == UINT32_MAX) {
		type = find_memory(n->memory, req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}
	if (type == UINT32_MAX) {
		say_error(error, error_size, "fg_neural: no host-visible memory for the weights");
		fg_neural_destroy(n);
		return nullptr;
	}
	const VkMemoryAllocateInfo allocInfo{ .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .allocationSize = req.size, .memoryTypeIndex = type };
	NN_CALL(vkAllocateMemory(d, &allocInfo, nullptr, &n->weights_memory));
	NN_CALL(vkBindBufferMemory(d, n->weights, n->weights_memory, 0));
	void *mapped = nullptr;
	NN_CALL(vkMapMemory(d, n->weights_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
	memcpy(mapped, weights.data(), weights.size() * sizeof(float));
	vkUnmapMemory(d, n->weights_memory);

	const VkSamplerCreateInfo samplerInfo{
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.maxLod = 0.0f,
	};
	NN_CALL(vkCreateSampler(d, &samplerInfo, nullptr, &n->sampler));

	// Descriptor layouts: S = combined image sampler, I = storage image, B = storage buffer
	const char *shapes[kKinds] = { "SI", "SI", "SSSSI", "IIB", "ISSII", "SSSSIS", "SSI", "SSIS", "SSI", "SSSSI" };
	const uint32_t push_sizes[kKinds] = { 8, 12, 24, 16, 16, 36, 16, 20, 8, 16 };
	for (uint32_t k = 0; k < kKinds; k++) {
		VkDescriptorSetLayoutBinding bindings[8]{};
		uint32_t count = 0;
		for (const char *c = shapes[k]; *c; c++, count++) {
			bindings[count] = { count, *c == 'S' ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : *c == 'I' ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };
		}
		const VkDescriptorSetLayoutCreateInfo layoutInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .bindingCount = count, .pBindings = bindings };
		NN_CALL(vkCreateDescriptorSetLayout(d, &layoutInfo, nullptr, &n->set_layouts[k]));
		const VkPushConstantRange push{ VK_SHADER_STAGE_COMPUTE_BIT, 0, push_sizes[k] };
		const VkPipelineLayoutCreateInfo plInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, .setLayoutCount = 1,
			.pSetLayouts = &n->set_layouts[k], .pushConstantRangeCount = 1, .pPushConstantRanges = &push };
		NN_CALL(vkCreatePipelineLayout(d, &plInfo, nullptr, &n->pipeline_layouts[k]));
	}

	struct Code { const uint32_t *words; size_t bytes; };
	const Code code[kKinds] = { { fg_spv_nn_downsample, sizeof(fg_spv_nn_downsample) }, { fg_spv_nn_pool, sizeof(fg_spv_nn_pool) },
		{ fg_spv_nn_prep, sizeof(fg_spv_nn_prep) }, { fg_spv_nn_conv, sizeof(fg_spv_nn_conv) },
		{ fg_spv_nn_update, sizeof(fg_spv_nn_update) }, { fg_spv_nn_final, sizeof(fg_spv_nn_final) },
		{ fg_spv_nn_tiles, sizeof(fg_spv_nn_tiles) }, { fg_spv_nn_motion, sizeof(fg_spv_nn_motion) },
		{ fg_spv_nn_static, sizeof(fg_spv_nn_static) }, { fg_spv_nn_field, sizeof(fg_spv_nn_field) } };
	auto makePipeline = [&](PassKind kind, const VkSpecializationInfo *spec, VkPipeline *out) {
		const VkShaderModuleCreateInfo moduleInfo{ .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, .codeSize = code[kind].bytes, .pCode = code[kind].words };
		VkShaderModule module = VK_NULL_HANDLE;
		VkResult r = vkCreateShaderModule(d, &moduleInfo, nullptr, &module);
		if (r != VK_SUCCESS) return r;
		const VkComputePipelineCreateInfo pipelineInfo{ .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			.stage = { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_COMPUTE_BIT, .module = module,
				.pName = "main", .pSpecializationInfo = spec },
			.layout = n->pipeline_layouts[kind] };
		r = vkCreateComputePipelines(d, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, out);
		vkDestroyShaderModule(d, module, nullptr);
		return r;
	};
	// The downsample's taps are specialised (unrolled): they follow from the frames' size
	const int32_t taps[2] = {
		int32_t(std::min(8u, std::max(1u, ceil_div(ceil_div(info->width, n->work_w), 2)))),
		int32_t(std::min(8u, std::max(1u, ceil_div(ceil_div(info->height, n->work_h), 2)))) };
	const VkSpecializationMapEntry tapEntries[2] = { { 0, 0, 4 }, { 1, 4, 4 } };
	const VkSpecializationInfo tapSpec{ 2, tapEntries, sizeof(taps), taps };
	for (uint32_t k = 0; k < kKinds; k++) {
		if (k != kConv) {
			NN_CALL(makePipeline(PassKind(k), k == kDownsample ? &tapSpec : nullptr, &n->pipelines[k]));
		}
	}
	// One conv pipeline for each (input groups, output groups, leaky) the network uses
	struct Variant { uint32_t ig, og, leaky, ogs, lds, lz; VkPipeline pipeline; };
	std::vector<Variant> variants;
	// A convolution's variant by the image's size: below 4096 pixels the workgroup loads the tile into
	// shared memory and splits the output groups between up to four layers of threads; above, each
	// thread reads for itself and makes them all (shaders/nn_conv.comp)
	auto convPipeline = [&](const Conv &c, uint32_t pixels, uint32_t *lz_out) -> VkPipeline {
		const uint32_t ig = ceil_div(c.cin, 4), og = ceil_div(c.cout, 4);
		const uint32_t lds = pixels < 4096 ? 1 : 0;
		const uint32_t lz = !lds ? 1 : (og % 4 == 0 ? 4 : (og % 2 == 0 ? 2 : 1)), ogs = og / lz;
		*lz_out = lz;
		for (const Variant &v : variants) {
			if (v.ig == ig && v.og == og && v.leaky == c.leaky && v.ogs == ogs && v.lds == lds && v.lz == lz) return v.pipeline;
		}
		const uint32_t values[6] = { ig, og, c.leaky, ogs, lds, lz };
		const VkSpecializationMapEntry entries[6] = { { 0, 0, 4 }, { 1, 4, 4 }, { 2, 8, 4 }, { 3, 12, 4 }, { 4, 16, 4 }, { 5, 20, 4 } };
		const VkSpecializationInfo spec{ 6, entries, sizeof(values), values };
		VkPipeline pipeline = VK_NULL_HANDLE;
		if (makePipeline(kConv, &spec, &pipeline) != VK_SUCCESS) return VK_NULL_HANDLE;
		n->conv_pipelines.push_back(pipeline);
		variants.push_back({ ig, og, c.leaky, ogs, lds, lz, pipeline });
		return pipeline;
	};

	// Images: storage and sampled (the arrays storage only), created in GENERAL layout later
	auto makeImage = [&](Image &img, uint32_t w, uint32_t h, uint32_t layers, bool sampled) -> VkResult {
		const VkImageCreateInfo imageInfo{ .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, .imageType = VK_IMAGE_TYPE_2D, .format = kFeatureFormat,
			.extent = { w, h, 1 }, .mipLevels = 1, .arrayLayers = layers, .samples = VK_SAMPLE_COUNT_1_BIT, .tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VkImageUsageFlags(VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | (sampled ? VK_IMAGE_USAGE_SAMPLED_BIT : 0)), .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED };
		VkResult r = vkCreateImage(d, &imageInfo, nullptr, &img.image);
		if (r != VK_SUCCESS) return r;
		VkMemoryRequirements mr;
		vkGetImageMemoryRequirements(d, img.image, &mr);
		const uint32_t mt = find_memory(n->memory, mr.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		if (mt == UINT32_MAX) return VK_ERROR_OUT_OF_DEVICE_MEMORY;
		const VkMemoryAllocateInfo ai{ .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .allocationSize = mr.size, .memoryTypeIndex = mt };
		r = vkAllocateMemory(d, &ai, nullptr, &img.memory);
		if (r != VK_SUCCESS) return r;
		r = vkBindImageMemory(d, img.image, img.memory, 0);
		if (r != VK_SUCCESS) return r;
		const VkImageViewCreateInfo viewInfo{ .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, .image = img.image,
			.viewType = layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D, .format = kFeatureFormat,
			.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, layers } };
		return vkCreateImageView(d, &viewInfo, nullptr, &img.view);
	};

	// The pool: every pass of every slot has one set
	const uint32_t per_slot_passes = 2 + n->block_count * 2 + n->block_count * 2 + [&] {
		uint32_t convs = 0;
		for (uint32_t b = 0; b < n->block_count; b++) convs += n->blocks[b].convs + 1;
		return convs;
	}() + 1;
	const uint32_t set_count = (per_slot_passes + (kRing - 1) * 3 + 4 * kRing + 4) * n->slots;
	const VkDescriptorPoolSize sizes[] = { { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, set_count * 6 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, set_count * 3 }, { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, set_count } };
	const VkDescriptorPoolCreateInfo poolInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, .maxSets = set_count, .poolSizeCount = 3, .pPoolSizes = sizes };
	NN_CALL(vkCreateDescriptorPool(d, &poolInfo, nullptr, &n->pool));

	for (uint32_t s = 0; s < n->slots; s++) {
		SlotImages &si = n->slot[s];
		for (Image &img : si.work) NN_CALL(makeImage(img, n->work_w, n->work_h, 1, true));
		for (uint32_t b = 0; b < n->block_count; b++) {
			const Block &blk = n->blocks[b];
			uint32_t layers = 2;
			for (const Conv &c : blk.layers) layers = std::max({ layers, ceil_div(c.cin, 4), ceil_div(c.cout, 4) });
			for (Image &img : si.pooled[b]) NN_CALL(makeImage(img, blk.w, blk.h, 1, true));
			for (Image &img : si.features[b]) NN_CALL(makeImage(img, blk.w, blk.h, layers, false));
			NN_CALL(makeImage(si.flow[b], blk.w, blk.h, 1, true));
			NN_CALL(makeImage(si.mask[b], blk.w, blk.h, 1, true));
		}
		if (n->src_w) {
			if (s == 0) {
				NN_CALL(makeImage(n->tiles, n->grid_w, n->grid_h, 1, true));
				NN_CALL(makeImage(n->stable, n->src_w, n->src_h, 1, true));
				NN_CALL(makeImage(n->field, n->src_w, n->src_h, 1, true));
			}
			NN_CALL(makeImage(si.motion, n->src_w, n->src_h, 1, true));
		}

		auto alloc = [&](PassKind kind, VkDescriptorSet *set) -> VkResult {
			const VkDescriptorSetAllocateInfo ai{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .descriptorPool = n->pool,
				.descriptorSetCount = 1, .pSetLayouts = &n->set_layouts[kind] };
			return vkAllocateDescriptorSets(d, &ai, set);
		};
		// Writes a set: S entries take a view with the sampler, I entries a view, B the weights
		auto write = [&](VkDescriptorSet set, PassKind kind, std::initializer_list<VkImageView> views) {
			VkDescriptorImageInfo infos[8]{};
			VkDescriptorBufferInfo bufferInfo{ n->weights, 0, VK_WHOLE_SIZE };
			VkWriteDescriptorSet writes[8]{};
			uint32_t i = 0;
			auto view = views.begin();
			for (const char *c = shapes[kind]; *c; c++, i++) {
				writes[i] = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = set, .dstBinding = i, .descriptorCount = 1 };
				if (*c == 'B') {
					writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					writes[i].pBufferInfo = &bufferInfo;
				} else {
					infos[i] = { *c == 'S' ? n->sampler : VK_NULL_HANDLE, view != views.end() ? *view++ : VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL };
					writes[i].descriptorType = *c == 'S' ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
					writes[i].pImageInfo = &infos[i];
				}
			}
			vkUpdateDescriptorSets(d, i, writes, 0, nullptr);
		};
		auto add = [&](PassKind kind, VkPipeline pipeline, VkDescriptorSet set, uint32_t w, uint32_t h, const void *push, uint32_t push_size,
			const std::string &label) {
			Pass p{};
			p.label = label;
			p.kind = kind; p.pipeline = pipeline; p.set = set;
			p.groups_x = ceil_div(w, kGroup); p.groups_y = ceil_div(h, kGroup); p.groups_z = 1;
			memcpy(p.push, push, push_size);
			p.push_size = push_size;
			si.passes.push_back(p);
		};

		// 1. the frames at working size (their views are written on every call)
		for (uint32_t f = 0; f < 2; f++) {
			for (uint32_t r = 0; r < kRing; r++) {
				NN_CALL(alloc(kDownsample, &si.down_sets[r][f]));
				write(si.down_sets[r][f], kDownsample, { si.work[f].view, si.work[f].view });
			}
			const int32_t push[2] = { int32_t(n->work_w), int32_t(n->work_h) };
			add(kDownsample, n->pipelines[kDownsample], si.down_sets[0][f], n->work_w, n->work_h, push, sizeof(push), "downsample " + std::to_string(f));
			si.passes.back().which = int32_t(f);
		}
		// 2. both frames averaged to each block's scale, the finest from the working size and each
		// coarser from the next finer (an average of equal averages is the average)
		for (uint32_t b = n->block_count; b-- > 0;) {
			const Block &blk = n->blocks[b];
			if (blk.scale == 1) continue;
			const bool finest = b == n->block_count - 1;
			const uint32_t ratio = finest ? blk.scale : blk.scale / n->blocks[b + 1].scale;
			for (uint32_t f = 0; f < 2; f++) {
				VkDescriptorSet set;
				NN_CALL(alloc(kPool, &set));
				write(set, kPool, { finest ? si.work[f].view : si.pooled[b + 1][f].view, si.pooled[b][f].view });
				const int32_t push[3] = { int32_t(blk.w), int32_t(blk.h), int32_t(ratio) };
				add(kPool, n->pipelines[kPool], set, blk.w, blk.h, push, sizeof(push), "pool b" + std::to_string(b) + " " + std::to_string(f));
			}
		}
		for (uint32_t b = 0; b < n->block_count; b++) {
			const Block &blk = n->blocks[b];
			VkImageView pooled_view[2];
			for (uint32_t f = 0; f < 2; f++) {
				pooled_view[f] = blk.scale == 1 ? si.work[f].view : si.pooled[b][f].view;
			}
			// 3. the block's input: the frames, what the previous blocks found, and t
			const Block *prev = b ? &n->blocks[b - 1] : nullptr;
			const VkImageView prev_flow = b ? si.flow[b - 1].view : pooled_view[0];
			const VkImageView prev_mask = b ? si.mask[b - 1].view : pooled_view[0];
			VkDescriptorSet prep_set;
			NN_CALL(alloc(kPrep, &prep_set));
			write(prep_set, kPrep, { pooled_view[0], pooled_view[1], prev_flow, prev_mask, si.features[b][0].view });
			struct PrepPush { int32_t w, h, first; float t, ratio; int32_t radius; };
			PrepPush pp{ int32_t(blk.w), int32_t(blk.h), b == 0 ? 1 : 0, 0.0f, prev ? float(prev->scale) / float(blk.scale) : 1.0f, int32_t(blk.radius) };
			add(kPrep, n->pipelines[kPrep], prep_set, blk.w, blk.h, &pp, sizeof(pp), "prep b" + std::to_string(b) + " " + std::to_string(blk.w) + "x" + std::to_string(blk.h));
			// 4. the convolutions, ping-ponging between the two arrays
			uint32_t from = 0;
			for (const Conv &c : blk.layers) {
				VkDescriptorSet set;
				NN_CALL(alloc(kConv, &set));
				write(set, kConv, { si.features[b][from].view, si.features[b][1 - from].view });
				uint32_t lz = 1;
				VkPipeline pipeline = convPipeline(c, blk.w * blk.h, &lz);
				if (!pipeline) {
					say_error(error, error_size, "fg_neural: could not make a convolution pipeline");
					fg_neural_destroy(n);
					return nullptr;
				}
				const uint32_t push[4] = { blk.w, blk.h, c.weight_base, c.bias_base };
				add(kConv, pipeline, set, blk.w, blk.h, push, sizeof(push),
					"conv b" + std::to_string(b) + " " + std::to_string(c.cin) + ">" + std::to_string(c.cout) + " " + std::to_string(blk.w) + "x" + std::to_string(blk.h));
				from = 1 - from;
			}
			// 5. the state: the previous block's, scaled up, plus this block's output
			VkDescriptorSet update_set;
			NN_CALL(alloc(kUpdate, &update_set));
			write(update_set, kUpdate, { si.features[b][from].view, prev_flow, prev_mask, si.flow[b].view, si.mask[b].view });
			struct UpdatePush { int32_t w, h, first; float ratio; };
			UpdatePush up{ int32_t(blk.w), int32_t(blk.h), b == 0 ? 1 : 0, prev ? float(prev->scale) / float(blk.scale) : 1.0f };
			add(kUpdate, n->pipelines[kUpdate], update_set, blk.w, blk.h, &up, sizeof(up), "update b" + std::to_string(b));
		}
		// 5b. pixel art: the motion of whole source pixels (the frames are written on every call)
		if (n->src_w) {
			for (uint32_t r = 0; r < kRing; r++) {
				NN_CALL(alloc(kTiles, &si.tiles_sets[r]));
				write(si.tiles_sets[r], kTiles, { si.work[0].view, si.work[1].view, n->tiles.view });
				NN_CALL(alloc(kMotion, &si.motion_sets[r]));
				write(si.motion_sets[r], kMotion, { n->tiles.view, n->field.view, si.motion.view, n->stable.view });
				NN_CALL(alloc(kField, &si.field_sets[r]));
				write(si.field_sets[r], kField, { si.work[0].view, si.work[1].view, n->tiles.view, n->stable.view, n->field.view });
				NN_CALL(alloc(kStatic, &si.static_sets[r]));
				write(si.static_sets[r], kStatic, { si.work[0].view, si.work[1].view, n->stable.view });
			}
			const int32_t sp[2] = { int32_t(n->src_w), int32_t(n->src_h) };
			add(kStatic, n->pipelines[kStatic], si.static_sets[0], n->src_w, n->src_h, sp, sizeof(sp), "static " + std::to_string(n->src_w) + "x" + std::to_string(n->src_h));
			const int32_t fp4[4] = { int32_t(n->src_w), int32_t(n->src_h), int32_t(n->grid_w), int32_t(n->grid_h) };
			const int32_t tp[4] = { int32_t(n->src_w), int32_t(n->src_h), int32_t(n->grid_w), int32_t(n->grid_h) };
			add(kTiles, n->pipelines[kTiles], si.tiles_sets[0], 8, 8, tp, sizeof(tp), "tiles " + std::to_string(n->grid_w) + "x" + std::to_string(n->grid_h));
			si.passes.back().groups_x = n->grid_w;
			si.passes.back().groups_y = n->grid_h;
			add(kField, n->pipelines[kField], si.field_sets[0], n->src_w, n->src_h, fp4, sizeof(fp4), "field " + std::to_string(n->src_w) + "x" + std::to_string(n->src_h));
			struct MotionPush { int32_t w, h, gw, gh; float t; };
			MotionPush mp{ int32_t(n->src_w), int32_t(n->src_h), int32_t(n->grid_w), int32_t(n->grid_h), 0.5f };
			add(kMotion, n->pipelines[kMotion], si.motion_sets[0], n->src_w, n->src_h, &mp, sizeof(mp), "motion " + std::to_string(n->src_w) + "x" + std::to_string(n->src_h));
		}
		// 6. the frames at full size, warped and blended (the frames and the output are written on every call)
		for (uint32_t r = 0; r < kRing; r++) NN_CALL(alloc(kFinal, &si.final_sets[r]));
		const Block &last = n->blocks[n->block_count - 1];
		for (uint32_t r = 0; r < kRing; r++)
			write(si.final_sets[r], kFinal, { si.work[0].view, si.work[1].view, si.flow[n->block_count - 1].view, si.mask[n->block_count - 1].view, si.work[0].view,
				n->src_w ? si.motion.view : si.work[0].view });
		struct FinalPush { int32_t w, h; float ww, wh, scale, t; int32_t crisp; int32_t sw, sh; };
		FinalPush fp{ int32_t(info->width), int32_t(info->height), float(n->work_w), float(n->work_h), float(last.scale), 0.5f, 0, int32_t(n->src_w), int32_t(n->src_h) };
		add(kFinal, n->pipelines[kFinal], si.final_sets[0], info->width, info->height, &fp, sizeof(fp), "final " + std::to_string(info->width) + "x" + std::to_string(info->height));
	}
#undef NN_CALL
	return n;
}

void fg_neural_record(fg_neural *n, VkCommandBuffer cmd, uint32_t slot, const fg_frame_pair *pair, VkImageView output)
{
	if (!n || slot >= n->slots) {
		return;
	}
	VkDevice d = n->device;
	SlotImages &si = n->slot[slot];

	if (!si.initialised) {
		// The first call: every intermediate image to GENERAL, contents undefined
		bool clear_stable = false;
		std::vector<VkImageMemoryBarrier> barriers;
		auto add = [&](const Image &img, uint32_t layers) {
			barriers.push_back({ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, .srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_GENERAL, .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = img.image, .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, layers } });
		};
		for (const Image &i : si.work) add(i, 1);
		for (uint32_t b = 0; b < n->block_count; b++) {
			for (const Image &i : si.pooled[b]) add(i, 1);
			for (const Image &i : si.features[b]) add(i, VK_REMAINING_ARRAY_LAYERS);
			add(si.flow[b], 1);
			add(si.mask[b], 1);
		}
		if (n->src_w) {
			if (!n->tiles_init) {
				add(n->tiles, 1);     // (shared by the slots: moved to GENERAL once, never again, or it would lose what a call found)
				add(n->stable, 1);
				add(n->field, 1);
				clear_stable = true;
				n->tiles_init = true;
			}
			add(si.motion, 1);
		}
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
			static_cast<uint32_t>(barriers.size()), barriers.data());
		if (clear_stable) {
			// (the counts start at nothing)
			const VkClearColorValue zero{};
			const VkImageSubresourceRange range{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			VkMemoryBarrier toClear{ .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER, .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT };
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &toClear, 0, nullptr, 0, nullptr);
			vkCmdClearColorImage(cmd, n->stable.image, VK_IMAGE_LAYOUT_GENERAL, &zero, 1, &range);
			VkMemoryBarrier fromClear{ .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER, .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT };
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &fromClear, 0, nullptr, 0, nullptr);
		}
		si.initialised = true;
	}

	// The call's own bindings: the frames to reduce, and the frames and output of the last pass. They go in the
	// next set of a ring: a set the GPU may still be reading is never rewritten (on the PS5's driver that update
	// waits for the GPU, and a frame waits for the display).
	const uint32_t ring = si.ring;
	si.ring = (ring + 1) % kRing;
	const VkImageView frames[2] = { pair->prev_color, pair->next_color };
	VkDescriptorImageInfo infos[13]{};
	VkWriteDescriptorSet writes[13]{};
	uint32_t count = 0;
	auto sampled = [&](VkDescriptorSet set, uint32_t binding, VkImageView view) {
		infos[count] = { n->sampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		writes[count] = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = set, .dstBinding = binding, .descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = &infos[count] };
		count++;
	};
	for (uint32_t f = 0; f < 2; f++) {
		sampled(si.down_sets[ring][f], 0, frames[f]);
		sampled(si.final_sets[ring], f, frames[f]);
		if (n->src_w) {
			sampled(si.tiles_sets[ring], f, frames[f]);
			sampled(si.field_sets[ring], f, frames[f]);
			sampled(si.static_sets[ring], f, frames[f]);
		}
	}
	infos[count] = { VK_NULL_HANDLE, output, VK_IMAGE_LAYOUT_GENERAL };
	writes[count] = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = si.final_sets[ring], .dstBinding = 4, .descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .pImageInfo = &infos[count] };
	count++;
	vkUpdateDescriptorSets(d, count, writes, 0, nullptr);
	uint32_t crispMode = pair->crisp;
	if (const char *force = getenv("FG_CRISP")) {
		crispMode = uint32_t(atoi(force));   // (tests)
	}
	if (crispMode == 5 && !n->src_w) {
		crispMode = 1;                        // (no motion pass to stand in for the network)
	}
	const bool motionPass = n->src_w != 0 && crispMode != 0;

	const VkMemoryBarrier barrier{ .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER, .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT };
	bool first = true;
	uint32_t index = 0;
	if (motionPass) {
		// the tiles found by an earlier call (another command buffer) are read here, and written here for later ones
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
	}
	if (n->profile_pool) {
		vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, n->profile_pool, n->profile_base);
	}
	for (Pass &p : si.passes) {
		if ((p.kind == kTiles || p.kind == kMotion || p.kind == kStatic || p.kind == kField) && !motionPass) {
			continue;
		}
		if ((p.kind == kTiles || p.kind == kStatic || p.kind == kField) && pair->reuse_tiles) {
			continue;
		}
		if (crispMode == 5 && p.kind <= kUpdate) {
			continue;                     // the motion pass alone: no network
		}
		if (!first) {
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
		}
		first = false;
		uint32_t push[16];
		memcpy(push, p.push, sizeof(push));
		if (p.kind == kMotion) {
			// the time goes in the fifth word (see MotionPush)
			memcpy(&push[4], &pair->t, sizeof(float));
		} else if (p.kind == kPrep) {
			// the time goes in the fourth word (see PrepPush)
			memcpy(&push[3], &pair->t, sizeof(float));
		} else if (p.kind == kFinal) {
			// the time goes in the sixth word (see FinalPush)
			memcpy(&push[5], &pair->t, sizeof(float));
			push[6] = crispMode;
			if (!motionPass) {
				push[7] = push[8] = 0;       // no motion pass for this call: nothing to read
			}
		}
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, p.pipeline);
		VkDescriptorSet set = p.set;
		if (p.kind == kDownsample) set = si.down_sets[ring][p.which];
		else if (p.kind == kFinal) set = si.final_sets[ring];
		else if (p.kind == kTiles) set = si.tiles_sets[ring];
		else if (p.kind == kMotion) set = si.motion_sets[ring];
		else if (p.kind == kStatic) set = si.static_sets[ring];
		else if (p.kind == kField) set = si.field_sets[ring];
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, n->pipeline_layouts[p.kind], 0, 1, &set, 0, nullptr);
		vkCmdPushConstants(cmd, n->pipeline_layouts[p.kind], VK_SHADER_STAGE_COMPUTE_BIT, 0, p.push_size, push);
		vkCmdDispatch(cmd, p.groups_x, p.groups_y, p.groups_z);
		index++;
		if (n->profile_pool) {
			vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, n->profile_pool, n->profile_base + index);
		}
	}
}

void fg_neural_set_profile(fg_neural *n, VkQueryPool pool, uint32_t base)
{
	if (n) {
		n->profile_pool = pool;
		n->profile_base = base;
	}
}

uint32_t fg_neural_pass_count(const fg_neural *n)
{
	return n ? static_cast<uint32_t>(n->slot[0].passes.size()) : 0;
}

const char *fg_neural_pass_label(const fg_neural *n, uint32_t pass)
{
	return n && pass < n->slot[0].passes.size() ? n->slot[0].passes[pass].label.c_str() : "";
}
