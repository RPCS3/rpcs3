/*
 * PS5_FrameGen - the frame generator's interface.
 *
 * The generator makes a frame at a time t between two frames a program has
 * rendered, from their colour, depth and motion vectors. It is a set of compute
 * passes recorded into the caller's command buffer: one device, one queue, no
 * threads and no memory of its own between calls beyond what fg_context_create
 * made. It is designed from public sources.
 *
 * What the caller owns, and what the generator expects:
 * - The device, the command buffer, every image and every barrier. The
 *   generator records no barriers: before fg_record_generate the inputs are in
 *   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL (depth: the depth aspect's view) and
 *   the output is in VK_IMAGE_LAYOUT_GENERAL, with their writes made visible to
 *   the compute stage; after it, the caller makes the output's compute writes
 *   visible to whatever reads it. One place, the program, owns synchronisation.
 * - Slots: the generator keeps one descriptor set for each (slot, method). A slot
 *   must not be reused until the command buffer that last used it has completed;
 *   passing the frame-in-flight index whose fence the program waited on keeps
 *   that true.
 * - Motion vectors are backward: for a pixel of the next frame, its uv minus the
 *   uv the same surface had in the previous frame (uv in [0, 1], y down).
 *
 * What it owns: fg_context_create makes the pipelines, a descriptor pool, the
 * descriptor sets and a sampler; fg_context_destroy releases them. On failure
 * fg_context_create leaves nothing allocated and says why in `error`.
 *
 * Methods: FG_METHOD_REPEAT (the previous frame, unchanged) and FG_METHOD_BLEND
 * (the two frames mixed at t) are baselines, the floor any generator must beat
 * in the harness (examples/framegenlab). FG_METHOD_MVWARP warps both frames by
 * the next frame's motion vectors. FG_METHOD_MVSPLAT carries the next frame's vectors to the instant
 * t with a depth test, fills what no surface reached from the background around it and reads both frames through
 * the field (docs/design.md): it needs fg_context_enable_mvsplat first, and owns images like the neural method.
 * FG_METHOD_NEURAL is the trained network
 * (train/model.py): it reads the two colour frames only, and is available once
 * fg_context_load_network has been given its weights. Unlike the others it owns
 * images of its own (one set for each slot), and records the barriers between its
 * passes; the caller's contract for its inputs and output is unchanged.
 *
 * Copyright (C) 2026 Mihawk
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

/* The including unit sees Vulkan's declarations first: volk's in the title
 * build (every unit is compiled with -include volk.h), the headers in the host
 * tests. */
#include <vulkan/vulkan.h>

enum fg_method : uint32_t {
	FG_METHOD_REPEAT = 0,
	FG_METHOD_BLEND = 1,
	FG_METHOD_MVWARP = 2,
	FG_METHOD_NEURAL = 3,
	FG_METHOD_MVSPLAT = 4,
	FG_METHOD_COUNT
};

struct fg_create_info {
	VkDevice device;
	/* The device's physical device: fg_context_load_network reads its memory types */
	VkPhysicalDevice physical_device;
	/* The frames' size in pixels: inputs and output all have it */
	uint32_t width;
	uint32_t height;
	/* Frames in flight: descriptor sets are kept per slot (see above), 1 to 8 */
	uint32_t slots;
	/* The output's format: VK_FORMAT_R8G8B8A8_UNORM, the one the shaders write */
	VkFormat output_format;
	/* Pixel art for FG_METHOD_NEURAL: the frames are a source image of this size enlarged by a whole number (blocks of
	   equal pixels), and the method then also finds the motion of whole source pixels (fg_frame_pair::crisp). 0: not. */
	uint32_t source_width;
	uint32_t source_height;
};

struct fg_frame_pair {
	VkImageView prev_color;  /* the frame at t = 0, sampled */
	VkImageView next_color;  /* the frame at t = 1, sampled */
	VkImageView prev_depth;  /* their depth (depth aspect), sampled */
	VkImageView next_depth;
	VkImageView next_motion; /* the next frame's backward motion vectors, RG16F: they span the
	                            whole interval from the previous frame to the next */
	float t;                 /* where the generated frame lies: 0 < t < 1 */
	uint32_t source_width, source_height; /* FG_METHOD_REPEAT only: the part of prev_color that holds the
	                            frame, when the image is padded or larger; 0: the whole image. The
	                            output is the frame averaged down (or copied) to the context's size. */
	uint32_t crisp;          /* FG_METHOD_NEURAL: 1 for pixel art, warped in whole pixels (no resampling blur) */
	uint32_t reuse_tiles;    /* FG_METHOD_NEURAL, pixel art: 1 when an earlier call with this pair of frames already searched the
	                            motion of its tiles (the search does not depend on t): it is not made again */
	/* FG_METHOD_MVSPLAT: what the depth images hold. depth_far 0: a linear view depth, smaller nearer. Otherwise a
	   projection's depth between the planes depth_near and depth_far (1 for the near plane when depth_inverted is 1). */
	float depth_near, depth_far;
	uint32_t depth_inverted;
	/* The fraction of a surface's distance (an absolute difference for a linear depth) within which two depths are one
	   surface; 0: 0.05 */
	float depth_tolerance;
};

struct fg_context;

/* What is wrong with `info`, or nullptr when it is valid. Checks only what can
 * be checked without a device. */
const char *fg_check_create_info(const fg_create_info *info);

/* Makes the context. VK_SUCCESS, or the failing call's result with `error`
 * (error_size bytes, may be 0) saying which call failed. */
VkResult fg_context_create(const fg_create_info *info, fg_context **out, char *error, size_t error_size);
void fg_context_destroy(fg_context *ctx);

/* Loads the neural method's weights (the file train/export.py writes; `blob` is its content,
 * which the context copies) and makes the method's pipelines and images. VK_SUCCESS, or a
 * failure with `error` saying why. A second call replaces the network. */
VkResult fg_context_load_network(fg_context *ctx, const void *blob, size_t size, char *error, size_t error_size);

/* Makes FG_METHOD_MVSPLAT's pipelines and images (one set for each slot, about 70 MB at 1920x1080). VK_SUCCESS, or a
 * failure with `error` saying why. Needs the physical_device the context was made with. */
VkResult fg_context_enable_mvsplat(fg_context *ctx, char *error, size_t error_size);
uint32_t fg_mvsplat_pass_count(const fg_context *ctx);
const char *fg_mvsplat_pass_label(const fg_context *ctx, uint32_t pass);

/* Profiling the neural method: with a timestamp query pool set, fg_record_generate writes a
 * timestamp before the method's first pass and one after each pass, at queries base, base + 1,
 * ... base + fg_neural_pass_count(ctx). The caller resets and reads the queries. Pass nullptr to
 * stop. fg_neural_pass_label gives pass i's name (valid until the network is replaced). */
void fg_context_set_profile(fg_context *ctx, VkQueryPool pool, uint32_t base);
uint32_t fg_neural_pass_count(const fg_context *ctx);
const char *fg_neural_pass_label(const fg_context *ctx, uint32_t pass);

/* Whether the method can be recorded: the baselines and mvwarp always; neural after a network is loaded */
bool fg_method_available(const fg_context *ctx, fg_method method);

/* Records the dispatches that write the frame at pair->t into `output` (a
 * storage view of an image of output_format and the context's size). */
void fg_record_generate(fg_context *ctx, VkCommandBuffer cmd, uint32_t slot, fg_method method,
	const fg_frame_pair *pair, VkImageView output);

/* The method's short name ("repeat", "blend", "mvwarp", "neural", "mvsplat"), or "unknown" */
const char *fg_method_name(fg_method method);

/* Peak signal-to-noise ratio in dB of values in [0, 1], from a sum of squared
 * errors over `samples` values: 10 log10(1 / mse). Infinity for no error, NaN
 * for no samples. */
double fg_psnr(double sum_squared_error, uint64_t samples);
