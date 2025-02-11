R"(
#version 420
#extension GL_ARB_separate_shader_objects: enable
#extension GL_ARB_shader_stencil_export : enable

#ifdef VULKAN
layout(set=0, binding=0) uniform sampler2DMS fs0;
layout(set=0, binding=1) uniform usampler2DMS fs1;
layout(push_constant) uniform static_data { ivec2 sample_count; };
#else
layout(binding=31) uniform sampler2DMS fs0;
layout(binding=30) uniform usampler2DMS fs1;
uniform ivec2 sample_count;
#endif

void main()
{
	ivec2 out_coord = ivec2(gl_FragCoord.xy);
	ivec2 in_coord = (out_coord / sample_count.xy);
	ivec2 sample_loc = out_coord % ivec2(sample_count.xy);
	int sample_index = sample_loc.x + (sample_loc.y * sample_count.y);
	float frag_depth = texelFetch(fs0, in_coord, sample_index).x;
	uint frag_stencil = texelFetch(fs1, in_coord, sample_index).x;
	gl_FragDepth = frag_depth;
	gl_FragStencilRefARB = int(frag_stencil);
}

)"
