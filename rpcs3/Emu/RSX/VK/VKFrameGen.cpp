#include "stdafx.h"
#include "VKFrameGen.h"

#include "VKHelpers.h"
#include "VKResourceManager.h"
#include "vkutils/barriers.h"
#include "vkutils/device.h"

#include "Emu/System.h"
#include "Emu/Cell/timers.hpp"
#include "Utilities/File.h"
#include "util/asm.hpp"

#include <ctime>
#include <thread>
#include <sys/stat.h>

// The frames a capture writes, as PNG (stb_image_write, its own here)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#ifdef _MSC_VER
#pragma warning(push, 0)
#include <stb_image_write.h>
#include <stb_truetype.h>
#pragma warning(pop)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include <stb_image_write.h>
#include <stb_truetype.h>
#pragma GCC diagnostic pop
#endif

// PS5: for the trace's status line, since the title started: the frames made;
// the RSX thread's time making and showing them, and in it waiting for the
// GPU (VKPresent.cpp); the frames made but dropped, no image free to show
// them in; and the RSX thread's time waiting for an image to show the game's
// own frames [0] and those made [1] in; the game's frames whose camera
// moved too quickly for frames between, shown as the real ones instead; and
// those whose camera's field held (pass_camera)
atomic_t<u64> g_ps5_frames_generated = 0;
atomic_t<u64> g_ps5_frame_gen_us = 0;
atomic_t<u64> g_ps5_frame_gen_gpu_wait_us = 0;
atomic_t<u64> g_ps5_frames_dropped = 0;
atomic_t<u64> g_ps5_image_wait_us[2] = {};
atomic_t<u64> g_ps5_frames_quick_moves = 0;
atomic_t<u64> g_ps5_frames_camera_field = 0;

// The pause menu's Save frame generation's frames: the sets wanted
atomic_t<u32> g_ps5_frame_gen_capture = 0;

namespace vk
{
	namespace
	{
		// Every pass: 8x8 invocations a group, four inputs sampled (unused ones
		// bound to the first), one output written, twelve numbers pushed
		constexpr const char* c_header = R"(
#version 450
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout(set = 0, binding = 0) uniform sampler2D tex0;
layout(set = 0, binding = 1) uniform sampler2D tex1;
layout(set = 0, binding = 2) uniform sampler2D tex2;
layout(set = 0, binding = 3) uniform sampler2D tex3;
layout(set = 0, binding = 4, %FORMAT%) uniform writeonly image2D dst;
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
			pass_motion,         // the camera's motion, voted from the rough vectors
			pass_camera,         // the camera's motion as one smooth field over the frame
			pass_camera_check,   // each block: whether it moved with that field
		};

		constexpr const char* c_formats[] = { "r32f", "r32f", "rgba16f", "rgba16f", "rgba16f", "rgba8", "rgba8", "rgba32f", "rgba32f", "r32f" };

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
			// reach, p1.y the favour shorter vectors get a texel. Each block of
			// this frame is sought in the last one up to the reach away
			// (c_quality); the vector points from here to there, its z the mean
			// difference per texel, its w how much better it matched than no
			// motion (how clearly the block moved). Shorter vectors are a little
			// favoured, but only a little: at build 107's 0.0015 a quarter texel
			// the favour outweighed a true match 14 texels away (0.0005 now).
			// Also run at a sixteenth of the working size (0.002 a texel there,
			// the same per pixel), for the camera's motion (pass_motion). The
			// margin no motion keeps and the favour both shrink with the block's
			// contrast (its texels' mean distance from their mean), to 15% of
			// themselves below 0.06: GTA IV's asphalt is that faint, a true move
			// beat no motion there by less than the margin, and in the model of
			// these passes (a street driven at 0.7 and 1.4 metres a game frame)
			// the road stood still between the frames while its markings moved;
			// so scaled, the road gained 2.4 to 3 dB. Only with p1.z set (the
			// Method setting's Build 117)
			R"(
void main()
{
	const ivec2 p = ivec2(gl_GlobalInvocationID.xy);
	if (p.x >= int(p0.z) || p.y >= int(p0.w)) return;
	const ivec2 lim = ivec2(p0.xy) - 1;
	const int reach = int(p1.x);
	const ivec2 base = p * 4;
	float cur[16];
	float mean = 0.0;
	for (int k = 0; k < 16; k++)
	{
		cur[k] = texelFetch(tex1, min(base + ivec2(k & 3, k >> 2), lim), 0).r;
		mean += cur[k] / 16.0;
	}
	float contrast = 0.0;
	for (int k = 0; k < 16; k++)
	{
		contrast += abs(cur[k] - mean) / 16.0;
	}
	const float scale = p1.z > 0.5 ? clamp(contrast / 0.06, 0.15, 1.0) : 1.0;
	const float favour = p1.y * scale;
	// No motion first, and kept unless another vector is clearly better: in
	// flat, plain areas (the ground under a shadow) every vector matches about
	// as well, and the best by a hair is noise
	float error = 0.0;
	for (int k = 0; k < 16; k++)
	{
		error += abs(cur[k] - texelFetch(tex0, min(base + ivec2(k & 3, k >> 2), lim), 0).r) / 16.0;
	}
	const float still_error = error;
	float best = error - 0.006 * scale;
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
			if (s + favour * float(abs(dx) + abs(dy)) < best)
			{
				best = s + favour * float(abs(dx) + abs(dy));
				error = s;
				best_v = vec2(dx, dy);
			}
		}
	}
	// The match's own error, without the favours
	imageStore(dst, p, vec4(best_v, error, still_error - error));
}
)",
			// pass_fine_search: tex0 the last frame's working brightness, tex1
			// this one's, tex2 the coarse vectors; p0 = (the working size, its
			// 8x8 block grid), p1.xy the coarse grid. Each block starts from no
			// motion and from the coarse vectors of its coarse block and its four
			// neighbours (four times as long here), and looks 2 texels around each.
			// Its z the match's own error, without the favour shorter vectors get:
			// the frames between trust a vector by it, and with the favour in it
			// (build 107) every long vector looked like a poor match. Its w how
			// much better it matched than no motion (pass_camera weighs it by
			// that). The margin and the favour scale with the block's contrast,
			// as in the coarse search, with p1.z set
			R"(
void main()
{
	const ivec2 p = ivec2(gl_GlobalInvocationID.xy);
	if (p.x >= int(p0.z) || p.y >= int(p0.w)) return;
	const ivec2 lim = ivec2(p0.xy) - 1;
	const ivec2 coarse_lim = ivec2(p1.xy) - 1;
	const ivec2 base = p * 8;
	float cur[64];
	float mean = 0.0;
	for (int k = 0; k < 64; k++)
	{
		cur[k] = texelFetch(tex1, min(base + ivec2(k & 7, k >> 3), lim), 0).r;
		mean += cur[k] / 64.0;
	}
	float contrast = 0.0;
	for (int k = 0; k < 64; k++)
	{
		contrast += abs(cur[k] - mean) / 64.0;
	}
	const float scale = p1.z > 0.5 ? clamp(contrast / 0.06, 0.15, 1.0) : 1.0;
	const float favour = 0.0003 * scale;
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
	const float still_error = error;
	float best = error - 0.006 * scale;
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
				if (s + favour * float(abs(v.x) + abs(v.y)) < best)
				{
					best = s + favour * float(abs(v.x) + abs(v.y));
					error = s;
					best_v = vec2(v);
				}
			}
		}
	}
	// The match's own error, without the favours; and how clearly it moved
	imageStore(dst, p, vec4(best_v, error, still_error - error));
}
)",
			// pass_median: tex0 the vectors, tex1 each block's check against the
			// camera's field (pass_camera_check), tex2 the last frame's working
			// brightness, tex3 this one's; p0 = (their grid, the working size).
			// Each vector's parts become the medians of its 3x3 neighbourhood's,
			// which drops lone wrong ones; but a block keeps its own where it
			// matches its block clearly better than the median does (by 0.01 a
			// texel): a thing a block or two across moving its own way, the
			// player's head in a camera turn, lost its vectors to the
			// background's, and GTA IV's radar turning in its ring too (in the
			// model of these passes, the radar 3.5 to 4.5 dB better so); with
			// p1.x set (the Method setting's Builds 117). The match error stays
			// its own, and w is the check
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
	const vec4 own = texelFetch(tex0, p, 0);
	vec2 v = vec2(xs[4], ys[4]);
	if (p1.x > 0.5)
	{
		const ivec2 median = ivec2(round(v));
		const ivec2 work_lim = ivec2(p0.zw) - 1;
		const ivec2 base = p * 8;
		float median_error = 0.0;
		for (int k = 0; k < 64; k++)
		{
			const ivec2 q = base + ivec2(k & 7, k >> 3);
			median_error += abs(texelFetch(tex3, min(q, work_lim), 0).r - texelFetch(tex2, clamp(q + median, ivec2(0), work_lim), 0).r);
		}
		median_error /= 64.0;
		v = own.z + 0.01 < median_error ? own.xy : v;
	}
	imageStore(dst, p, vec4(v, own.z, texelFetch(tex1, p, 0).r));
}
)",
			// pass_between: tex0 the last frame (its own size), tex1 this one,
			// tex2 the vectors, tex3 the camera's field (pass_camera); p0 =
			// (this frame's used part as a share of its
			// texture, the output size), p1 = (the working size, the share of
			// the vector grid it covers), p2.x the time t from the last frame
			// (0) to this one (1). Each vector, from a point of this frame to
			// where it was in the last, is followed t of the way back from here
			// in the last frame and 1 - t of the way on in this one, and the two
			// blended by t where they agree closely. Which vector: of the one
			// blended between the four nearest blocks' and those four, the one
			// whose two ends agree best (the blended one a little favoured): at
			// a moving thing's edge the blend of its vector and the background's
			// belongs to neither and smeared the edge (build 106 on my console;
			// in a model of these passes, picking per pixel gained about 1 dB
			// overall and up to 1.6 dB at a moving object's edges). Anywhere else
			// the frame nearer in time as it is: where the block matched poorly,
			// and where the pixel is the same in both frames unmoved
			// (the HUD, the radar's frame and text, which the world's vectors
			// dragged along) beside a block that did not move: without that
			// block, a thin thing crossing a plain background (a lamp post over
			// the sky) was erased, both frames showing the sky where it was to
			// be, and lamp posts would flicker in a turn (in the model of these
			// passes, a city turned 1.5 to 4.5 degrees a game frame: the posts
			// kept 35-74% of their pixels, 51-78% with it; the HUD exact either
			// way). And the frame nearer in time as it is everywhere while the
			// camera moves too quickly (pass_motion, carried in tex3's third
			// texel). The camera's field is a sixth candidate, favoured over the
			// rest, beside a block that moved with it (the vectors' w): on a
			// building's rows of windows the blocks' own vectors are often a
			// window off, and the frame between jumped half a window and back
			// (GTA IV's buildings looked off and zigzagged on my console, build
			// 114); the field is the whole frame's motion, fitted to a fraction
			// of a texel. Not where no block moved with it: GTA IV's radar turns
			// within its ring, and the field pulled the world's motion into it.
			// A seventh, favoured well over all (0.04): the block the pixel is
			// in, as the field where that block moved with it, else as its own
			// vector. Picking by the two ends alone, a walking player's head lost
			// to the wall behind it, whose ends agree where an animated head's
			// never quite do (GTA IV, Niko's head gone in turns, build 115 on my
			// console; in the model of these passes, a head over a wall in a
			// turn of 3 and 6 degrees a game frame: 75% and 70% of it right,
			// 87% and 83% with this and the median that keeps a block's own).
			// And where the chosen vector's two ends disagree, the one that sees
			// the point, not the nearer frame as it is: if the thing at the
			// point's place in this frame moves with the vector, this frame sees
			// it and the last frame's view was covered (beside the player in a
			// turn, the wall the player hid); else the last frame's end, if the
			// thing at its place there moves with the vector in this frame too
			// (else a thing standing still covers both, and the last frame's
			// end drew a copy of a piece of the player beside the player in the
			// model); else the nearer frame. Either end
			// is one frame's own picture, never two blended where they disagree
			// (GTA IV's shadows shimmer by themselves, and averaging two patterns
			// softened every other frame's on my console, build 94). In the
			// model the walls of a turn gained 0.4 to 2.7 dB so, and the outline
			// of the player no longer showed in them. The seventh candidate and
			// the ends that see only with p2.y set (the Method setting's Builds
			// 117); else as build 115, the nearer frame wherever the ends disagree
			R"(
void main()
{
	const ivec2 p = ivec2(gl_GlobalInvocationID.xy);
	if (p.x >= int(p0.z) || p.y >= int(p0.w)) return;
	const float t = p2.x;
	const vec2 uv = (vec2(p) + 0.5) / p0.zw;
	const vec2 grid_uv = uv * p1.zw;
	const ivec2 grid = textureSize(tex2, 0);
	const ivec2 near = ivec2(floor(grid_uv * vec2(grid) - 0.5));
	const vec4 camera = texelFetch(tex3, ivec2(2, 0), 0);
	const bool holds = camera.x > 0.5;
	// The field here, in working texels, x and y from the frame's centre in
	// half widths; its w marks it as the field
	const vec4 m0 = texelFetch(tex3, ivec2(0, 0), 0);
	const vec4 m1 = texelFetch(tex3, ivec2(1, 0), 0);
	const float fx = 2.0 * uv.x - 1.0;
	const float fy = (2.0 * uv.y - 1.0) * p1.y / p1.x;
	const vec4 field = vec4(m0.x + m0.y * fx + m0.z * fy + m1.z * fx * fx + m1.w * fx * fy,
		m0.w + m1.x * fx + m1.y * fy + m1.z * fx * fy + m1.w * fy * fy, 0.0, 2.0);
	const vec4 own = texelFetch(tex2, clamp(ivec2(floor(grid_uv * vec2(grid))), ivec2(0), grid - 1), 0);
	vec4 mv = vec4(0.0);
	vec3 last = vec3(0.0);
	vec3 next = vec3(0.0);
	float best = 1e9;
	bool beside_still = false;
	bool beside_camera = false;
	const bool keeps_own = p2.y > 0.5;
	for (int c = 0; c < (keeps_own ? 7 : 6); c++)
	{
		vec4 v;
		float favour = c == 0 ? 0.0 : 0.004;
		if (c < 5)
		{
			v = c == 0 ? texture(tex2, grid_uv)
				: texelFetch(tex2, clamp(near + ivec2((c - 1) & 1, (c - 1) >> 1), ivec2(0), grid - 1), 0);
			beside_still = beside_still || (c > 0 && length(v.xy) <= 1.0);
			beside_camera = beside_camera || (c > 0 && v.w > 0.5);
		}
		else if (c == 5)
		{
			if (!holds || !beside_camera)
			{
				continue;
			}
			v = field;
			favour = -0.006;
		}
		else
		{
			v = holds && own.w > 0.5 ? field : own;
			favour = -0.04;
		}
		const vec2 d = v.xy / p1.xy;
		const vec3 l = texture(tex0, uv + t * d).rgb;
		const vec3 n = texture(tex1, (uv - (1.0 - t) * d) * p0.xy).rgb;
		const vec3 g = abs(l - n);
		const float s = max(max(g.r, g.g), g.b) + favour;
		if (s < best)
		{
			best = s;
			mv = v;
			last = l;
			next = n;
		}
	}
	const vec3 here = texture(tex1, uv * p0.xy).rgb;
	const vec3 there = texture(tex0, uv).rgb;
	const vec3 moved_gap = abs(last - next);
	const vec3 still_gap = abs(there - here);
	const float trust = 1.0 - smoothstep(0.035, 0.11, mv.z);
	const float agree = 1.0 - smoothstep(0.03, 0.1, max(max(moved_gap.r, moved_gap.g), moved_gap.b));
	const float still = beside_still ? 1.0 - smoothstep(0.012, 0.04, max(max(still_gap.r, still_gap.g), still_gap.b)) : 0.0;
	const vec3 nearer = t < 0.5 ? there : here;
	// The end that sees the point, for where the two disagree: whether the
	// block at its place in this frame moves with the vector; else whether
	// the block at its place in the last frame does, here (else a thing
	// standing still covers it in both, and neither end shows it)
	const vec2 d = mv.xy / p1.xy;
	const float tolerance = 1.5 + 0.1 * length(mv.xy);
	const vec4 at = texelFetch(tex2, clamp(ivec2(floor((uv - (1.0 - t) * d) * p1.zw * vec2(grid))), ivec2(0), grid - 1), 0);
	const vec4 at_last = texelFetch(tex2, clamp(ivec2(floor((uv + t * d) * p1.zw * vec2(grid))), ivec2(0), grid - 1), 0);
	const bool sees = length(at.xy - mv.xy) <= tolerance || (holds && mv.w > 1.5 && at.w > 0.5);
	const bool last_sees = length(at_last.xy - mv.xy) <= tolerance || (holds && mv.w > 1.5 && at_last.w > 0.5);
	const vec3 moved = mix(sees ? next : last_sees ? last : nearer, mix(last, next, t), agree);
	const vec3 between = keeps_own ? mix(nearer, moved, trust) : mix(nearer, mix(last, next, t), trust * agree);
	const bool quick = camera.w > 0.5;
	imageStore(dst, p, vec4(quick ? nearer : mix(between, nearer, still), 1.0));
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
			// pass_motion: tex0 the rough vectors (pass_coarse_search at a
			// sixteenth of the working size, 64 working texels a block); p0.x how
			// far the camera may move between two game frames, in sixteenth
			// texels, for frames between. One group of 64. The camera's motion
			// is the rough vector with the most support: the clearness (w) of
			// every block whose vector is within a texel of it; none when no
			// block moved clearly. At a sixteenth, a building's rows of windows
			// average away, where the quarter size matched them a row or a
			// column off in a camera turn. Beyond p0.x (6% of the frame's width)
			// the frames between show the real frames as they are: GTA IV's
			// quick turns broke apart on my console, and in the model of these
			// passes (a city turned through 3, 6 and 12 degrees a game frame:
			// rows of windows, poles, a road, the player, the radar) the vectors
			// were right for 62%, 28% and 20% of the blocks that could be
			// matched; a search led by the camera's motion got 84%, 80% and 73%,
			// yet from 6 degrees on its walls, poles and road markings still
			// broke up in places, and it garbled the turning radar. 6% of the
			// width is about 5 degrees a game frame at 70 degrees across. w = 1
			// when it is beyond; z the support
			R"(
shared vec4 s_rough[256];
shared float s_support[64];
shared vec2 s_best[64];

void main()
{
	const int id = int(gl_LocalInvocationIndex);
	const ivec2 grid = textureSize(tex0, 0);
	const int n = min(grid.x * grid.y, 256);
	for (int k = id; k < n; k += 64)
	{
		s_rough[k] = texelFetch(tex0, ivec2(k % grid.x, k / grid.x), 0);
	}
	barrier();
	float best_support = 0.0;
	vec2 best = vec2(0.0);
	for (int k = id; k < n; k += 64)
	{
		const vec2 v = s_rough[k].xy;
		float support = 0.0;
		for (int m = 0; m < n; m++)
		{
			const vec4 o = s_rough[m];
			if (max(abs(o.x - v.x), abs(o.y - v.y)) <= 1.0)
			{
				support += max(o.w, 0.0);
			}
		}
		if (support > best_support)
		{
			best_support = support;
			best = v;
		}
	}
	s_support[id] = best_support;
	s_best[id] = best;
	barrier();
	if (id == 0)
	{
		for (int k = 1; k < 64; k++)
		{
			if (s_support[k] > best_support)
			{
				best_support = s_support[k];
				best = s_best[k];
			}
		}
		const vec2 motion = best_support > 0.02 ? best : vec2(0.0);
		imageStore(dst, ivec2(0), vec4(motion, best_support, length(motion) >= p0.x ? 1.0 : 0.0));
	}
}
)",
			// pass_camera: tex0 the fine vectors (w how clearly each block moved),
			// tex1 the camera's rough motion (pass_motion); p0.xy the working
			// size. One group of 64. The camera's motion as one smooth field,
			// the 8-parameter quadratic flow of a camera turning (or of a plane):
			//   u = a0 + a1 x + a2 y + a6 x^2 + a7 x y
			//   v = a3 + a4 x + a5 y + a6 x y + a7 y^2
			// in working texels, x and y a block's place from the frame's centre
			// in half widths. Fitted to the blocks' vectors weighted by how
			// clearly each moved, from the rough motion's translation, five times
			// over with a reach that narrows from 24 texels to 2 (a block further
			// off weighs nothing, nearer ones less as they near it), which drops
			// the blocks matched a window off and the things moving their own
			// way. Then measured: what share of the clearly moving blocks it puts
			// within 2 texels. Out: texel 0 (a0, a1, a2, a3), texel 1 (a4, a5,
			// a6, a7), texel 2 (whether it holds: a share of 30% or more; that
			// share; the clearness it was fitted to; and pass_motion's verdict on
			// the camera's speed, carried for pass_between). In the model of these
			// passes (a city turned 1.5, 3 and 4.5 degrees a game frame) it held
			// for 38-73%, the vectors right for as few as 38% at Quality
			R"(
shared float s_sums[64 * 45];
shared float s_total[45];
shared float s_model[8];
shared int s_failed;

void main()
{
	const int id = int(gl_LocalInvocationIndex);
	const ivec2 grid = textureSize(tex0, 0);
	const int n = grid.x * grid.y;
	const vec2 work = p0.xy;
	const float half_w = work.x * 0.5;
	const vec4 rough = texelFetch(tex1, ivec2(0), 0);
	if (id < 8)
	{
		s_model[id] = id == 0 ? rough.x * 16.0 : id == 3 ? rough.y * 16.0 : 0.0;
	}
	if (id == 0)
	{
		s_failed = 0;
	}
	barrier();
	const float reaches[5] = float[](24.0, 12.0, 6.0, 3.0, 2.0);
	// Five fits, then a measure
	for (int round_ = 0; round_ < 6; round_++)
	{
		float sums[45];
		for (int i = 0; i < 45; i++)
		{
			sums[i] = 0.0;
		}
		float m[8];
		for (int i = 0; i < 8; i++)
		{
			m[i] = s_model[i];
		}
		for (int k = id; k < n; k += 64)
		{
			const ivec2 b = ivec2(k % grid.x, k / grid.x);
			const vec2 c = (vec2(b) + 0.5) * 8.0;
			if (c.x > work.x || c.y > work.y)
			{
				continue;
			}
			const vec4 v = texelFetch(tex0, b, 0);
			const float clear = max(v.w, 0.0);
			if (clear <= 0.0)
			{
				continue;
			}
			const float x = (c.x - half_w) / half_w;
			const float y = (c.y - work.y * 0.5) / half_w;
			const float pu[8] = float[](1.0, x, y, 0.0, 0.0, 0.0, x * x, x * y);
			const float pv[8] = float[](0.0, 0.0, 0.0, 1.0, x, y, x * y, y * y);
			float mu = 0.0;
			float mv = 0.0;
			for (int i = 0; i < 8; i++)
			{
				mu += pu[i] * m[i];
				mv += pv[i] * m[i];
			}
			const float r = length(v.xy - vec2(mu, mv));
			if (round_ == 5)
			{
				sums[0] += clear;
				sums[1] += r < 2.0 ? clear : 0.0;
				continue;
			}
			const float reach = reaches[round_];
			if (r >= reach)
			{
				continue;
			}
			const float q = 1.0 - (r / reach) * (r / reach);
			const float w = clear * q * q;
			int at = 0;
			for (int i = 0; i < 8; i++)
			{
				for (int j = i; j < 8; j++)
				{
					sums[at] += w * (pu[i] * pu[j] + pv[i] * pv[j]);
					at++;
				}
			}
			for (int i = 0; i < 8; i++)
			{
				sums[36 + i] += w * (pu[i] * v.x + pv[i] * v.y);
			}
			sums[44] += w;
		}
		for (int i = 0; i < 45; i++)
		{
			s_sums[id * 45 + i] = sums[i];
		}
		barrier();
		if (id < 45)
		{
			float total = 0.0;
			for (int k = 0; k < 64; k++)
			{
				total += s_sums[k * 45 + id];
			}
			s_total[id] = total;
		}
		barrier();
		if (id == 0 && round_ < 5)
		{
			if (s_total[44] <= 1e-6)
			{
				s_failed = 1;
			}
			else
			{
				// The normal equations, a little ridge so that they always solve,
				// by elimination with partial pivoting
				float a[8][9];
				int at = 0;
				for (int i = 0; i < 8; i++)
				{
					for (int j = i; j < 8; j++)
					{
						a[i][j] = s_total[at];
						a[j][i] = s_total[at];
						at++;
					}
					a[i][8] = s_total[36 + i];
				}
				float trace = 0.0;
				for (int i = 0; i < 8; i++)
				{
					trace += a[i][i];
				}
				for (int i = 0; i < 8; i++)
				{
					a[i][i] += 1e-4 * trace / 8.0 + 1e-9;
				}
				for (int col = 0; col < 8; col++)
				{
					int pivot = col;
					for (int r = col + 1; r < 8; r++)
					{
						if (abs(a[r][col]) > abs(a[pivot][col]))
						{
							pivot = r;
						}
					}
					for (int j = 0; j < 9; j++)
					{
						const float swap = a[col][j];
						a[col][j] = a[pivot][j];
						a[pivot][j] = swap;
					}
					for (int r = col + 1; r < 8; r++)
					{
						const float f = a[r][col] / a[col][col];
						for (int j = col; j < 9; j++)
						{
							a[r][j] -= f * a[col][j];
						}
					}
				}
				for (int i = 7; i >= 0; i--)
				{
					float v = a[i][8];
					for (int j = i + 1; j < 8; j++)
					{
						v -= a[i][j] * s_model[j];
					}
					s_model[i] = v / a[i][i];
				}
			}
		}
		barrier();
	}
	if (id == 0)
	{
		const float share = s_total[0] > 0.0 ? s_total[1] / s_total[0] : 0.0;
		const bool holds = s_failed == 0 && share >= 0.3 && s_total[0] >= 0.5;
		imageStore(dst, ivec2(0, 0), vec4(s_model[0], s_model[1], s_model[2], s_model[3]));
		imageStore(dst, ivec2(1, 0), vec4(s_model[4], s_model[5], s_model[6], s_model[7]));
		imageStore(dst, ivec2(2, 0), vec4(holds ? 1.0 : 0.0, share, s_total[0], rough.w));
	}
}
)",
			// pass_camera_check: tex0 the last frame's working brightness, tex1
			// this one's, tex2 the camera's field (pass_camera); p0 = (the
			// working size, its 8x8 block grid). Each block: whether it matched
			// clearly better moved with the field than unmoved (by 0.01 a texel)
			// and matched well so (under 0.08): the walls that turned with the
			// camera do; the HUD, the radar's turning map, the player and plain
			// areas do not. In the model of these passes the field so placed kept
			// all its gain on the walls and lamp posts, and the radar as it was
			// (a field used everywhere took 1 to 6.5 dB off it)
			R"(
void main()
{
	const ivec2 p = ivec2(gl_GlobalInvocationID.xy);
	if (p.x >= int(p0.z) || p.y >= int(p0.w)) return;
	const vec4 holds = texelFetch(tex2, ivec2(2, 0), 0);
	float moved_with = 0.0;
	if (holds.x > 0.5)
	{
		const vec4 m0 = texelFetch(tex2, ivec2(0, 0), 0);
		const vec4 m1 = texelFetch(tex2, ivec2(1, 0), 0);
		const float half_w = p0.x * 0.5;
		const vec2 c = (vec2(p) + 0.5) * 8.0;
		const float x = (c.x - half_w) / half_w;
		const float y = (c.y - p0.y * 0.5) / half_w;
		const ivec2 d = ivec2(round(vec2(m0.x + m0.y * x + m0.z * y + m1.z * x * x + m1.w * x * y,
			m0.w + m1.x * x + m1.y * y + m1.z * x * y + m1.w * y * y)));
		const ivec2 lim = ivec2(p0.xy) - 1;
		const ivec2 base = p * 8;
		float moved = 0.0;
		float still = 0.0;
		for (int k = 0; k < 64; k++)
		{
			const ivec2 q = base + ivec2(k & 7, k >> 3);
			const float cur = texelFetch(tex1, min(q, lim), 0).r;
			moved += abs(cur - texelFetch(tex0, clamp(q + d, ivec2(0), lim), 0).r);
			still += abs(cur - texelFetch(tex0, min(q, lim), 0).r);
		}
		moved /= 64.0;
		still /= 64.0;
		moved_with = moved + 0.01 < still && moved < 0.08 ? 1.0 : 0.0;
	}
	imageStore(dst, p, vec4(moved_with));
}
)",
		};

		template <int Kind>
		struct frame_generation_pass : compute_task
		{
			const vk::image_view* m_inputs[4]{};
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
				for (u32 i = 0; i < 4; i++)
				{
					result.push_back(glsl::program_input::make(::glsl::program_domain::glsl_compute_program, "tex" + std::to_string(i), vk::glsl::input_type_texture, 0, i));
				}
				result.push_back(glsl::program_input::make(::glsl::program_domain::glsl_compute_program, "dst", vk::glsl::input_type_storage_texture, 0, 4));
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

				for (u32 i = 0; i < 4; i++)
				{
					m_program->bind_uniform({ *m_inputs[i], *m_sampler }, 0, i);
				}
				m_program->bind_uniform({ *m_output }, 0, 4);
			}

			void run(const vk::command_buffer& cmd, std::initializer_list<vk::viewable_image*> inputs, vk::viewable_image* output, u32 width, u32 height, const std::array<f32, 12>& params)
			{
				const auto view = [](vk::viewable_image* image) { return image->get_view(rsx::default_remap_vector.with_encoding(VK_REMAP_IDENTITY)); };
				u32 i = 0;
				for (vk::viewable_image* input : inputs)
				{
					m_inputs[i++] = view(input);
				}
				for (; i < 4; i++)
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

		// A capture's header text, white on its dark strip, in the title's font
		// (ASCII only); nothing where the font is missing
		void draw_text(std::vector<u8>& rgb, u32 w, u32 h, s32 x, s32 baseline, std::string_view text, f32 pixels, const std::vector<u8>& font_data)
		{
			stbtt_fontinfo font;
			if (font_data.empty() || !stbtt_InitFont(&font, font_data.data(), stbtt_GetFontOffsetForIndex(font_data.data(), 0)))
			{
				return;
			}
			const f32 scale = stbtt_ScaleForPixelHeight(&font, pixels);
			f32 pen = static_cast<f32>(x);
			for (usz i = 0; i < text.size(); i++)
			{
				const int c = static_cast<u8>(text[i]);
				int x0, y0, x1, y1;
				stbtt_GetCodepointBitmapBox(&font, c, scale, scale, &x0, &y0, &x1, &y1);
				const int gw = x1 - x0, gh = y1 - y0;
				if (gw > 0 && gh > 0)
				{
					std::vector<u8> mask(static_cast<usz>(gw) * gh);
					stbtt_MakeCodepointBitmap(&font, mask.data(), gw, gh, gw, scale, scale, c);
					for (int j = 0; j < gh; j++)
					{
						for (int i2 = 0; i2 < gw; i2++)
						{
							const s32 px = static_cast<s32>(pen) + x0 + i2, py = baseline + y0 + j;
							if (px < 0 || py < 0 || px >= static_cast<s32>(w) || py >= static_cast<s32>(h))
							{
								continue;
							}
							u8* out = &rgb[(static_cast<usz>(py) * w + px) * 3];
							const u32 a = mask[static_cast<usz>(j) * gw + i2];
							for (int k = 0; k < 3; k++)
							{
								out[k] = static_cast<u8>((255 * a + out[k] * (255 - a)) / 255);
							}
						}
					}
				}
				int advance, bearing;
				stbtt_GetCodepointHMetrics(&font, c, &advance, &bearing);
				pen += advance * scale;
				if (i + 1 < text.size())
				{
					pen += stbtt_GetCodepointKernAdvance(&font, c, static_cast<u8>(text[i + 1])) * scale;
				}
			}
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
		for (auto& image : m_luma_rough) drop(image);
		drop(m_vectors_rough);
		drop(m_motion);
		drop(m_camera);
		drop(m_camera_moved);
		if (m_motion_read)
		{
			m_motion_read->unmap();
			m_motion_mapped = nullptr;
			vk::get_resource_manager()->dispose(m_motion_read);
		}
		drop(m_vectors_coarse);
		drop(m_vectors);
		drop(m_vectors_smooth);
		drop(m_capture_cur);
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
		const u32 rough_w = utils::aligned_div(coarse_w, 4u);
		const u32 rough_h = utils::aligned_div(coarse_h, 4u);

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
			m_luma_rough[i] = make_image(rough_w, rough_h, VK_FORMAT_R32_SFLOAT);
		}
		m_vectors_rough = make_image(utils::aligned_div(rough_w, 4u), utils::aligned_div(rough_h, 4u), VK_FORMAT_R16G16B16A16_SFLOAT);
		m_motion = make_image(1, 1, VK_FORMAT_R32G32B32A32_SFLOAT);
		m_camera = make_image(3, 1, VK_FORMAT_R32G32B32A32_SFLOAT);
		m_camera_moved = make_image(utils::aligned_div(m_work_w, 8u), utils::aligned_div(m_work_h, 8u), VK_FORMAT_R32_SFLOAT);
		const auto pdev = vk::get_current_renderer();
		m_motion_read = std::make_unique<vk::buffer>(*pdev, 32, pdev->get_memory_mapping().host_visible_coherent,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0, VMM_ALLOCATION_POOL_UNDEFINED);
		if (m_motion_read->value != VK_NULL_HANDLE)
		{
			m_motion_mapped = static_cast<const f32*>(m_motion_read->map(0, 32));
			std::memset(const_cast<f32*>(m_motion_mapped), 0, 32);
		}
		m_vectors_coarse = make_image(utils::aligned_div(coarse_w, 4u), utils::aligned_div(coarse_h, 4u), VK_FORMAT_R16G16B16A16_SFLOAT);
		m_vectors = make_image(utils::aligned_div(m_work_w, 8u), utils::aligned_div(m_work_h, 8u), VK_FORMAT_R16G16B16A16_SFLOAT);
		m_vectors_smooth = make_image(utils::aligned_div(m_work_w, 8u), utils::aligned_div(m_work_h, 8u), VK_FORMAT_R16G16B16A16_SFLOAT);

		if (!m_prev_color || !outputs || !m_luma[0] || !m_luma[1] || !m_luma_coarse[0] || !m_luma_coarse[1] || !m_luma_rough[0] || !m_luma_rough[1] ||
			!m_vectors_rough || !m_motion || !m_camera || !m_camera_moved || !m_motion_mapped || !m_vectors_coarse || !m_vectors || !m_vectors_smooth)
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

	void frame_generator::record_capture(const vk::command_buffer& cmd, vk::viewable_image* cur, u32 width, u32 height, u32 count, u32 quality, u32 step, u32 method)
	{
		const auto pdev = vk::get_current_renderer();
		const u32 grid_w = utils::aligned_div(m_work_w, 8u);
		const u32 grid_h = utils::aligned_div(m_work_h, 8u);
		const usz frame_bytes = usz{width} * height * 4;
		const usz vectors_bytes = usz{grid_w} * grid_h * 8;
		const usz total = frame_bytes * (2 + count) + vectors_bytes + 64;

		if (m_capture_cur && (m_capture_cur->width() != width || m_capture_cur->height() != height))
		{
			vk::get_resource_manager()->dispose(m_capture_cur);
		}
		if (!m_capture_cur)
		{
			m_capture_cur = make_image(width, height, VK_FORMAT_R8G8B8A8_UNORM);
			if (!m_capture_cur)
			{
				rsx_log.warning("Frame generation: no memory for a capture at %ux%u", width, height);
				m_capture_sets = 0;
				return;
			}
			m_capture_cur->change_layout(cmd, VK_IMAGE_LAYOUT_GENERAL);
		}
		if (m_capture_buffer && m_capture_buffer->size() < total)
		{
			vk::get_resource_manager()->dispose(m_capture_buffer);
		}
		if (!m_capture_buffer)
		{
			m_capture_buffer = std::make_unique<vk::buffer>(*pdev, frame_bytes * (2 + max_frames) + vectors_bytes + 64, pdev->get_memory_mapping().host_visible_coherent,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0, VMM_ALLOCATION_POOL_UNDEFINED);
			if (m_capture_buffer->value == VK_NULL_HANDLE)
			{
				rsx_log.warning("Frame generation: no memory for a capture's copy");
				m_capture_buffer.reset();
				m_capture_sets = 0;
				return;
			}
		}

		// This frame as RGBA8 (the game's own buffer may be in another format),
		// then everything copied: the last frame, this one, the frames between,
		// the vectors, the camera's field and its motion
		barrier(cmd);
		vk::get_compute_task<frame_generation_pass<pass_keep>>()->run(cmd, { cur }, m_capture_cur.get(), width, height,
			{ static_cast<f32>(width) / cur->width(), static_cast<f32>(height) / cur->height(), static_cast<f32>(width), static_cast<f32>(height) });
		barrier(cmd);
		usz at = 0;
		const auto copy = [&](vk::viewable_image* image, u32 w, u32 h, usz bytes)
		{
			const VkBufferImageCopy region = { at, 0, 0, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 }, { 0, 0, 0 }, { w, h, 1 } };
			vkCmdCopyImageToBuffer(cmd, image->value, VK_IMAGE_LAYOUT_GENERAL, m_capture_buffer->value, 1, &region);
			at += bytes;
		};
		copy(m_prev_color.get(), width, height, frame_bytes);
		copy(m_capture_cur.get(), width, height, frame_bytes);
		for (u32 k = 0; k < count; k++)
		{
			copy(m_output[k].get(), width, height, frame_bytes);
		}
		copy(m_vectors_smooth.get(), grid_w, grid_h, vectors_bytes);
		copy(m_camera.get(), 3, 1, 48);
		copy(m_motion.get(), 1, 1, 16);

		const VkBufferMemoryBarrier2KHR copied =
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR,
			.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR,
			.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR,
			.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR,
			.dstAccessMask = 0,
			.buffer = m_capture_buffer->value,
			.offset = 0,
			.size = total
		};
		m_capture_event = std::make_unique<vk::event>(*pdev, sync_domain::host);
		m_capture_event->signal(cmd, { .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .bufferMemoryBarrierCount = 1, .pBufferMemoryBarriers = &copied });

		m_capture_w = width;
		m_capture_h = height;
		m_capture_count = count;
		m_capture_grid_w = grid_w;
		m_capture_grid_h = grid_h;
		std::string order = "Top to bottom: the game's frame before";
		for (u32 k = 0; k < count; k++)
		{
			order += fmt::format(", made at %.2f of the way", static_cast<f32>(k + 1) / static_cast<f32>(count + 1));
		}
		order += ", the game's frame after";
		m_capture_info = fmt::format("%s | %ux%u, %s (motion at %ux%u) | method: %s | steps aside: %s\n%s",
			Emu.GetTitleID(), width, height, c_quality[quality].name, m_work_w, m_work_h,
			method == 2 ? "as build 117" : method == 1 ? "117, searches as 115" : "as build 115",
			step == 0 ? "never" : step == 2 ? "in most moves" : "in quick moves", order);
	}

	void frame_generator::take_capture()
	{
		if (const u32 asked = g_ps5_frame_gen_capture.exchange(0))
		{
			m_capture_sets = asked;
			m_capture_set = 1;
			m_capture_run = static_cast<u32>(std::time(nullptr));
			m_capture_next_us = get_system_time() + 2'000'000;
			rsx_log.notice("Frame generation: capturing %u sets of frames, 2 seconds from now", asked);
		}
		if (!m_capture_event || m_capture_event->status() != VK_EVENT_SET)
		{
			return;
		}

		const usz frame_bytes = usz{m_capture_w} * m_capture_h * 4;
		const usz vectors_bytes = usz{m_capture_grid_w} * m_capture_grid_h * 8;
		const usz total = frame_bytes * (2 + m_capture_count) + vectors_bytes + 64;
		std::vector<u8> data(total);
		std::memcpy(data.data(), m_capture_buffer->map(0, total), total);
		m_capture_buffer->unmap();
		vk::get_resource_manager()->dispose(m_capture_event);

		const std::string path = fmt::format("%scaptures/%s-%u-%u.png", fs::get_config_dir(), Emu.GetTitleID(), m_capture_run, m_capture_set);
		const std::string header = fmt::format("Moment %u of %u | %s", m_capture_set, m_capture_set + m_capture_sets - 1, m_capture_info);
		rsx_log.notice("Frame generation: capture %u written to %s", m_capture_set, path);
		m_capture_sets--;
		m_capture_set++;
		m_capture_next_us = get_system_time() + 500'000;

		// One picture for the moment, so a report is a few pictures: a strip
		// saying what it is, then the frames in time order, the game's before,
		// those made between, the game's after. On a thread of its own: PNG
		// takes a while
		std::thread([path, header, data = std::move(data), w = m_capture_w, h = m_capture_h, count = m_capture_count, frame_bytes]()
		{
			constexpr u32 strip = 56;
			const u32 frames = 2 + count;
			const u32 total_h = strip + frames * h;
			std::vector<u8> rgb(usz{w} * total_h * 3, 0);
			for (usz i = 0; i < usz{w} * strip; i++)
			{
				rgb[i * 3 + 0] = 16;
				rgb[i * 3 + 1] = 18;
				rgb[i * 3 + 2] = 30;
			}
			// In the copy: the last frame, this one, then those made between
			const auto place = [&](u32 slot, const u8* rgba)
			{
				u8* out = &rgb[(usz{strip} + usz{slot} * h) * w * 3];
				for (usz i = 0; i < usz{w} * h; i++)
				{
					out[i * 3 + 0] = rgba[i * 4 + 0];
					out[i * 3 + 1] = rgba[i * 4 + 1];
					out[i * 3 + 2] = rgba[i * 4 + 2];
				}
			};
			place(0, data.data());
			place(frames - 1, data.data() + frame_bytes);
			for (u32 k = 0; k < count; k++)
			{
				place(1 + k, data.data() + frame_bytes * (2 + k));
			}

			std::vector<u8> font;
			if (fs::file file{"/app0/assets/fonts/Inter-SemiBold.ttf"})
			{
				file.read(font, file.size());
			}
			const usz split = header.find('\n');
			draw_text(rgb, w, total_h, 14, 23, header.substr(0, split), 17.f, font);
			draw_text(rgb, w, total_h, 14, 47, split == umax ? std::string() : header.substr(split + 1), 17.f, font);

			std::vector<u8> out;
			stbi_write_png_to_func([](void* context, void* bytes, int size)
			{
				auto& sink = *static_cast<std::vector<u8>*>(context);
				sink.insert(sink.end(), static_cast<u8*>(bytes), static_cast<u8*>(bytes) + size);
			}, &out, static_cast<int>(w), static_cast<int>(total_h), 3, rgb.data(), static_cast<int>(w * 3));

			const std::string captures = fs::get_config_dir() + "captures";
			fs::create_path(captures);
			::chmod(captures.c_str(), 0777);
			fs::write_file(path, fs::rewrite, out.data(), out.size());
			::chmod(path.c_str(), 0666);
		}).detach();
	}

	u32 frame_generator::process(const vk::command_buffer& cmd, vk::viewable_image* cur, u32 width, u32 height, u32 count, u32 quality, u32 step,
		u32 method, vk::viewable_image* (&out)[max_frames])
	{
		take_capture();

		if (!cur || !width || !height)
		{
			return 0;
		}
		width = std::min<u32>(width, cur->width());
		height = std::min<u32>(height, cur->height());
		quality = std::min<u32>(quality, static_cast<u32>(std::size(c_quality)) - 1);
		count = std::min(count, max_frames);
		// The Method setting: 0 as build 115; 1 a block's own vector kept and
		// favoured, and the ends that see; 2 also the searches' margins by contrast
		const f32 keeps_own = method >= 1 ? 1.f : 0.f;
		const f32 by_contrast = method >= 2 ? 1.f : 0.f;

		if (width != m_width || height != m_height || quality != m_quality)
		{
			allocate(width, height, quality);
			if (!m_width)
			{
				return 0;
			}
			for (auto* image : { m_prev_color.get(), m_output[0].get(), m_output[1].get(), m_output[2].get(), m_luma[0].get(), m_luma[1].get(),
				m_luma_coarse[0].get(), m_luma_coarse[1].get(), m_luma_rough[0].get(), m_luma_rough[1].get(), m_vectors_rough.get(), m_motion.get(),
				m_camera.get(), m_camera_moved.get(),
				m_vectors_coarse.get(), m_vectors.get(), m_vectors_smooth.get() })
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
		const u32 rough_w = utils::aligned_div(coarse_w, 4u);
		const u32 rough_h = utils::aligned_div(coarse_h, 4u);
		const u32 rough_grid_w = utils::aligned_div(rough_w, 4u);
		const u32 rough_grid_h = utils::aligned_div(rough_h, 4u);
		const u32 grid_w = utils::aligned_div(m_work_w, 8u);
		const u32 grid_h = utils::aligned_div(m_work_h, 8u);

		// What the last frame's presentation read of these images is done
		barrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

		cur->push_layout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		// The camera's motion the last time frames were made, as the trace counts it
		if (make && m_motion_mapped[3] > 0.5f)
		{
			g_ps5_frames_quick_moves++;
		}
		if (make && m_motion_mapped[4] > 0.5f)
		{
			g_ps5_frames_camera_field++;
		}

		// This frame's brightness, at the working size, a quarter and a sixteenth of it
		vk::get_compute_task<frame_generation_pass<pass_brightness>>()->run(cmd, { cur }, m_luma[1].get(), m_work_w, m_work_h,
			{ uv_x, uv_y, work_w, work_h });
		barrier(cmd);
		vk::get_compute_task<frame_generation_pass<pass_quarter>>()->run(cmd, { m_luma[1].get() }, m_luma_coarse[1].get(), coarse_w, coarse_h,
			{ work_w, work_h, static_cast<f32>(coarse_w), static_cast<f32>(coarse_h) });
		barrier(cmd);
		vk::get_compute_task<frame_generation_pass<pass_quarter>>()->run(cmd, { m_luma_coarse[1].get() }, m_luma_rough[1].get(), rough_w, rough_h,
			{ static_cast<f32>(coarse_w), static_cast<f32>(coarse_h), static_cast<f32>(rough_w), static_cast<f32>(rough_h) });
		barrier(cmd);

		u32 made = 0;
		if (make)
		{
			vk::get_compute_task<frame_generation_pass<pass_coarse_search>>()->run(cmd, { m_luma_coarse[0].get(), m_luma_coarse[1].get() }, m_vectors_coarse.get(), coarse_grid_w, coarse_grid_h,
				{ static_cast<f32>(coarse_w), static_cast<f32>(coarse_h), static_cast<f32>(coarse_grid_w), static_cast<f32>(coarse_grid_h),
				  static_cast<f32>(c_quality[quality].reach), 0.0005f, by_contrast });
			barrier(cmd);
			vk::get_compute_task<frame_generation_pass<pass_fine_search>>()->run(cmd, { m_luma[0].get(), m_luma[1].get(), m_vectors_coarse.get() }, m_vectors.get(), grid_w, grid_h,
				{ work_w, work_h, static_cast<f32>(grid_w), static_cast<f32>(grid_h), static_cast<f32>(coarse_grid_w), static_cast<f32>(coarse_grid_h), by_contrast });
			barrier(cmd);
			// The camera's motion: the rough vectors (a block of 64 working texels,
			// up to a quarter of the reach in sixteenth texels), voted
			vk::get_compute_task<frame_generation_pass<pass_coarse_search>>()->run(cmd, { m_luma_rough[0].get(), m_luma_rough[1].get() }, m_vectors_rough.get(), rough_grid_w, rough_grid_h,
				{ static_cast<f32>(rough_w), static_cast<f32>(rough_h), static_cast<f32>(rough_grid_w), static_cast<f32>(rough_grid_h),
				  static_cast<f32>(std::max(1u, c_quality[quality].reach / 4)), 0.002f, by_contrast });
			barrier(cmd);
			// Stepping aside beyond 6% of the frame's width a game frame (Quick
			// moves), 3% (Most moves), or never
			const f32 step_aside = step == 0 ? 1e9f : step == 2 ? 0.03f : 0.06f;
			vk::get_compute_task<frame_generation_pass<pass_motion>>()->run(cmd, { m_vectors_rough.get() }, m_motion.get(), 1, 1,
				{ step_aside * work_w / 16.f });
			barrier(cmd);
			// The camera's field, and each block checked against it
			vk::get_compute_task<frame_generation_pass<pass_camera>>()->run(cmd, { m_vectors.get(), m_motion.get() }, m_camera.get(), 1, 1,
				{ work_w, work_h });
			barrier(cmd);
			vk::get_compute_task<frame_generation_pass<pass_camera_check>>()->run(cmd, { m_luma[0].get(), m_luma[1].get(), m_camera.get() }, m_camera_moved.get(), grid_w, grid_h,
				{ work_w, work_h, static_cast<f32>(grid_w), static_cast<f32>(grid_h) });
			barrier(cmd);
			// For the trace: the motion's verdict, and whether the field held
			const VkBufferImageCopy regions[2] = {
				{ 0, 0, 0, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 }, { 0, 0, 0 }, { 1, 1, 1 } },
				{ 16, 0, 0, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 }, { 2, 0, 0 }, { 1, 1, 1 } } };
			vkCmdCopyImageToBuffer(cmd, m_motion->value, VK_IMAGE_LAYOUT_GENERAL, m_motion_read->value, 1, &regions[0]);
			vkCmdCopyImageToBuffer(cmd, m_camera->value, VK_IMAGE_LAYOUT_GENERAL, m_motion_read->value, 1, &regions[1]);
			VkMemoryBarrier host = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };
			host.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			host.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &host, 0, nullptr, 0, nullptr);
			vk::get_compute_task<frame_generation_pass<pass_median>>()->run(cmd, { m_vectors.get(), m_camera_moved.get(), m_luma[0].get(), m_luma[1].get() }, m_vectors_smooth.get(), grid_w, grid_h,
				{ static_cast<f32>(grid_w), static_cast<f32>(grid_h), work_w, work_h, keeps_own });
			barrier(cmd);

			// The frames between, each at its time: with three, a quarter, a half
			// and three quarters of the way from the last frame to this one
			for (u32 k = 0; k < count; k++)
			{
				const f32 t = static_cast<f32>(k + 1) / static_cast<f32>(count + 1);
				vk::get_compute_task<frame_generation_pass<pass_between>>()->run(cmd, { m_prev_color.get(), cur, m_vectors_smooth.get(), m_camera.get() }, m_output[k].get(), width, height,
					{ uv_x, uv_y, static_cast<f32>(width), static_cast<f32>(height), work_w, work_h, work_w / (grid_w * 8.f), work_h / (grid_h * 8.f), t, keeps_own });
				out[k] = m_output[k].get();
			}
			made = count;
			g_ps5_frames_generated += count;

			// A capture's set, before the last frame's picture is replaced
			if (m_capture_sets && !m_capture_event && get_system_time() >= m_capture_next_us)
			{
				record_capture(cmd, cur, width, height, count, quality, step, method);
			}
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
		std::swap(m_luma_rough[0], m_luma_rough[1]);
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
