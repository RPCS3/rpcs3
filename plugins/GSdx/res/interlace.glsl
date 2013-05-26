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
layout(std140) uniform cb11
#else
layout(std140, binding = 11) uniform cb11
#endif
{
    vec2 ZrH;
    float hH;
};

#ifdef DISABLE_GL42
uniform sampler2D TextureSampler;
#else
layout(binding = 0) uniform sampler2D TextureSampler;
#endif

// TODO ensure that clip (discard) is < 0 and not <= 0 ???
void ps_main0()
{
    // I'm not sure it impact us but be safe to lookup texture before conditional if
    // see: http://www.opengl.org/wiki/GLSL_Sampler#Non-uniform_flow_control
    vec4 c = texture(TextureSampler, PSin_t);
    if (fract(PSin_t.y * hH) - 0.5 < 0.0)
        discard;

    SV_Target0 = c;
}

void ps_main1()
{
    // I'm not sure it impact us but be safe to lookup texture before conditional if
    // see: http://www.opengl.org/wiki/GLSL_Sampler#Non-uniform_flow_control
    vec4 c = texture(TextureSampler, PSin_t);
    if (0.5 - fract(PSin_t.y * hH) < 0.0)
        discard;

    SV_Target0 = c;
}

void ps_main2()
{
    vec4 c0 = texture(TextureSampler, PSin_t - ZrH);
    vec4 c1 = texture(TextureSampler, PSin_t);
    vec4 c2 = texture(TextureSampler, PSin_t + ZrH);

    SV_Target0 = (c0 + c1 * 2 + c2) / 4;
}

void ps_main3()
{
    SV_Target0 = texture(TextureSampler, PSin_t);
}

#endif
