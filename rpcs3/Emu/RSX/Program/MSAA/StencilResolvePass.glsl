R"(
#version 420
#extension GL_ARB_separate_shader_objects: enable

#ifdef VULKAN
layout(set=0, binding=0) uniform usampler2DMS fs0;
layout(push_constant) uniform static_data
{
	layout(offset=0) ivec2 sample_count;
	layout(offset=8) int stencil_mask;
};
#else
layout(binding=31) uniform usampler2DMS fs0;
uniform ivec2 sample_count;
uniform int stencil_mask;
#endif

void main()
{
	ivec2 out_coord = ivec2(gl_FragCoord.xy);
	ivec2 in_coord = (out_coord / sample_count.xy);
	ivec2 sample_loc = out_coord % sample_count.xy;
	int sample_index = sample_loc.x + (sample_loc.y * sample_count.y);
	uint frag_stencil = texelFetch(fs0, in_coord, sample_index).x;
	if ((frag_stencil & uint(stencil_mask)) == 0) discard;
}

)"
