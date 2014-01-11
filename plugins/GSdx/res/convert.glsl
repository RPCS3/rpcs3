//#version 420 // Keep it for editor detection

struct vertex_basic
{
    vec4 p;
    vec2 t;
};


#ifdef VERTEX_SHADER

#if !pGL_ES && __VERSION__ > 140
out gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
};
#endif

layout(location = 0) in vec4 POSITION;
layout(location = 1) in vec2 TEXCOORD0;

// FIXME set the interpolation (don't know what dx do)
// flat means that there is no interpolation. The value given to the fragment shader is based on the provoking vertex conventions.
//
// noperspective means that there will be linear interpolation in window-space. This is usually not what you want, but it can have its uses.
//
// smooth, the default, means to do perspective-correct interpolation.
//
// The centroid qualifier only matters when multisampling. If this qualifier is not present, then the value is interpolated to the pixel's center, anywhere in the pixel, or to one of the pixel's samples. This sample may lie outside of the actual primitive being rendered, since a primitive can cover only part of a pixel's area. The centroid qualifier is used to prevent this; the interpolation point must fall within both the pixel's area and the primitive's area.
#if !pGL_ES && __VERSION__ > 140

out SHADER
{
    vec4 p;
    vec2 t;
} VSout;

#define VSout_p (VSout.p)
#define VSout_t (VSout.t)

#else

#ifdef DISABLE_SSO
out vec4 SHADERp;
out vec2 SHADERt;
#else
layout(location = 0) out vec4 SHADERp;
layout(location = 1) out vec2 SHADERt;
#endif
#define VSout_p SHADERp
#define VSout_t SHADERt

#endif

void vs_main()
{
    VSout_p = POSITION;
    VSout_t = TEXCOORD0;
    gl_Position = POSITION; // NOTE I don't know if it is possible to merge POSITION_OUT and gl_Position
}

#endif

#ifdef FRAGMENT_SHADER

#if !pGL_ES && __VERSION__ > 140

in SHADER
{
    vec4 p;
    vec2 t;
} PSin;

#define PSin_p (PSin.p)
#define PSin_t (PSin.t)

#else

#ifdef DISABLE_SSO
in vec4 SHADERp;
in vec2 SHADERt;
#else
layout(location = 0) in vec4 SHADERp;
layout(location = 1) in vec2 SHADERt;
#endif
#define PSin_p SHADERp
#define PSin_t SHADERt

#endif

// Give a different name so I remember there is a special case!
#ifdef ps_main1
layout(location = 0) out uint SV_Target1;
#else
layout(location = 0) out vec4 SV_Target0;
#endif

#ifdef ENABLE_BINDLESS_TEX
layout(bindless_sampler, location = 0) uniform sampler2D TextureSampler;
#else
#ifdef DISABLE_GL42
uniform sampler2D TextureSampler;
#else
layout(binding = 0) uniform sampler2D TextureSampler;
#endif
#endif

vec4 sample_c()
{
    return texture(TextureSampler, PSin_t );
}

//uniform vec4 mask[4] = vec4[4]
//(
//		vec4(1, 0, 0, 0),
//		vec4(0, 1, 0, 0),
//		vec4(0, 0, 1, 0),
//		vec4(1, 1, 1, 0)
//);


vec4 ps_crt(uint i)
{
    vec4 mask[4] = vec4[4]
        (
         vec4(1, 0, 0, 0),
         vec4(0, 1, 0, 0),
         vec4(0, 0, 1, 0),
         vec4(1, 1, 1, 0)
        );
	return sample_c() * clamp((mask[i] + 0.5f), 0.0f, 1.0f);
}

#ifdef ps_main0
void ps_main0()
{
    SV_Target0 = sample_c();
}
#endif

#ifdef ps_main1
void ps_main1()
{
    vec4 c = sample_c();

	c.a *= 256.0f / 127.0f; // hm, 0.5 won't give us 1.0 if we just multiply with 2

	highp uvec4 i = uvec4(c * vec4(uint(0x001f), uint(0x03e0), uint(0x7c00), uint(0x8000)));

    SV_Target1 = (i.x & uint(0x001f)) | (i.y & uint(0x03e0)) | (i.z & uint(0x7c00)) | (i.w & uint(0x8000));
}
#endif

#ifdef ps_main7
void ps_main7()
{
    vec4 c = sample_c();

	c.a = dot(c.rgb, vec3(0.299, 0.587, 0.114));

    SV_Target0 = c;
}
#endif

#ifdef ps_main5
void ps_main5() // triangular
{
	highp uvec4 p = uvec4(PSin_p);

	vec4 c = ps_crt(((p.x + ((p.y >> 1u) & 1u) * 3u) >> 1u) % 3u);

    SV_Target0 = c;
}
#endif

#ifdef ps_main6
void ps_main6() // diagonal
{
	uvec4 p = uvec4(PSin_p);

	vec4 c = ps_crt((p.x + (p.y % 3u)) % 3u);

    SV_Target0 = c;
}
#endif

// Used for DATE (stencil)
// DATM == 1
#ifdef ps_main2
void ps_main2()
{
    if(sample_c().a < 127.5f / 255.0f) // >= 0x80 pass
        discard;

#ifdef ENABLE_OGL_STENCIL_DEBUG
    SV_Target0 = vec4(1.0f, 0.0f, 0.0f, 1.0f);
#endif
}
#endif

// Used for DATE (stencil)
// DATM == 0
#ifdef ps_main3
void ps_main3()
{
    if(127.5f / 255.0f < sample_c().a) // < 0x80 pass (== 0x80 should not pass)
        discard;

#ifdef ENABLE_OGL_STENCIL_DEBUG
    SV_Target0 = vec4(1.0f, 0.0f, 0.0f, 1.0f);
#endif
}
#endif

#ifdef ps_main4
void ps_main4()
{
    // FIXME mod and fmod are different when value are negative
    // 	output.c = fmod(sample_c(input.t) * 255 + 0.5f, 256) / 255;
    vec4 c = mod(sample_c() * 255.0f + 0.5f, 256.0f) / 255.0f;

    SV_Target0 = c;
}
#endif

#endif
