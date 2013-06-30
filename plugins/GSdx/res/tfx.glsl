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

#ifndef GS_IIP
#define GS_IIP 0
#define GS_PRIM 3
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
#endif

struct vertex
{
    vec4 t;
    vec4 tp;
    vec4 c;
};

#ifdef VERTEX_SHADER
layout(location = 0) in vec2  i_st;
layout(location = 1) in vec4  i_c;
layout(location = 2) in float i_q;
layout(location = 3) in uvec2 i_p;
layout(location = 4) in uint  i_z;
layout(location = 5) in uvec2 i_uv;
layout(location = 6) in vec4  i_f;

#if __VERSION__ > 140 && !(defined(NO_STRUCT))
layout(location = 0) out vertex VSout;
#define VSout_p (VSout.p)
#define VSout_t (VSout.t)
#define VSout_tp (VSout.tp)
#define VSout_c (VSout.c)
#else
#ifdef DISABLE_SSO
out vec4 SHADERt;
out vec4 SHADERtp;
out vec4 SHADERc;
#else
layout(location = 0) out vec4 SHADERt;
layout(location = 1) out vec4 SHADERtp;
layout(location = 2) out vec4 SHADERc;
#endif
#define VSout_t SHADERt
#define VSout_tp SHADERtp
#define VSout_c SHADERc
#endif

#if __VERSION__ > 140
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
            VSout_t.xy = i_uv * TextureScale;
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
    VSout_t.z = i_f.r;
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

layout(location = 0) in vertex GSin[];

layout(location = 0) out vertex GSout;

#if GS_PRIM == 0
layout(points) in;
layout(points, max_vertices = 1) out;

void gs_main()
{
    for(int i = 0; i < gl_in.length(); i++) {
        gl_Position = gl_in[i].gl_Position;
        GSout = GSin[i];
        EmitVertex();
    }
    EndPrimitive();
}

#elif GS_PRIM == 1
layout(lines) in;
layout(line_strip, max_vertices = 2) out;

void gs_main()
{
    for(int i = 0; i < gl_in.length(); i++) {
        gl_Position = gl_in[i].gl_Position;
        GSout = GSin[i];
#if GS_IIP == 0
        if (i == 0)
            GSout.c = GSin[1].c;
#endif
        EmitVertex();
    }
    EndPrimitive();
}

#elif GS_PRIM == 2
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

void gs_main()
{
    for(int i = 0; i < gl_in.length(); i++) {
        gl_Position = gl_in[i].gl_Position;
        GSout = GSin[i];
#if GS_IIP == 0
        if (i == 0 || i == 1)
            GSout.c = GSin[2].c;
#endif
        EmitVertex();
    }
    EndPrimitive();
}

#elif GS_PRIM == 3
layout(lines) in;
layout(triangle_strip, max_vertices = 6) out;

void gs_main()
{
    // left top     => GSin[0];
    // right bottom => GSin[1];
    vertex rb = GSin[1];
    vertex lt = GSin[0];
    vec4 rb_p = gl_in[1].gl_Position;
    vec4 lb_p = gl_in[1].gl_Position;
    vec4 rt_p = gl_in[1].gl_Position;
    vec4 lt_p = gl_in[0].gl_Position;

    lt_p.z = rb_p.z;
    lt.t.zw = rb.t.zw;
#if GS_IIP == 0
    lt.c = rb.c;
#endif

    vertex lb = rb;
    lb_p.x = lt_p.x;
    lb.t.x = lt.t.x;

    vertex rt = rb;
    rt_p.y = lt_p.y;
    rt.t.y = lt.t.y;

    // Triangle 1
    gl_Position = lt_p;
    GSout = lt;
    EmitVertex();

    gl_Position = lb_p;
    GSout = lb;
    EmitVertex();

    gl_Position = rt_p;
    GSout = rt;
    EmitVertex();

    EndPrimitive();

    // Triangle 2
    gl_Position = lb_p;
    GSout = lb;
    EmitVertex();

    gl_Position = rt_p;
    GSout = rt;
    EmitVertex();

    gl_Position = rb_p;
    GSout = rb;
    EmitVertex();

    EndPrimitive();

}

#endif

#endif

#ifdef FRAGMENT_SHADER
#if __VERSION__ > 140 && !(defined(NO_STRUCT))
layout(location = 0) in vertex PSin;
#define PSin_t (PSin.t)
#define PSin_tp (PSin.tp)
#define PSin_c (PSin.c)
#else
#ifdef DISABLE_SSO
in vec4 SHADERt;
in vec4 SHADERtp;
in vec4 SHADERc;
#else
layout(location = 0) in vec4 SHADERt;
layout(location = 1) in vec4 SHADERtp;
layout(location = 2) in vec4 SHADERc;
#endif
#define PSin_t SHADERt
#define PSin_tp SHADERtp
#define PSin_c SHADERc
#endif

// Same buffer but 2 colors for dual source blending
layout(location = 0, index = 0) out vec4 SV_Target0;
layout(location = 0, index = 1) out vec4 SV_Target1;

#ifdef DISABLE_GL42
uniform sampler2D TextureSampler;
uniform sampler2D PaletteSampler;
uniform sampler2D RTCopySampler;
#else
layout(binding = 0) uniform sampler2D TextureSampler;
layout(binding = 1) uniform sampler2D PaletteSampler;
layout(binding = 2) uniform sampler2D RTCopySampler;
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
            uv_out = vec4(((ivec4(uv * WH.xyxy) & ivec4(MskFix.xyxy)) | ivec4(MskFix.zwzw)) / WH.xyxy);
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
            uv_out.xz = vec2(((ivec2(uv.xz * WH.xx) & ivec2(MskFix.xx)) | ivec2(MskFix.zz)) / WH.xx);
        }
        if(PS_WMT == 2)
        {
            uv_out.yw = clamp(uv.yw, MinMax.yy, MinMax.ww);
        }
        else if(PS_WMT == 3)
        {
            uv_out.yw = vec2(((ivec2(uv.yw * WH.yy) & ivec2(MskFix.yy)) | ivec2(MskFix.ww)) / WH.yy);
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

	return c * 255./256 + 0.5/256;
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
            c_out = c * t * 255.0f / 128;
        }
        else
        {
            c_out.rgb = c.rgb * t.rgb * 255.0f / 128;
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
        c_out.rgb = c.rgb * t.rgb * 255.0f / 128 + c.a;

        if(PS_TCC != 0) 
        {
            c_out.a += t.a;
        }
    }
    else if(PS_TFX == 3)
    {
        c_out.rgb = c.rgb * t.rgb * 255.0f / 128 + c.a;

        if(PS_TCC != 0) 
        {
            c_out.a = t.a;
        }
    }

    return clamp(c_out, vec4(0.0f, 0.0f, 0.0f, 0.0f), vec4(1.0f, 1.0f, 1.0f, 1.0f));
}

void datst()
{
#if PS_DATE > 0
    float alpha = sample_rt(PSin_tp.xy).a;
    float alpha0x80 = 128. / 255;

    if (PS_DATE == 1 && alpha >= alpha0x80)
        discard;
    else if (PS_DATE == 2 && alpha < alpha0x80)
        discard;
#endif
}

void atst(vec4 c)
{
    float a = trunc(c.a * 255 + 0.01);

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

vec4 fog(vec4 c, float f)
{
    vec4 c_out = c;
    if(PS_FOG != 0)
    {
        c_out.rgb = mix(FogColor, c.rgb, f);
    }

    return c_out;
}

vec4 ps_color()
{
    datst();

    vec4 t = sample_color(PSin_t.xy, PSin_t.w);

    vec4 c = tfx(t, PSin_c);

    atst(c);

    c = fog(c, PSin_t.z);

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

    if(PS_CLR1 != 0) // needed for Cd * (As/Ad/F + 1) blending modes
    {
        c.rgb = vec3(1.0f, 1.0f, 1.0f); 
    }

    return c;
}

void ps_main()
{
    vec4 c = ps_color();

    float alpha = c.a * 2;

    if(PS_AOUT != 0) // 16 bit output
    {
        float a = 128.0f / 255; // alpha output will be 0x80

        c.a = (PS_FBA != 0) ? a : step(0.5, c.a) * a;
    }
    else if(PS_FBA != 0)
    {
        if(c.a < 0.5) c.a += 0.5;
    }

    SV_Target0 = c;
    SV_Target1 = vec4(alpha, alpha, alpha, alpha);
}
#endif
