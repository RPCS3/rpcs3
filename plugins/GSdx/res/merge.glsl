//#version 420 // Keep it for editor detection

struct vertex_basic
{
    vec4 p;
    vec2 t;
};

#ifdef FRAGMENT_SHADER
layout(location = 0) in vertex_basic PSin;

layout(location = 0) out vec4 SV_Target0;

layout(std140, binding = 1) uniform cb0
{
    vec4 BGColor;
};

layout(binding = 0) uniform sampler2D TextureSampler;

void ps_main0()
{
    vec4 c = texture(TextureSampler, PSin.t);
	c.a = min(c.a * 2, 1.0);
    SV_Target0 = c;
}

void ps_main1()
{
    vec4 c = texture(TextureSampler, PSin.t);
	c.a = BGColor.a;
    SV_Target0 = c;
}

#endif
