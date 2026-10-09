#include "stdafx.h"
#include "VKFrameGen.h"

#include "VKHelpers.h"
#include "VKResourceManager.h"
#include "vkutils/barriers.h"
#include "vkutils/device.h"

#include "util/asm.hpp"

// PS5: for the trace's status line, since the title started: the frames made;
// the RSX thread's time making and showing them, and in it waiting for the
// GPU (VKPresent.cpp); the frames made but dropped, no image free to show
// them in; and the RSX thread's time waiting for an image to show the game's
// own frames [0] and those made [1] in
atomic_t<u64> g_ps5_frames_generated = 0;
atomic_t<u64> g_ps5_frame_gen_us = 0;
atomic_t<u64> g_ps5_frame_gen_gpu_wait_us = 0;
atomic_t<u64> g_ps5_frames_dropped = 0;
atomic_t<u64> g_ps5_image_wait_us[2] = {};

namespace vk
{
	namespace
	{
		// Every pass: 8x8 invocations a group, three inputs sampled (unused ones
		// bound to the first), one output written, twelve numbers pushed
		constexpr const char* c_header = R"(
#version 450
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout(set = 0, binding = 0) uniform sampler2D tex0;
layout(set = 0, binding = 1) uniform sampler2D tex1;
layout(set = 0, binding = 2) uniform sampler2D tex2;
layout(set = 0, binding = 3, %FORMAT%) uniform writeonly image2D dst;
layout(push_constant) uniform push_block { vec4 p0; vec4 p1; vec4 p2; };
)";

		// The Quality setting's levels: the working size's width at most, and
		// how far the coarse search reaches, in quarter-size texels: about 256
		// pixels of a 1280-wide frame at each, a quick camera turn with the game
		// at 15 fps (build 107's reach, about 100 pixels, lost GTA IV's frames
		// between to the nearer real frame when it fell to 11-16 fps in busy
		// streets; in the model of these passes a 55-pixel move at the working
		// size went from 14.0 to 21.8 dB, 80 from 14.0 to 20.1, 110 from 13.9
		// to 18.1, and small moves stayed as they were)
		struct quality_level
		{
			const char* name;
			u32 width;
			u32 reach;
		};
		constexpr quality_level c_quality[] = { { "Fast", 480, 24 }, { "Balanced", 640, 32 }, { "Quality", 960, 48 } };

		enum pass_kind : int
		{
			pass_brightness,     // the frame's brightness at the working size
			pass_quarter,        // that at a quarter of it, 4x4 averaged
			pass_coarse_search,  // a vector per 4x4 block of the quarter size
			pass_fine_search,    // a vector per 8x8 block of the working size, from the coarse ones
			pass_median,         // the vectors' 3x3 median
			pass_between,        // the frame between
			pass_keep,           // the frame, kept for the next one
		};

		constexpr const char* c_formats[] = { "r32f", "r32f", "rgba16f", "rgba16f", "rgba16f", "rgba8", "rgba8" };

		constexpr const char* c_bodies[] =
		{
			// pass_brightness: tex0 the frame; p0 = (its used part as a share of
			// its texture across and down, the working size)
			R"(
void main()
{
	const ivec2 p = ivec2(gl_GlobalInvocationID.xy);
	const vec2 size = p0.zw;
	if (p.x >= int(size.x) || p.y >= int(size.y)) return;
	float l = 0.0;
	for (int j = 0; j < 2; j++)
	{
		for (int i = 0; i < 2; i++)
		{
			const vec2 uv = (vec2(p) + vec2(0.25 + 0.5 * float(i), 0.25 + 0.5 * float(j))) / size * p0.xy;
			l += dot(texture(tex0, uv).rgb, vec3(0.299, 0.587, 0.114));
		}
	}
	imageStore(dst, p, vec4(l * 0.25));
}
)",
			// pass_quarter: tex0 the working brightness; p0 = (the working size, the quarter size)
			R"(
void main()
{
	const ivec2 p = ivec2(gl_GlobalInvocationID.xy);
	if (p.x >= int(p0.z) || p.y >= int(p0.w)) return;
	const ivec2 lim = ivec2(p0.xy) - 1;
	float l = 0.0;
	for (int j = 0; j < 4; j++)
	{
		for (int i = 0; i < 4; i++)
		{
			l += texelFetch(tex0, min(p * 4 + ivec2(i, j), lim), 0).r;
		}
	}
	imageStore(dst, p, vec4(l / 16.0));
}
)",
			// pass_coarse_search: tex0 the last frame's quarter brightness, tex1
			// this one's; p0 = (the quarter size, its 4x4 block grid), p1.x the
			// reach. Each block of this frame is sought in the last one up to
			// the reach away (c_quality); the vector points from here to there,
			// its z the mean difference per texel. Shorter vectors are a little
			// favoured, but only a little: at build 107's 0.0015 a texel the
			// favour outweighed a true match 14 texels away
			R"(
void main()
{
	const ivec2 p = ivec2(gl_GlobalInvocationID.xy);
	if (p.x >= int(p0.z) || p.y >= int(p0.w)) return;
	const ivec2 lim = ivec2(p0.xy) - 1;
	const int reach = int(p1.x);
	const ivec2 base = p * 4;
	float cur[16];
	for (int k = 0; k < 16; k++)
	{
		cur[k] = texelFetch(tex1, min(base + ivec2(k & 3, k >> 2), lim), 0).r;
	}
	// No motion first, and kept unless another vector is clearly better: in
	// flat, plain areas (the ground under a shadow) every vector matches about
	// as well, and the best by a hair is noise
	float error = 0.0;
	for (int k = 0; k < 16; k++)
	{
		error += abs(cur[k] - texelFetch(tex0, min(base + ivec2(k & 3, k >> 2), lim), 0).r) / 16.0;
	}
	float best = error - 0.006;
	vec2 best_v = vec2(0.0);
	for (int dy = -reach; dy <= reach; dy++)
	{
		for (int dx = -reach; dx <= reach; dx++)
		{
			float s = 0.0;
			for (int k = 0; k < 16; k++)
			{
				s += abs(cur[k] - texelFetch(tex0, clamp(base + ivec2(k & 3, k >> 2) + ivec2(dx, dy), ivec2(0), lim), 0).r);
			}
			s /= 16.0;
			if (s + 0.0005 * float(abs(dx) + abs(dy)) < best)
			{
				best = s + 0.0005 * float(abs(dx) + abs(dy));
				error = s;
				best_v = vec2(dx, dy);
			}
		}
	}
	// The match's own error, without the favours
	imageStore(dst, p, vec4(best_v, error, 0.0));
}
)",
			// pass_fine_search: tex0 the last frame's working brightness, tex1
			// this one's, tex2 the coarse vectors; p0 = (the working size, its
			// 8x8 block grid), p1.xy the coarse grid. Each block starts from no
			// motion and from the coarse vectors of its coarse block and its four
			// neighbours (four times as long here), and looks 2 texels around each.
			// Its z the match's own error, without the favour shorter vectors get:
			// the frames between trust a vector by it, and with the favour in it
			// (build 107) every long vector looked like a poor match
			R"(
void main()
{
	const ivec2 p = ivec2(gl_GlobalInvocationID.xy);
	if (p.x >= int(p0.z) || p.y >= int(p0.w)) return;
	const ivec2 lim = ivec2(p0.xy) - 1;
	const ivec2 coarse_lim = ivec2(p1.xy) - 1;
	const ivec2 base = p * 8;
	float cur[64];
	for (int k = 0; k < 64; k++)
	{
		cur[k] = texelFetch(tex1, min(base + ivec2(k & 7, k >> 3), lim), 0).r;
	}
	const ivec2 cb = p / 2;
	ivec2 starts[6];
	starts[0] = ivec2(0);
	starts[1] = ivec2(round(texelFetch(tex2, clamp(cb, ivec2(0), coarse_lim), 0).xy * 4.0));
	starts[2] = ivec2(round(texelFetch(tex2, clamp(cb + ivec2(1, 0), ivec2(0), coarse_lim), 0).xy * 4.0));
	starts[3] = ivec2(round(texelFetch(tex2, clamp(cb - ivec2(1, 0), ivec2(0), coarse_lim), 0).xy * 4.0));
	starts[4] = ivec2(round(texelFetch(tex2, clamp(cb + ivec2(0, 1), ivec2(0), coarse_lim), 0).xy * 4.0));
	starts[5] = ivec2(round(texelFetch(tex2, clamp(cb - ivec2(0, 1), ivec2(0), coarse_lim), 0).xy * 4.0));
	// No motion first, kept unless another vector is clearly better (as in the coarse search)
	float error = 0.0;
	for (int k = 0; k < 64; k++)
	{
		error += abs(cur[k] - texelFetch(tex0, min(base + ivec2(k & 7, k >> 3), lim), 0).r) / 64.0;
	}
	float best = error - 0.006;
	vec2 best_v = vec2(0.0);
	for (int c = 0; c < 6; c++)
	{
		for (int dy = -2; dy <= 2; dy++)
		{
			for (int dx = -2; dx <= 2; dx++)
			{
				const ivec2 v = starts[c] + ivec2(dx, dy);
				float s = 0.0;
				for (int k = 0; k < 64; k++)
				{
					s += abs(cur[k] - texelFetch(tex0, clamp(base + ivec2(k & 7, k >> 3) + v, ivec2(0), lim), 0).r);
				}
				s /= 64.0;
				if (s + 0.0003 * float(abs(v.x) + abs(v.y)) < best)
				{
					best = s + 0.0003 * float(abs(v.x) + abs(v.y));
					error = s;
					best_v = vec2(v);
				}
			}
		}
	}
	// The match's own error, without the favours
	imageStore(dst, p, vec4(best_v, error, 0.0));
}
)",
			// pass_median: tex0 the vectors; p0.xy their grid. Each vector's
			// parts become the medians of its 3x3 neighbourhood's, which drops
			// lone wrong ones; the match error stays its own
			R"(
void main()
{
	const ivec2 p = ivec2(gl_GlobalInvocationID.xy);
	if (p.x >= int(p0.x) || p.y >= int(p0.y)) return;
	const ivec2 lim = ivec2(p0.xy) - 1;
	float xs[9];
	float ys[9];
	int n = 0;
	for (int j = -1; j <= 1; j++)
	{
		for (int i = -1; i <= 1; i++)
		{
			const vec4 v = texelFetch(tex0, clamp(p + ivec2(i, j), ivec2(0), lim), 0);
			xs[n] = v.x;
			ys[n] = v.y;
			n++;
		}
	}
	for (int a = 1; a < 9; a++)
	{
		const float x = xs[a];
		const float y = ys[a];
		int b = a - 1;
		while (b >= 0 && xs[b] > x)
		{
			xs[b + 1] = xs[b];
			b--;
		}
		xs[b + 1] = x;
		b = a - 1;
		while (b >= 0 && ys[b] > y)
		{
			ys[b + 1] = ys[b];
			b--;
		}
		ys[b + 1] = y;
	}
	imageStore(dst, p, vec4(xs[4], ys[4], texelFetch(tex0, p, 0).z, 0.0));
}
)",
			// pass_between: tex0 the last frame (its own size), tex1 this one,
			// tex2 the vectors; p0 = (this frame's used part as a share of its
			// texture, the output size), p1 = (the working size, the share of
			// the vector grid it covers), p2.x the time t from the last frame
			// (0) to this one (1). Each vector, from a point of this frame to
			// where it was in the last, is followed t of the way back from here
			// in the last frame and 1 - t of the way on in this one, and the two
			// blended by t where they agree closely. Which vector: the one
			// blended between the four nearest blocks', those four, and those of
			// the blocks 8, 16 and 24 away in the four directions, the one whose
			// two ends agree best, less its block's own match error (nearer ones
			// a little favoured): at a moving thing's edge the blend of its
			// vector and the background's belongs to neither and smeared the
			// edge (build 106 on my console), and where the world comes into the
			// frame no block's own vector is right: what came in was not in the
			// last frame. An end holds where it falls inside its frame, on a
			// block moving with the vector or one that matched worse than it
			// (which says nothing): in a camera turn round the player, the end
			// that falls on the player (a block still, and sure of it) is where
			// the world was hidden. With both ends holding, they are blended as
			// before; with one, that end alone, and with both but apart, the one
			// nearer in time, each moved along its vector. Build 110 put the
			// nearer frame unmoved there, and a quick camera turn in GTA IV
			// looked wrong on my console: in the model of these passes the
			// screen's sides, the world beside the player and the radar, and
			// every patch matched poorly stood still while the rest moved, as
			// if torn off it. In the model, a
			// turn of 40-120 texels a game frame at the working size went from
			// 18.8-24.4 dB to 27.9-33.1, the side the world comes in at from
			// 15.1-15.7 to 33.8-40.7, and round the player from 17.0-20.6 to
			// 19.6-31.6; the earlier scenes' whole frames -0.5 to +4.7 dB (most
			// gained), the HUD exact as before. The end alone is read unfiltered,
			// its texel as it is: a filtered read between texels softened GTA
			// IV's dithered shadows' grain (contrast 0.116-0.149 in the model, the
			// real frames' 0.157-0.159; read as texels 0.137-0.156, build 110's
			// 0.140-0.158). The frame nearer in time as it is where the
			// pixel is the same in both frames unmoved (the HUD, the radar's
			// frame and text, which the world's vectors dragged along), or where
			// neither end of any vector holds
			R"(
ivec2 block_of(const vec2 frame_uv, const ivec2 grid)
{
	return clamp(ivec2(floor(frame_uv * p1.zw * vec2(grid))), ivec2(0), grid - 1);
}

// The end at frame_uv holds: in the frame, and on a block moving with v, or on
// one whose own match was worse than v's
bool holds(const vec2 frame_uv, const vec4 v, const ivec2 grid)
{
	if (any(lessThan(frame_uv, vec2(0.0))) || any(greaterThan(frame_uv, vec2(1.0))))
	{
		return false;
	}
	const vec4 b = texelFetch(tex2, block_of(frame_uv, grid), 0);
	return length(b.xy - v.xy) <= 2.0 + 0.15 * length(v.xy) || b.z > v.z + 0.01;
}

void main()
{
	const ivec2 p = ivec2(gl_GlobalInvocationID.xy);
	if (p.x >= int(p0.z) || p.y >= int(p0.w)) return;
	const float t = p2.x;
	const vec2 uv = (vec2(p) + 0.5) / p0.zw;
	const vec2 grid_uv = uv * p1.zw;
	const ivec2 grid = textureSize(tex2, 0);
	const ivec2 near = ivec2(floor(grid_uv * vec2(grid) - 0.5));
	const ivec2 own = clamp(ivec2(floor(grid_uv * vec2(grid))), ivec2(0), grid - 1);
	vec4 mv = vec4(0.0);
	vec3 last = vec3(0.0);
	vec3 next = vec3(0.0);
	vec2 last_uv = uv;
	vec2 next_uv = uv;
	bool last_holds = false;
	bool next_holds = false;
	float best = 1e9;
	for (int c = 0; c < 17; c++)
	{
		vec4 v;
		float favour;
		if (c == 0)
		{
			v = texture(tex2, grid_uv);
			favour = 0.0;
		}
		else if (c < 5)
		{
			v = texelFetch(tex2, clamp(near + ivec2((c - 1) & 1, (c - 1) >> 1), ivec2(0), grid - 1), 0);
			favour = 0.004;
		}
		else
		{
			const int k = c - 5;
			const ivec2 dir = (k & 3) == 0 ? ivec2(1, 0) : (k & 3) == 1 ? ivec2(-1, 0) : (k & 3) == 2 ? ivec2(0, 1) : ivec2(0, -1);
			v = texelFetch(tex2, clamp(own + dir * (8 * ((k >> 2) + 1)), ivec2(0), grid - 1), 0);
			favour = 0.008;
		}
		const vec2 d = v.xy / p1.xy;
		const vec2 lu = uv + t * d;
		const vec2 nu = uv - (1.0 - t) * d;
		const vec3 l = texture(tex0, lu).rgb;
		const vec3 n = texture(tex1, nu * p0.xy).rgb;
		const bool lh = holds(lu, v, grid);
		const bool nh = holds(nu, v, grid);
		const vec3 g = abs(l - n);
		const float s = (lh && nh ? max(max(g.r, g.g), g.b) : lh || nh ? 0.06 : 1.0) + favour + v.z;
		if (s < best)
		{
			best = s;
			mv = v;
			last = l;
			next = n;
			last_uv = lu;
			next_uv = nu;
			last_holds = lh;
			next_holds = nh;
		}
	}
	const vec3 here = texture(tex1, uv * p0.xy).rgb;
	const vec3 there = texture(tex0, uv).rgb;
	const vec3 nearer = t < 0.5 ? there : here;
	const ivec2 size0 = textureSize(tex0, 0);
	const ivec2 size1 = textureSize(tex1, 0);
	const vec3 last_texel = texelFetch(tex0, clamp(ivec2(floor(last_uv * vec2(size0))), ivec2(0), size0 - 1), 0).rgb;
	const vec3 next_texel = texelFetch(tex1, clamp(ivec2(floor(next_uv * p0.xy * vec2(size1))), ivec2(0), size1 - 1), 0).rgb;
	const vec3 one_end = last_holds && next_holds ? (t < 0.5 ? last_texel : next_texel) : last_holds ? last_texel : next_holds ? next_texel : nearer;
	const vec3 moved_gap = abs(last - next);
	const vec3 still_gap = abs(there - here);
	const float trust = 1.0 - smoothstep(0.035, 0.11, mv.z);
	const float agree = 1.0 - smoothstep(0.03, 0.1, max(max(moved_gap.r, moved_gap.g), moved_gap.b));
	const float still = 1.0 - smoothstep(0.012, 0.04, max(max(still_gap.r, still_gap.g), still_gap.b));
	const vec3 between = mix(one_end, mix(last, next, t), last_holds && next_holds ? trust * agree : 0.0);
	imageStore(dst, p, vec4(mix(between, nearer, still), 1.0));
}
)",
			// pass_keep: tex0 the frame; p0 = (its used part as a share of its
			// texture, the output size). Sampled, not blitted: a game's display
			// buffer may be in a format a blit cannot read
			R"(
void main()
{
	const ivec2 p = ivec2(gl_GlobalInvocationID.xy);
	if (p.x >= int(p0.z) || p.y >= int(p0.w)) return;
	imageStore(dst, p, vec4(texture(tex0, (vec2(p) + 0.5) / p0.zw * p0.xy).rgb, 1.0));
}
)",
		};

		template <int Kind>
		struct frame_generation_pass : compute_task
		{
			const vk::image_view* m_inputs[3]{};
			const vk::image_view* m_output = nullptr;
			std::unique_ptr<vk::sampler> m_sampler;

			frame_generation_pass()
			{
				m_src = std::string(c_header) + c_bodies[Kind];
				const usz at = m_src.find("%FORMAT%");
				m_src.replace(at, 8, c_formats[Kind]);

				ssbo_count = 0;
				use_push_constants = true;
				push_constants_size = 48;
				create();
			}

			std::vector<glsl::program_input> get_inputs() override
			{
				auto result = compute_task::get_inputs();
				for (u32 i = 0; i < 3; i++)
				{
					result.push_back(glsl::program_input::make(::glsl::program_domain::glsl_compute_program, "tex" + std::to_string(i), vk::glsl::input_type_texture, 0, i));
				}
				result.push_back(glsl::program_input::make(::glsl::program_domain::glsl_compute_program, "dst", vk::glsl::input_type_storage_texture, 0, 3));
				return result;
			}

			void bind_resources(const vk::command_buffer&) override
			{
				if (!m_sampler)
				{
					const auto pdev = vk::get_current_renderer();
					m_sampler = std::make_unique<vk::sampler>(*pdev,
						VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
						VK_FALSE, 0.f, 1.f, 0.f, 0.f, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST, VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK);
				}

				for (u32 i = 0; i < 3; i++)
				{
					m_program->bind_uniform({ *m_inputs[i], *m_sampler }, 0, i);
				}
				m_program->bind_uniform({ *m_output }, 0, 3);
			}

			void run(const vk::command_buffer& cmd, std::initializer_list<vk::viewable_image*> inputs, vk::viewable_image* output, u32 width, u32 height, const std::array<f32, 12>& params)
			{
				const auto view = [](vk::viewable_image* image) { return image->get_view(rsx::default_remap_vector.with_encoding(VK_REMAP_IDENTITY)); };
				u32 i = 0;
				for (vk::viewable_image* input : inputs)
				{
					m_inputs[i++] = view(input);
				}
				for (; i < 3; i++)
				{
					m_inputs[i] = m_inputs[0];
				}
				m_output = view(output);

				if (!m_program)
				{
					load_program(cmd);
				}
				vkCmdPushConstants(cmd, m_program->layout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, push_constants_size, params.data());
				compute_task::run(cmd, utils::aligned_div(width, 8u), utils::aligned_div(height, 8u), 1);
			}
		};

		// Each pass's writes seen by the next pass's reads and writes, and by
		// copies; and the last frame's reads of these images done before this
		// one writes them
		void barrier(const vk::command_buffer& cmd, VkPipelineStageFlags from = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT)
		{
			VkMemoryBarrier memory = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };
			memory.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
			memory.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
			vkCmdPipelineBarrier(cmd, from, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &memory, 0, nullptr, 0, nullptr);
		}

		std::unique_ptr<vk::viewable_image> make_image(u32 width, u32 height, VkFormat format)
		{
			const auto pdev = vk::get_current_renderer();
			auto image = std::make_unique<vk::viewable_image>(
				*pdev, pdev->get_memory_mapping().device_local, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_TYPE_2D, format, std::max(width, 1u), std::max(height, 1u), 1, 1, 1, VK_SAMPLE_COUNT_1_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
				VK_IMAGE_CREATE_ALLOW_NULL_RPCS3, VMM_ALLOCATION_POOL_SWAPCHAIN, RSX_FORMAT_CLASS_COLOR);
			if (image->value == VK_NULL_HANDLE)
			{
				return nullptr;
			}
			return image;
		}

		// The working size: at most the quality's width across, the frame's shape kept
		void working_size(u32 width, u32 height, u32 quality, u32& work_w, u32& work_h)
		{
			work_w = std::min(width, c_quality[quality].width);
			work_h = std::max(1u, static_cast<u32>(std::lround(static_cast<f64>(height) * work_w / width)));
		}
	}

	frame_generator::~frame_generator()
	{
		// The renderer's teardown: the device is idle, the images go at once
	}

	void frame_generator::dispose()
	{
		const auto drop = [](std::unique_ptr<vk::viewable_image>& image)
		{
			if (image)
			{
				vk::get_resource_manager()->dispose(image);
			}
		};
		drop(m_prev_color);
		for (auto& image : m_luma) drop(image);
		for (auto& image : m_luma_coarse) drop(image);
		drop(m_vectors_coarse);
		drop(m_vectors);
		drop(m_vectors_smooth);
		for (auto& image : m_output) drop(image);
		m_width = m_height = 0;
		m_have_prev = false;
	}

	void frame_generator::allocate(u32 width, u32 height, u32 quality)
	{
		dispose();

		working_size(width, height, quality, m_work_w, m_work_h);
		const u32 coarse_w = utils::aligned_div(m_work_w, 4u);
		const u32 coarse_h = utils::aligned_div(m_work_h, 4u);

		m_prev_color = make_image(width, height, VK_FORMAT_R8G8B8A8_UNORM);
		bool outputs = true;
		for (auto& image : m_output)
		{
			image = make_image(width, height, VK_FORMAT_R8G8B8A8_UNORM);
			outputs = outputs && image;
		}
		for (u32 i = 0; i < 2; i++)
		{
			m_luma[i] = make_image(m_work_w, m_work_h, VK_FORMAT_R32_SFLOAT);
			m_luma_coarse[i] = make_image(coarse_w, coarse_h, VK_FORMAT_R32_SFLOAT);
		}
		m_vectors_coarse = make_image(utils::aligned_div(coarse_w, 4u), utils::aligned_div(coarse_h, 4u), VK_FORMAT_R16G16B16A16_SFLOAT);
		m_vectors = make_image(utils::aligned_div(m_work_w, 8u), utils::aligned_div(m_work_h, 8u), VK_FORMAT_R16G16B16A16_SFLOAT);
		m_vectors_smooth = make_image(utils::aligned_div(m_work_w, 8u), utils::aligned_div(m_work_h, 8u), VK_FORMAT_R16G16B16A16_SFLOAT);

		if (!m_prev_color || !outputs || !m_luma[0] || !m_luma[1] || !m_luma_coarse[0] || !m_luma_coarse[1] || !m_vectors_coarse || !m_vectors || !m_vectors_smooth)
		{
			rsx_log.warning("Frame generation: no memory for its images at %ux%u", width, height);
			dispose();
			return;
		}

		m_width = width;
		m_height = height;
		m_quality = quality;
		rsx_log.notice("Frame generation: %ux%u, motion at %ux%u (%s)", width, height, m_work_w, m_work_h, c_quality[quality].name);
	}

	void frame_generator::reset()
	{
		m_have_prev = false;
	}

	u32 frame_generator::process(const vk::command_buffer& cmd, vk::viewable_image* cur, u32 width, u32 height, u32 count, u32 quality,
		vk::viewable_image* (&out)[max_frames])
	{
		if (!cur || !width || !height)
		{
			return 0;
		}
		width = std::min<u32>(width, cur->width());
		height = std::min<u32>(height, cur->height());
		quality = std::min<u32>(quality, static_cast<u32>(std::size(c_quality)) - 1);
		count = std::min(count, max_frames);

		if (width != m_width || height != m_height || quality != m_quality)
		{
			allocate(width, height, quality);
			if (!m_width)
			{
				return 0;
			}
			for (auto* image : { m_prev_color.get(), m_output[0].get(), m_output[1].get(), m_output[2].get(), m_luma[0].get(), m_luma[1].get(),
				m_luma_coarse[0].get(), m_luma_coarse[1].get(), m_vectors_coarse.get(), m_vectors.get(), m_vectors_smooth.get() })
			{
				image->change_layout(cmd, VK_IMAGE_LAYOUT_GENERAL);
			}
		}

		const bool make = count && m_have_prev;
		const f32 uv_x = static_cast<f32>(width) / cur->width();
		const f32 uv_y = static_cast<f32>(height) / cur->height();
		const f32 work_w = static_cast<f32>(m_work_w);
		const f32 work_h = static_cast<f32>(m_work_h);
		const u32 coarse_w = utils::aligned_div(m_work_w, 4u);
		const u32 coarse_h = utils::aligned_div(m_work_h, 4u);
		const u32 coarse_grid_w = utils::aligned_div(coarse_w, 4u);
		const u32 coarse_grid_h = utils::aligned_div(coarse_h, 4u);
		const u32 grid_w = utils::aligned_div(m_work_w, 8u);
		const u32 grid_h = utils::aligned_div(m_work_h, 8u);

		// What the last frame's presentation read of these images is done
		barrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

		cur->push_layout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		// This frame's brightness, at the working size and a quarter of it
		vk::get_compute_task<frame_generation_pass<pass_brightness>>()->run(cmd, { cur }, m_luma[1].get(), m_work_w, m_work_h,
			{ uv_x, uv_y, work_w, work_h });
		barrier(cmd);
		vk::get_compute_task<frame_generation_pass<pass_quarter>>()->run(cmd, { m_luma[1].get() }, m_luma_coarse[1].get(), coarse_w, coarse_h,
			{ work_w, work_h, static_cast<f32>(coarse_w), static_cast<f32>(coarse_h) });
		barrier(cmd);

		u32 made = 0;
		if (make)
		{
			vk::get_compute_task<frame_generation_pass<pass_coarse_search>>()->run(cmd, { m_luma_coarse[0].get(), m_luma_coarse[1].get() }, m_vectors_coarse.get(), coarse_grid_w, coarse_grid_h,
				{ static_cast<f32>(coarse_w), static_cast<f32>(coarse_h), static_cast<f32>(coarse_grid_w), static_cast<f32>(coarse_grid_h),
				  static_cast<f32>(c_quality[quality].reach) });
			barrier(cmd);
			vk::get_compute_task<frame_generation_pass<pass_fine_search>>()->run(cmd, { m_luma[0].get(), m_luma[1].get(), m_vectors_coarse.get() }, m_vectors.get(), grid_w, grid_h,
				{ work_w, work_h, static_cast<f32>(grid_w), static_cast<f32>(grid_h), static_cast<f32>(coarse_grid_w), static_cast<f32>(coarse_grid_h) });
			barrier(cmd);
			vk::get_compute_task<frame_generation_pass<pass_median>>()->run(cmd, { m_vectors.get() }, m_vectors_smooth.get(), grid_w, grid_h,
				{ static_cast<f32>(grid_w), static_cast<f32>(grid_h) });
			barrier(cmd);

			// The frames between, each at its time: with three, a quarter, a half
			// and three quarters of the way from the last frame to this one
			for (u32 k = 0; k < count; k++)
			{
				const f32 t = static_cast<f32>(k + 1) / static_cast<f32>(count + 1);
				vk::get_compute_task<frame_generation_pass<pass_between>>()->run(cmd, { m_prev_color.get(), cur, m_vectors_smooth.get() }, m_output[k].get(), width, height,
					{ uv_x, uv_y, static_cast<f32>(width), static_cast<f32>(height), work_w, work_h, work_w / (grid_w * 8.f), work_h / (grid_h * 8.f), t });
				out[k] = m_output[k].get();
			}
			made = count;
			g_ps5_frames_generated += count;
		}

		// This frame becomes the last one: its picture, and its brightness.
		// (The frames between have read the last picture by now)
		barrier(cmd);
		vk::get_compute_task<frame_generation_pass<pass_keep>>()->run(cmd, { cur }, m_prev_color.get(), width, height,
			{ uv_x, uv_y, static_cast<f32>(width), static_cast<f32>(height) });
		cur->pop_layout(cmd);
		barrier(cmd);
		std::swap(m_luma[0], m_luma[1]);
		std::swap(m_luma_coarse[0], m_luma_coarse[1]);
		m_have_prev = true;

		if (made)
		{
			// The frames between, written, for the presentation to read
			VkMemoryBarrier memory = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };
			memory.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			memory.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT;
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &memory, 0, nullptr, 0, nullptr);
		}
		return made;
	}
}
