//#version 420 // Keep it for editor detection

#ifdef FRAGMENT_SHADER
layout(location = 0) in vec4 SV_Position;
layout(location = 1) in vec2 TEXCOORD0;

layout(location = 0) out vec4 SV_Target0;

layout(std140, binding = 1) uniform cb0
{
    vec2 ZrH;
    float hH;
};

layout(binding = 0) uniform sampler2D TextureSampler;

// TODO ensure that clip (discard) is < 0 and not <= 0 ???
void ps_main0()
{
    // I'm not sure it impact us but be safe to lookup texture before conditional if
    // see: http://www.opengl.org/wiki/GLSL_Sampler#Non-uniform_flow_control
    vec4 c = texture(TextureSampler, TEXCOORD0);
    if (fract(TEXCOORD0.y * hH) - 0.5 < 0.0)
        discard;

    SV_Target0 = c;
}

void ps_main1()
{
    // I'm not sure it impact us but be safe to lookup texture before conditional if
    // see: http://www.opengl.org/wiki/GLSL_Sampler#Non-uniform_flow_control
    vec4 c = texture(TextureSampler, TEXCOORD0);
    if (0.5 - fract(TEXCOORD0.y * hH) < 0.0)
        discard;

    SV_Target0 = c;
}

void ps_main2()
{
    vec4 c0 = texture(TextureSampler, TEXCOORD0 - ZrH);
    vec4 c1 = texture(TextureSampler, TEXCOORD0);
    vec4 c2 = texture(TextureSampler, TEXCOORD0 + ZrH);

    SV_Target0 = (c0 + c1 * 2 + c2) / 4;
}

void ps_main3()
{
    SV_Target0 = texture(TextureSampler, TEXCOORD0);
}

#endif
