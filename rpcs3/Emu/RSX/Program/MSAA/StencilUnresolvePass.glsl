R"(
#version 420
#extension GL_ARB_separate_shader_objects: enable

#ifdef VULKAN
layout(set=0, binding=0) uniform usampler2D fs0;
layout(push_constant) uniform static_data
{
	layout(offset=0) ivec2 sample_count;
	layout(offset=8) int stencil_mask;
};
#else
layout(binding=31) uniform usampler2D fs0;
uniform ivec2 sample_count;
uniform int stencil_mask;
#endif

void main()
{
	ivec2 pixel_coord = ivec2(gl_FragCoord.xy);
	pixel_coord *= sample_count.xy;
	pixel_coord.x += (gl_SampleID % sample_count.x);
	pixel_coord.y += (gl_SampleID / sample_count.x);
	uint frag_stencil = texelFetch(fs0, pixel_coord, 0).x;
	if ((frag_stencil & uint(stencil_mask)) == 0) discard;
}

)"
