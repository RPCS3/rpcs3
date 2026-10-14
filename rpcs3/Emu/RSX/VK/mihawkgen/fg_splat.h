/*
 * PS5_FrameGen - the forward-splat method's interface to the context (fg_splat.cpp).
 *
 * Copyright (C) 2026 Mihawk
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "framegen.h"

struct fg_splat;

struct fg_splat_info {
	VkDevice device;
	VkPhysicalDevice physical_device;
	uint32_t width, height, slots;
};

/* Builds the pipelines, the images (one set for each slot) and the descriptor sets. nullptr on failure, `error` says why. */
fg_splat *fg_splat_create(const fg_splat_info *info, char *error, size_t error_size);
void fg_splat_destroy(fg_splat *splat);
/* Records the method's passes into `cmd`, with the barriers between them. */
void fg_splat_record(fg_splat *splat, VkCommandBuffer cmd, uint32_t slot, const fg_frame_pair *pair, VkImageView output);
/* Profiling, as fg_context_set_profile does for the neural method */
void fg_splat_set_profile(fg_splat *splat, VkQueryPool pool, uint32_t base);
uint32_t fg_splat_pass_count(const fg_splat *splat);
const char *fg_splat_pass_label(const fg_splat *splat, uint32_t pass);
