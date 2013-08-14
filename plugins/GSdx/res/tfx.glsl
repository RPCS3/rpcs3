//#version 420 // Keep it for text editor detection

// note lerp => mix

#define FMT_32 0
#define FMT_24 1
#define FMT_16 2
#define FMT_PAL 4 /* flag bit */

// Not sure we have same issue on opengl. Doesn't work anyway on ATI card
// And I say this as an ATI user.
#define ATI_SUCKS 0

#ifndef VS_BPPZ
#define VS_BPPZ 0
#define VS_TME 1
#define VS_FST 1
#define VS_LOGZ 0
#endif

#ifndef PS_FST
#define PS_FST 0
#define PS_WMS 0
#define PS_WMT 0
#define PS_FMT FMT_32
#define PS_AEM 0
#define PS_TFX 0
#define PS_TCC 1
#define PS_ATST 1
#define PS_FOG 0
#define PS_CLR1 0
#define PS_FBA 0
#define PS_AOUT 0
#define PS_LTF 1
#define PS_COLCLIP 0
#define PS_DATE 0
#define PS_SPRITEHACK 0
#define PS_POINT_SAMPLER 0
#define PS_TCOFFSETHACK 0
#define PS_IIP 1
#endif

struct vertex
{
    vec4 t;
    vec4 c;
	vec4 fc;
};

#ifdef VERTEX_SHADER
layout(location = 0) in vec2  i_st;
layout(location = 1) in vec4  i_c;
layout(location = 2) in float i_q;
layout(location = 3) in uvec2 i_p;
layout(location = 4) in uint  i_z;
layout(location = 5) in uvec2 i_uv;
layout(location = 6) in vec4  i_f;

#if !GL_ES && __VERSION__ > 140

out SHADER
{
    vec4 t;
    vec4 c;
	flat vec4 fc;
} VSout;

#define VSout_t (VSout.t)
#define VSout_c (VSout.c)
#define VSout_fc (VSout.fc)

#else

#ifdef DISABLE_SSO
out vec4 SHADERt;
out vec4 SHADERc;
flat out vec4 SHADERfc;
#else
layout(location = 0) out vec4 SHADERt;
layout(location = 1) out vec4 SHADERc;
flat layout(location = 2) out vec4 SHADERfc;
#endif
#define VSout_t SHADERt
#define VSout_c SHADERc
#define VSout_fc SHADERfc

#endif

#if !GL_ES && __VERSION__ > 140
out gl_PerVertex {
    invariant vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
};
#endif

#ifdef DISABLE_GL42
layout(std140) uniform cb20
#else
layout(std140, binding = 20) uniform cb20
#endif
{
    vec2 VertexScale;
    vec2 VertexOffset;
    vec2 TextureScale;
};

const float exp_min32 = exp2(-32.0f);

void vs_main()
{
    uint z;
    if(VS_BPPZ == 1) // 24
        z = i_z & uint(0xffffff);
    else if(VS_BPPZ == 2) // 16
        z = i_z & uint(0xffff);
    else
        z = i_z;

    // pos -= 0.05 (1/320 pixel) helps avoiding rounding problems (integral part of pos is usually 5 digits, 0.05 is about as low as we can go)
    // example: ceil(afterseveralvertextransformations(y = 133)) => 134 => line 133 stays empty
    // input granularity is 1/16 pixel, anything smaller than that won't step drawing up/left by one pixel
    // example: 133.0625 (133 + 1/16) should start from line 134, ceil(133.0625 - 0.05) still above 133

    vec3 p = vec3(i_p, z) - vec3(0.05f, 0.05f, 0.0f);
    p = p * vec3(VertexScale, exp_min32) - vec3(VertexOffset, 0.0f);

    if(VS_LOGZ == 1)
    {
        p.z = log2(1.0f + float(z)) / 32.0f;
    }

    gl_Position = vec4(p, 1.0f); // NOTE I don't know if it is possible to merge POSITION_OUT and gl_Position

    if(VS_TME != 0)
    {
        if(VS_FST != 0)
        {
            VSout_t.xy = vec2(i_uv) * TextureScale;
            VSout_t.w = 1.0f;
        }
        else
        {
            VSout_t.xy = i_st;
            VSout_t.w = i_q;
        }
    }
    else
    {
        VSout_t.xy = vec2(0.0f, 0.0f);
        VSout_t.w = 1.0f;
    }

    VSout_c = i_c;
	VSout_fc = i_c;
    VSout_t.z = i_f.r;
}

#endif

#ifdef GEOMETRY_SHADER
in gl_PerVertex {
    invariant vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
} gl_in[];
//in int gl_PrimitiveIDIn;

out gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
};
//out int gl_PrimitiveID;

in SHADER
{
    vec4 t;
    vec4 c;
    flat vec4 fc;
} GSin[];

out SHADER
{
    vec4 t;
    vec4 c;
    flat vec4 fc;
} GSout;

void out_vertex(in vertex v)
{
    GSout.t = v.t;
    GSout.c = v.c;
    GSout.fc = v.fc;
    gl_PrimitiveID = gl_PrimitiveIDIn;
    EmitVertex();
}

layout(lines) in;
layout(triangle_strip, max_vertices = 6) out;

void gs_main()
{
    // left top     => GSin[0];
    // right bottom => GSin[1];
    vertex rb = vertex(GSin[1].t, GSin[1].c, GSin[1].fc);
    vertex lt = vertex(GSin[0].t, GSin[0].c, GSin[0].fc);

    vec4 rb_p = gl_in[1].gl_Position;
    vec4 lb_p = gl_in[1].gl_Position;
    vec4 rt_p = gl_in[1].gl_Position;
    vec4 lt_p = gl_in[0].gl_Position;

    // flat depth
    lt_p.z = rb_p.z;
    // flat fog and texture perspective
    lt.t.zw = rb.t.zw;
    // flat color
    lt.c = rb.c;

	// Swap texture and position coordinate
    vertex lb = rb;
    lb_p.x = lt_p.x;
    lb.t.x = lt.t.x;

    vertex rt = rb;
    rt_p.y = lt_p.y;
    rt.t.y = lt.t.y;

    // Triangle 1
    gl_Position = lt_p;
    out_vertex(lt);

    gl_Position = lb_p;
    out_vertex(lb);

    gl_Position = rt_p;
    out_vertex(rt);

    EndPrimitive();

    // Triangle 2
    gl_Position = lb_p;
    out_vertex(lb);

    gl_Position = rt_p;
    out_vertex(rt);

    gl_Position = rb_p;
    out_vertex(rb);

    EndPrimitive();
}

#endif

#ifdef FRAGMENT_SHADER

#if !GL_ES && __VERSION__ > 140

in SHADER
{
    vec4 t;
    vec4 c;
    flat vec4 fc;
} PSin;

#define PSin_t (PSin.t)
#define PSin_c (PSin.c)
#define PSin_fc (PSin.fc)

#else

#ifdef DISABLE_SSO
in vec4 SHADERt;
in vec4 SHADERc;
flat in vec4 SHADERfc;
#else
layout(location = 0) in vec4 SHADERt;
layout(location = 1) in vec4 SHADERc;
flat layout(location = 2) in vec4 SHADERfc;
#endif
#define PSin_t SHADERt
#define PSin_c SHADERc
#define PSin_fc SHADERfc

#endif

// Same buffer but 2 colors for dual source blending
#if GL_ES
layout(location = 0) out vec4 SV_Target0;
#else
layout(location = 0, index = 0) out vec4 SV_Target0;
layout(location = 0, index = 1) out vec4 SV_Target1;
#endif

#ifdef DISABLE_GL42
uniform sampler2D TextureSampler;
uniform sampler2D PaletteSampler;
uniform sampler2D RTCopySampler;
#else
layout(binding = 0) uniform sampler2D TextureSampler;
layout(binding = 1) uniform sampler2D PaletteSampler;
layout(binding = 2) uniform sampler2D RTCopySampler;
#endif

#ifndef DISABLE_GL42_image
#if PS_DATE > 0
// FIXME how to declare memory access
layout(r32ui, binding = 0) coherent uniform uimage2D img_prim_min;
#endif
#else
// use basic stencil
#endif

#ifndef DISABLE_GL42_image
#if PS_DATE > 0
// origin_upper_left
layout(pixel_center_integer) in vec4 gl_FragCoord;
//in int gl_PrimitiveID;
#endif
#endif

#ifdef DISABLE_GL42
layout(std140) uniform cb21
#else
layout(std140, binding = 21) uniform cb21
#endif
{
    vec3 FogColor;
    float AREF;
    vec4 HalfTexel;
    vec4 WH;
    vec4 MinMax;
    vec2 MinF;
    vec2 TA;
    uvec4 MskFix;
    vec4 TC_OffsetHack;
};

vec4 sample_c(vec2 uv)
{
    // FIXME: check the issue on openGL
	if (ATI_SUCKS == 1 && PS_POINT_SAMPLER == 1)
	{
		// Weird issue with ATI cards (happens on at least HD 4xxx and 5xxx),
		// it looks like they add 127/128 of a texel to sampling coordinates
		// occasionally causing point sampling to erroneously round up.
		// I'm manually adjusting coordinates to the centre of texels here,
		// though the centre is just paranoia, the top left corner works fine.
		uv = (trunc(uv * WH.zw) + vec2(0.5, 0.5)) / WH.zw;
	}

    return texture(TextureSampler, uv);
}

vec4 sample_p(float u)
{
    //FIXME do we need a 1D sampler. Big impact on opengl to find 1 dim
    // So for the moment cheat with 0.0f dunno if it work
    return texture(PaletteSampler, vec2(u, 0.0f));
}

vec4 sample_rt(vec2 uv)
{
    return texture(RTCopySampler, uv);
}

vec4 wrapuv(vec4 uv)
{
    vec4 uv_out = uv;

    if(PS_WMS == PS_WMT)
    {
        if(PS_WMS == 2)
        {
            uv_out = clamp(uv, MinMax.xyxy, MinMax.zwzw);
        }
        else if(PS_WMS == 3)
        {
            uv_out = vec4((ivec4(uv * WH.xyxy) & ivec4(MskFix.xyxy)) | ivec4(MskFix.zwzw)) / WH.xyxy;
        }
    }
    else
    {
        if(PS_WMS == 2)
        {
            uv_out.xz = clamp(uv.xz, MinMax.xx, MinMax.zz);
        }
        else if(PS_WMS == 3)
        {
            uv_out.xz = vec2((ivec2(uv.xz * WH.xx) & ivec2(MskFix.xx)) | ivec2(MskFix.zz)) / WH.xx;
        }
        if(PS_WMT == 2)
        {
            uv_out.yw = clamp(uv.yw, MinMax.yy, MinMax.ww);
        }
        else if(PS_WMT == 3)
        {
            uv_out.yw = vec2((ivec2(uv.yw * WH.yy) & ivec2(MskFix.yy)) | ivec2(MskFix.ww)) / WH.yy;
        }
    }

    return uv_out;
}

vec2 clampuv(vec2 uv)
{
    vec2 uv_out = uv;

    if(PS_WMS == 2 && PS_WMT == 2) 
    {
        uv_out = clamp(uv, MinF, MinMax.zw);
    }
    else if(PS_WMS == 2)
    {
        uv_out.x = clamp(uv.x, MinF.x, MinMax.z);
    }
    else if(PS_WMT == 2)
    {
        uv_out.y = clamp(uv.y, MinF.y, MinMax.w);
    }

    return uv_out;
}

mat4 sample_4c(vec4 uv)
{
    mat4 c;

    c[0] = sample_c(uv.xy);
    c[1] = sample_c(uv.zy);
    c[2] = sample_c(uv.xw);
    c[3] = sample_c(uv.zw);

    return c;
}

vec4 sample_4a(vec4 uv)
{
    vec4 c;

    // Dx used the alpha channel.
    // Opengl is only 8 bits on red channel.
    c.x = sample_c(uv.xy).r;
    c.y = sample_c(uv.zy).r;
    c.z = sample_c(uv.xw).r;
    c.w = sample_c(uv.zw).r;

	return c * 255.0/256.0 + 0.5/256.0;
}

mat4 sample_4p(vec4 u)
{
    mat4 c;

    c[0] = sample_p(u.x);
    c[1] = sample_p(u.y);
    c[2] = sample_p(u.z);
    c[3] = sample_p(u.w);

    return c;
}

vec4 sample_color(vec2 st, float q)
{
    if(PS_FST == 0) st /= q;

    if(PS_TCOFFSETHACK == 1) st += TC_OffsetHack.xy;

    vec4 t;
    mat4 c;
    vec2 dd;

    if (PS_LTF == 0 && PS_FMT <= FMT_16 && PS_WMS < 3 && PS_WMT < 3)
    {
        c[0] = sample_c(clampuv(st));
    }
    else
    {
        vec4 uv;

        if(PS_LTF != 0)
        {
            uv = st.xyxy + HalfTexel;
            dd = fract(uv.xy * WH.zw);
        }
        else
        {
            uv = st.xyxy;
        }

        uv = wrapuv(uv);

        if((PS_FMT & FMT_PAL) != 0)
        {
            c = sample_4p(sample_4a(uv));
        }
        else
        {
            c = sample_4c(uv);
        }
    }

    // PERF: see the impact of the exansion before/after the interpolation
    for (int i = 0; i < 4; i++)
    {
        if((PS_FMT & ~FMT_PAL) == FMT_24)
        {
            // FIXME GLSL any only support bvec so try to mix it with notEqual
            bvec3 rgb_check = notEqual( c[i].rgb, vec3(0.0f, 0.0f, 0.0f) );
            c[i].a = ( (PS_AEM == 0) || any(rgb_check)  ) ? TA.x : 0.0f;
        }
        else if((PS_FMT & ~FMT_PAL) == FMT_16)
        {
            // FIXME GLSL any only support bvec so try to mix it with notEqual
            bvec3 rgb_check = notEqual( c[i].rgb, vec3(0.0f, 0.0f, 0.0f) );
            c[i].a = c[i].a >= 0.5 ? TA.y : ( (PS_AEM == 0) || any(rgb_check) ) ? TA.x : 0.0f;
        }
    }

    if(PS_LTF != 0)
    {
        t = mix(mix(c[0], c[1], dd.x), mix(c[2], c[3], dd.x), dd.y);
    }
    else
    {
        t = c[0];
    }

    return t;
}

vec4 tfx(vec4 t, vec4 c)
{
    vec4 c_out = c;
    if(PS_TFX == 0)
    {
        if(PS_TCC != 0) 
        {
            c_out = c * t * 255.0f / 128.0f;
        }
        else
        {
            c_out.rgb = c.rgb * t.rgb * 255.0f / 128.0f;
        }
    }
    else if(PS_TFX == 1)
    {
        if(PS_TCC != 0) 
        {
            c_out = t;
        }
        else
        {
            c_out.rgb = t.rgb;
        }
    }
    else if(PS_TFX == 2)
    {
        c_out.rgb = c.rgb * t.rgb * 255.0f / 128.0f + c.a;

        if(PS_TCC != 0) 
        {
            c_out.a += t.a;
        }
    }
    else if(PS_TFX == 3)
    {
        c_out.rgb = c.rgb * t.rgb * 255.0f / 128.0f + c.a;

        if(PS_TCC != 0) 
        {
            c_out.a = t.a;
        }
    }

    return clamp(c_out, vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(1.0f, 1.0f, 1.0f, 1.0f));
}

#if 0
void datst()
{
#if PS_DATE > 0
    float alpha = sample_rt(PSin_tp.xy).a;
    float alpha0x80 = 128.0 / 255;

    if (PS_DATE == 1 && alpha >= alpha0x80)
        discard;
    else if (PS_DATE == 2 && alpha < alpha0x80)
        discard;
#endif
}
#endif

// Note layout stuff might require gl4.3
#ifdef SUBROUTINE_GL40
// Function pointer type
subroutine void AlphaTestType(vec4 c);

// a function pointer variable
layout(location = 0) subroutine uniform AlphaTestType atst;

// The function attached to AlphaTestType
layout(index = 0) subroutine(AlphaTestType)
void atest_never(vec4 c)
{
    discard;
}

layout(index = 1) subroutine(AlphaTestType)
void atest_always(vec4 c)
{
    // Nothing to do
}

layout(index = 2) subroutine(AlphaTestType)
void atest_l(vec4 c)
{
    float a = trunc(c.a * 255.0 + 0.01);
    if (PS_SPRITEHACK == 0)
        if ((AREF - a - 0.5f) < 0.0f)
            discard;
}

layout(index = 3) subroutine(AlphaTestType)
void atest_le(vec4 c)
{
    float a = trunc(c.a * 255.0 + 0.01);
    if ((AREF - a + 0.5f) < 0.0f)
        discard;
}

layout(index = 4) subroutine(AlphaTestType)
void atest_e(vec4 c)
{
    float a = trunc(c.a * 255.0 + 0.01);
    if ((0.5f - abs(a - AREF)) < 0.0f)
        discard;
}

layout(index = 5) subroutine(AlphaTestType)
void atest_ge(vec4 c)
{
    float a = trunc(c.a * 255.0 + 0.01);
    if ((a-AREF + 0.5f) < 0.0f)
        discard;
}

layout(index = 6) subroutine(AlphaTestType)
void atest_g(vec4 c)
{
    float a = trunc(c.a * 255.0 + 0.01);
    if ((a-AREF - 0.5f) < 0.0f)
        discard;
}

layout(index = 7) subroutine(AlphaTestType)
void atest_ne(vec4 c)
{
    float a = trunc(c.a * 255.0 + 0.01);
    if ((abs(a - AREF) - 0.5f) < 0.0f)
        discard;
}

#else
void atst(vec4 c)
{
    float a = trunc(c.a * 255.0 + 0.01);

    if(PS_ATST == 0) // never
    {
        discard;
    }
    else if(PS_ATST == 1) // always
    {
        // nothing to do
    }
    else if(PS_ATST == 2 ) // l
    {
        if (PS_SPRITEHACK == 0)
            if ((AREF - a - 0.5f) < 0.0f)
                discard;
    }
    else if(PS_ATST == 3 ) // le
    {
        if ((AREF - a + 0.5f) < 0.0f)
            discard;
    }
    else if(PS_ATST == 4) // e
    {
        if ((0.5f - abs(a - AREF)) < 0.0f)
            discard;
    }
    else if(PS_ATST == 5) // ge
    {
        if ((a-AREF + 0.5f) < 0.0f)
            discard;
    }
    else if(PS_ATST == 6) // g
    {
        if ((a-AREF - 0.5f) < 0.0f)
            discard;
    }
    else if(PS_ATST == 7) // ne
    {
        if ((abs(a - AREF) - 0.5f) < 0.0f)
            discard;
    }
}
#endif

// Note layout stuff might require gl4.3
#ifdef SUBROUTINE_GL40
// Function pointer type
subroutine void ColClipType(inout vec4 c);

// a function pointer variable
layout(location = 1) subroutine uniform ColClipType colclip;

layout(index = 8) subroutine(ColClipType)
void colclip_0(inout vec4 c)
{
	// nothing to do
}

layout(index = 9) subroutine(ColClipType)
void colclip_1(inout vec4 c)
{
	// FIXME !!!!
	//c.rgb *= c.rgb < 128./255;
	bvec3 factor = bvec3(128.0f/255.0f, 128.0f/255.0f, 128.0f/255.0f);
	c.rgb *= vec3(factor);
}

layout(index = 10) subroutine(ColClipType)
void colclip_2(inout vec4 c)
{
	c.rgb = 256.0f/255.0f - c.rgb;
	// FIXME !!!!
	//c.rgb *= c.rgb < 128./255;
	bvec3 factor = bvec3(128.0f/255.0f, 128.0f/255.0f, 128.0f/255.0f);
	c.rgb *= vec3(factor);
}

#else
void colclip(inout vec4 c)
{
    if (PS_COLCLIP == 2)
    {
        c.rgb = 256.0f/255.0f - c.rgb;
    }
    if (PS_COLCLIP > 0)
    {
        // FIXME !!!!
        //c.rgb *= c.rgb < 128./255;
        bvec3 factor = bvec3(128.0f/255.0f, 128.0f/255.0f, 128.0f/255.0f);
        c.rgb *= vec3(factor);
    }
}
#endif

void fog(vec4 c, float f)
{
    if(PS_FOG != 0)
    {
        c.rgb = mix(FogColor, c.rgb, f);
    }
}

vec4 ps_color()
{
    vec4 t = sample_color(PSin_t.xy, PSin_t.w);

#if PS_IIP == 1
    vec4 c = tfx(t, PSin_c);
#else
    vec4 c = tfx(t, PSin_fc);
#endif

    atst(c);

    fog(c, PSin_t.z);

	colclip(c);

    if(PS_CLR1 != 0) // needed for Cd * (As/Ad/F + 1) blending modes
    {
        c.rgb = vec3(1.0f, 1.0f, 1.0f); 
    }

    return c;
}

#if GL_ES
void ps_main()
{
    vec4 c = ps_color();
    c.a *= 2.0;
    SV_Target0 = c;
}
#endif

#if !GL_ES
void ps_main()
{
    vec4 c = ps_color();

    float alpha = c.a * 2.0;

    if(PS_AOUT != 0) // 16 bit output
    {
        float a = 128.0f / 255.0; // alpha output will be 0x80

        c.a = (PS_FBA != 0) ? a : step(0.5, c.a) * a;
    }
    else if(PS_FBA != 0)
    {
        if(c.a < 0.5) c.a += 0.5;
    }

    // Get first primitive that will write a failling alpha value
#if PS_DATE == 1 && !defined(DISABLE_GL42_image)
    // DATM == 0
    // Pixel with alpha equal to 1 will failed
    if (c.a > 127.5f / 255.0f) {
        imageAtomicMin(img_prim_min, ivec2(gl_FragCoord.xy), gl_PrimitiveID);
    }
    //memoryBarrier();
#elif PS_DATE == 2 && !defined(DISABLE_GL42_image)
    // DATM == 1
    // Pixel with alpha equal to 0 will failed
    if (c.a < 127.5f / 255.0f) {
        imageAtomicMin(img_prim_min, ivec2(gl_FragCoord.xy), gl_PrimitiveID);
    }
#endif

    // TODO
    // warning non uniform flow ???
#if PS_DATE == 3 && !defined(DISABLE_GL42_image)
    uint stencil_ceil = imageLoad(img_prim_min, ivec2(gl_FragCoord.xy));
    // Note gl_PrimitiveID == stencil_ceil will be the primitive that will update
    // the bad alpha value so we must keep it.
#if 0
	if (stencil_ceil > 0)
		c = vec4(1.0, 0.0, 0.0, 1.0);
	else
		c = vec4(0.0, 1.0, 0.0, 1.0);
#endif

#if 1
	if (gl_PrimitiveID > stencil_ceil) {
		discard;
	}
#endif

#endif


#if (PS_DATE == 2 || PS_DATE == 1) && !defined(DISABLE_GL42_image)
    // Don't write anything on the framebuffer
    // Note: you can't use discard because it will also drop
    // image operation
    // Note2: output will be disabled too in opengl
#else
    SV_Target0 = c;
    SV_Target1 = vec4(alpha, alpha, alpha, alpha);
#endif

}
#endif // !GL_ES

#endif
