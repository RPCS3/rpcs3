R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

%preprocessor

#ifdef VULKAN
	#define USE_UBO 1
#endif

layout(location=0) in vec4 in_pos;
layout(location=0) out vec2 tc0;
layout(location=1) out vec4 color;
layout(location=2) out vec4 clip_rect;

#if USE_UBO
layout(%push_block) uniform Configuration
{
	vec4 ui_scale;
	vec4 albedo;
	vec4 viewport;
	vec4 clip_bounds;
	uint vertex_config;
};
#else
	uniform vec4 ui_scale;
	uniform vec4 albedo;
	uniform vec4 viewport;
	uniform vec4 clip_bounds;
	uniform uint vertex_config;
#endif

struct config_t
{
	bool no_vertex_snap;
};

config_t unpack_vertex_options()
{
	config_t result;
	result.no_vertex_snap = bitfieldExtract(vertex_config, 0, 1) != 0;
	return result;
}

vec2 snap_to_grid(const in vec2 normalized)
{
	return floor(fma(normalized, viewport.xy, vec2(0.5))) / viewport.xy;
}

vec4 clip_to_ndc(const in vec4 coord)
{
	vec4 ret = (coord * ui_scale.zwzw) / ui_scale.xyxy;
#ifndef VULKAN
	// Flip Y for OpenGL
	ret.yw = 1. - ret.yw;
#endif
	return ret;
}

vec4 ndc_to_window(const in vec4 coord)
{
	return fma(coord, viewport.xyxy, viewport.zwzw);
}

void main()
{
	tc0.xy = in_pos.zw;
	color = albedo;
	clip_rect = ndc_to_window(clip_to_ndc(clip_bounds));

	vec4 pos = vec4(clip_to_ndc(in_pos).xy, 0.5, 1.);
	config_t config = unpack_vertex_options();

	if (!config.no_vertex_snap)
	{
		pos.xy = snap_to_grid(pos.xy);
	}

	gl_Position = (pos + pos) - 1.;
}
)"
