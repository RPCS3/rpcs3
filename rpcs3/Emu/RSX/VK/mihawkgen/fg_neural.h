/*
 * PS5_FrameGen - the neural method's interface to the context (fg_neural.cpp).
 *
 * Copyright (C) 2026 Mihawk
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "framegen.h"

struct fg_neural;

struct fg_neural_info {
	VkDevice device;
	VkPhysicalDevice physical_device;
	uint32_t width, height, slots;
	uint32_t source_width, source_height; // pixel art: the frames in source pixels (0: no motion pass)
};

/* Parses the weight file (train/export.py's format) and builds the pipelines, images and
 * descriptor sets. nullptr on failure, with `error` saying why. */
fg_neural *fg_neural_create(const fg_neural_info *info, const void *blob, size_t size, char *error, size_t error_size);
void fg_neural_destroy(fg_neural *neural);
/* Records the method's passes into `cmd`, with the barriers between them. */
void fg_neural_record(fg_neural *neural, VkCommandBuffer cmd, uint32_t slot, const fg_frame_pair *pair, VkImageView output);

/* Profiling: see fg_context_set_profile */
void fg_neural_set_profile(fg_neural *neural, VkQueryPool pool, uint32_t base);
uint32_t fg_neural_pass_count(const fg_neural *neural);
const char *fg_neural_pass_label(const fg_neural *neural, uint32_t pass);
