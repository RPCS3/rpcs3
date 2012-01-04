//#version 420 // Keep it for editor detection

struct vertex_basic
{
    vec4 p;
    vec2 t;
};


#ifdef VERTEX_SHADER

out gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
};

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
layout(location = 0) out vertex_basic VSout;

void vs_main()
{
    VSout.p = POSITION;
    VSout.t = TEXCOORD0;
    gl_Position = POSITION; // NOTE I don't know if it is possible to merge POSITION_OUT and gl_Position
}

#endif

#ifdef GEOMETRY_SHADER
in gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
} gl_in[];

out gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
};

// FIXME
// AMD Driver bug again !!!!
//layout(location = 0) in vertex GSin[];
in vertex_basic GSin[];

layout(location = 0) out vertex_basic GSout;
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

void gs_main()
{
    for(int i = 0; i < gl_in.length(); i++) {
        gl_Position = gl_in[i].gl_Position;
        GSout = GSin[i];
        EmitVertex();
    }
    EndPrimitive();
}
#endif

#ifdef FRAGMENT_SHADER
// NOTE: pixel can be clip with "discard"

layout(location = 0) in vertex_basic PSin;

layout(location = 0) out vec4 SV_Target0;
layout(location = 1) out uint SV_Target1;

layout(binding = 0) uniform sampler2D TextureSampler;

vec4 sample_c()
{
    return texture(TextureSampler, PSin.t );
}

vec4 ps_crt(uint i)
{
    vec4 mask[4] =
	{
		vec4(1, 0, 0, 0),
		vec4(0, 1, 0, 0),
		vec4(0, 0, 1, 0),
		vec4(1, 1, 1, 0)
	};

	return sample_c() * clamp((mask[i] + 0.5f), 0.0f, 1.0f);
}

void ps_main0()
{
    SV_Target0 = sample_c();
}

void ps_main1()
{
    vec4 c = sample_c();

	c.a *= 256.0f / 127.0f; // hm, 0.5 won't give us 1.0 if we just multiply with 2

	highp uvec4 i = uvec4(c * vec4(0x001f, 0x03e0, 0x7c00, 0x8000));

    SV_Target1 = (i.x & uint(0x001f)) | (i.y & uint(0x03e0)) | (i.z & uint(0x7c00)) | (i.w & uint(0x8000));
}

void ps_main7()
{
    vec4 c = sample_c();

	c.a = dot(c.rgb, vec3(0.299, 0.587, 0.114));

    SV_Target0 = c;
}

void ps_main5() // triangular
{
	highp uvec4 p = uvec4(PSin.p);

	vec4 c = ps_crt(((p.x + ((p.y >> 1u) & 1u) * 3u) >> 1u) % 3u);

    SV_Target0 = c;
}

void ps_main6() // diagonal
{
	uvec4 p = uvec4(PSin.p);

	vec4 c = ps_crt((p.x + (p.y % 3u)) % 3u);

    SV_Target0 = c;
}

// Avoid to log useless error compilation failure
void ps_main2()
{
}
void ps_main3()
{
}
void ps_main4()
{
}


// Texture2D Texture;
// SamplerState TextureSampler;
//
// uint ps_main1(PS_INPUT input) : SV_Target0
// {
// 	float4 c = sample_c(input.t);
//
// 	c.a *= 256.0f / 127; // hm, 0.5 won't give us 1.0 if we just multiply with 2
//
// 	uint4 i = c * float4(0x001f, 0x03e0, 0x7c00, 0x8000);
//
// 	return (i.x & 0x001f) | (i.y & 0x03e0) | (i.z & 0x7c00) | (i.w & 0x8000);
// }
//
// PS_OUTPUT ps_main2(PS_INPUT input)
// {
// 	PS_OUTPUT output;
//
// 	clip(sample_c(input.t).a - 128.0f / 255); // >= 0x80 pass
//
// 	output.c = 0;
//
// 	return output;
// }
//
// PS_OUTPUT ps_main3(PS_INPUT input)
// {
// 	PS_OUTPUT output;
//
// 	clip(127.95f / 255 - sample_c(input.t).a); // < 0x80 pass (== 0x80 should not pass)
//
// 	output.c = 0;
//
// 	return output;
// }
//
// PS_OUTPUT ps_main4(PS_INPUT input)
// {
// 	PS_OUTPUT output;
//
// 	output.c = fmod(sample_c(input.t) * 255 + 0.5f, 256) / 255;
//
// 	return output;
// }
//

#endif
