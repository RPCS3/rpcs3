//#version 420 // Keep it for editor detection

#ifdef FRAGMENT_SHADER

layout(location = 0) in vec4 SV_Position;
layout(location = 1) in vec2 TEXCOORD0;

out vec4 SV_Target0;

layout(std140, binding = 1) uniform cb0
{
    vec4 BGColor;
};

layout(binding = 0) uniform sampler2D TextureSampler;

void ps_main0()
{
    vec4 c = texture(TextureSampler, TEXCOORD0);
	c.a = min(c.a * 2, 1);
    SV_Target0 = c;
}

void ps_main1()
{
    vec4 c = texture(TextureSampler, TEXCOORD0);
	c.a = BGColor.a;
    SV_Target0 = c;
}

#endif
