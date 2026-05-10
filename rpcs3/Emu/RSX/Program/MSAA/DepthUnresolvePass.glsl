R"(
#version 420
#extension GL_ARB_separate_shader_objects: enable

#ifdef VULKAN
layout(set=0, binding=0) uniform sampler2D fs0;
layout(push_constant) uniform static_data { ivec2 sample_count; };
#else
layout(binding=31) uniform sampler2D fs0;
uniform ivec2 sample_count;
#endif

void main()
{
	ivec2 pixel_coord = ivec2(gl_FragCoord.xy);
	pixel_coord *= sample_count.xy;
	pixel_coord.x += (gl_SampleID % sample_count.x);
	pixel_coord.y += (gl_SampleID / sample_count.x);
	float frag_depth = texelFetch(fs0, pixel_coord, 0).x;
	gl_FragDepth = frag_depth;
}

)"
