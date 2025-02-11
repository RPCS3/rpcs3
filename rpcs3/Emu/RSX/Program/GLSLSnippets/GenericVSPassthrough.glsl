R"(
#version 420
#extension GL_ARB_separate_shader_objects: enable

layout(location=0) out vec2 tc0;

#ifdef VULKAN
#define gl_VertexID gl_VertexIndex
#endif

void main()
{
	vec2 positions[] = {vec2(-1., -1.), vec2(1., -1.), vec2(-1., 1.), vec2(1., 1.)};
#ifdef VULKAN
	// Origin at top left
	vec2 coords[] = {vec2(0., 0.), vec2(1., 0.), vec2(0., 1.), vec2(1., 1.)};
#else
	// Origin at bottom left. Flip Y coordinate.
	vec2 coords[] = {vec2(0., 1.), vec2(1., 1.), vec2(0., 0.), vec2(1., 0.)};
#endif

	tc0 = coords[gl_VertexID % 4];
	vec2 pos = positions[gl_VertexID % 4];
	gl_Position = vec4(pos, 0., 1.);
}
)"
