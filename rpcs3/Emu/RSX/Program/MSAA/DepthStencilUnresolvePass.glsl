R"(
#version 420
#extension GL_ARB_separate_shader_objects: enable
#extension GL_ARB_shader_stencil_export : enable

#ifdef VULKAN
layout(set=0, binding=0) uniform sampler2D fs0;
layout(set=0, binding=1) uniform usampler2D fs1;
layout(push_constant) uniform static_data { ivec2 sample_count; };
#else
layout(binding=31) uniform sampler2D fs0;
layout(binding=30) uniform usampler2D fs1;
uniform ivec2 sample_count;
#endif

void main()
{
	ivec2 pixel_coord = ivec2(gl_FragCoord.xy);
	pixel_coord *= sample_count.xy;
	pixel_coord.x += (gl_SampleID % sample_count.x);
	pixel_coord.y += (gl_SampleID / sample_count.x);
	float frag_depth = texelFetch(fs0, pixel_coord, 0).x;
	uint frag_stencil = texelFetch(fs1, pixel_coord, 0).x;
	gl_FragDepth = frag_depth;
	gl_FragStencilRefARB = int(frag_stencil);
}

)"
