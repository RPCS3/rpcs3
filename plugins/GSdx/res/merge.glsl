//#version 420 // Keep it for editor detection

struct vertex_basic
{
    vec4 p;
    vec2 t;
};

#ifdef FRAGMENT_SHADER
#if __VERSION__ > 140 && !(defined(NO_STRUCT))
layout(location = 0) in vertex_basic PSin;
#define PSin_p (PSin.p)
#define PSin_t (PSin.t)
#else
layout(location = 0) in vec4 p;
layout(location = 1) in vec2 t;
#define PSin_p p
#define PSin_t t
#endif

layout(location = 0) out vec4 SV_Target0;

#ifdef DISABLE_GL42
layout(std140) uniform cb10
#else
layout(std140, binding = 10) uniform cb10
#endif
{
    vec4 BGColor;
};

#ifdef DISABLE_GL42
uniform sampler2D TextureSampler;
#else
layout(binding = 0) uniform sampler2D TextureSampler;
#endif

void ps_main0()
{
    vec4 c = texture(TextureSampler, PSin_t);
	c.a = min(c.a * 2, 1.0);
    SV_Target0 = c;
}

void ps_main1()
{
    vec4 c = texture(TextureSampler, PSin_t);
	c.a = BGColor.a;
    SV_Target0 = c;
}

#endif
