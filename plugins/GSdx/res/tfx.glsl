//#version 420 // Keep it for text editor detection

// note lerp => mix

#define FMT_32 0
#define FMT_24 1
#define FMT_16 2
#define FMT_8H 3
#define FMT_4HL 4
#define FMT_4HH 5
#define FMT_8 6

#ifndef VS_BPPZ
#define VS_BPPZ 0
#define VS_TME 1
#define VS_FST 1
#endif

#ifndef GS_IIP
#define GS_IIP 0
#define GS_PRIM 3
#endif

#ifndef PS_FST
#define PS_FST 0
#define PS_WMS 0
#define PS_WMT 0
#define PS_FMT FMT_8
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
#endif

struct vertex
{
    vec4 p;
    vec4 t;
    vec4 tp;
    vec4 c;
};

#ifdef VERTEX_SHADER
layout(location = 0) in vec2  i_t;
layout(location = 1) in vec4  i_c;
layout(location = 2) in float i_q;
layout(location = 3) in uvec2 i_p;
layout(location = 4) in uint  i_z;
layout(location = 5) in vec4  i_f;

layout(location = 0) out vertex OUT;

out gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
};

layout(std140, binding = 4) uniform cb0
{
	vec4 VertexScale;
	vec4 VertexOffset;
	vec2 TextureScale;
};

void vs_main()
{
    uint z;
	if(VS_BPPZ == 1) // 24
		z = i_z & 0xffffff; 
	else if(VS_BPPZ == 2) // 16
		z = i_z & 0xffff;
    else
        z = i_z;

	// pos -= 0.05 (1/320 pixel) helps avoiding rounding problems (integral part of pos is usually 5 digits, 0.05 is about as low as we can go)
	// example: ceil(afterseveralvertextransformations(y = 133)) => 134 => line 133 stays empty
	// input granularity is 1/16 pixel, anything smaller than that won't step drawing up/left by one pixel
	// example: 133.0625 (133 + 1/16) should start from line 134, ceil(133.0625 - 0.05) still above 133
	
	vec4 p = vec4(i_p, z, 0) - vec4(0.05f, 0.05f, 0, 0); 
	vec4 final_p = p * VertexScale - VertexOffset;

	OUT.p = final_p;
    gl_Position = final_p; // NOTE I don't know if it is possible to merge POSITION_OUT and gl_Position
#if VS_RTCOPY
	OUT.tp = final_p * vec4(0.5, -0.5, 0, 0) + 0.5;
#endif

	if(VS_TME != 0)
	{
		if(VS_FST != 0)
		{
			OUT.t.xy = i_t * TextureScale;
			OUT.t.w = 1.0f;
		}
		else
		{
			OUT.t.xy = i_t;
			OUT.t.w = i_q;
		}
	}
	else
	{
		OUT.t.xy = vec2(0.0f, 0.0f);
		OUT.t.w = 1.0f;
	}

	OUT.c = i_c;
	OUT.t.z = i_f.a;
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
        gl_Position = gl_in[i].gl_Position; // FIXME is it useful
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
        gl_Position = gl_in[i].gl_Position; // FIXME is it useful
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
        gl_Position = gl_in[i].gl_Position; // FIXME is it useful
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
layout(triangle_strip, max_vertices = 4) out;

void gs_main()
{
    // left top     => GSin[0];
    // right bottom => GSin[1];


    // left top
	GSout = GSin[0];

    GSout.p.z = GSin[1].p.z;
    GSout.t.zw = GSin[1].t.zw;
    gl_Position = GSout.p; // FIXME is it useful
	#if GS_IIP == 0
	GSout.c = GSin[1].c;
	#endif
    EmitVertex();

    // left bottom
	GSout = GSin[1];
    gl_Position = gl_in[1].gl_Position; // FIXME is it useful
    gl_Position.x = GSin[0].p.x;
    GSout.p.x = GSin[0].p.x;
    GSout.t.x = GSin[0].t.x;
    EmitVertex();

    // rigth top
	GSout = GSin[1];
    gl_Position = gl_in[1].gl_Position; // FIXME is it useful
    gl_Position.y = GSin[0].p.y;
    GSout.p.y = GSin[0].p.y;
    GSout.t.y = GSin[0].t.y;
    EmitVertex();

    // rigth bottom
	GSout = GSin[1];
    gl_Position = GSin[1].p; // FIXME is it useful
    EmitVertex();

}

#endif

#endif

#ifdef FRAGMENT_SHADER
layout(location = 0) in vertex PSin;

// Same buffer but 2 colors for dual source blending
//FIXME
#if 1
	layout(location = 0, index = 0) out vec4 SV_Target0;
	layout(location = 0, index = 1) out vec4 SV_Target1;
#else
	layout(location = 0)		    out vec4 SV_Target;
#endif

layout(binding = 0) uniform sampler2D TextureSampler;
layout(binding = 1) uniform sampler2D PaletteSampler;
layout(binding = 2) uniform sampler2D RTCopySampler;

layout(std140, binding = 5) uniform cb1
{
	vec3 FogColor;
	float AREF;
	vec4 HalfTexel;
	vec4 WH;
	vec4 MinMax;
	vec2 MinF;
	vec2 TA;
	uvec4 MskFix;
};

vec4 sample_c(vec2 uv)
{
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
			uv_out = vec4(((ivec4(uv * WH.xyxy) & MskFix.xyxy) | MskFix.zwzw) / WH.xyxy);
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
			uv_out.xz = vec2(((ivec2(uv.xz * WH.xx) & MskFix.xx) | MskFix.zz) / WH.xx);
		}
		if(PS_WMT == 2)
		{
			uv_out.yw = clamp(uv.yw, MinMax.yy, MinMax.ww);
		}
		else if(PS_WMT == 3)
		{
			uv_out.yw = vec2(((ivec2(uv.yw * WH.yy) & MskFix.yy) | MskFix.ww) / WH.yy);
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

	c.x = sample_c(uv.xy).a;
	c.y = sample_c(uv.zy).a;
	c.z = sample_c(uv.xw).a;
	c.w = sample_c(uv.zw).a;
	
	return c;
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
	if(PS_FST == 0)
	{
		st /= q;
	}
	
	vec4 t;
	if((PS_FMT <= FMT_16) && (PS_WMS < 3) && (PS_WMT < 3))
	{
		t = sample_c(clampuv(st));
	}
	else
	{
		vec4 uv;
		vec2 dd;
		
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

		mat4 c;

		if(PS_FMT == FMT_8H)
		{
			c = sample_4p(sample_4a(uv));
		}
		else if(PS_FMT == FMT_4HL)
		{
            // FIXME mod and fmod are different when value are negative
			c = sample_4p(mod(sample_4a(uv), 1.0f / 16));
		}
		else if(PS_FMT == FMT_4HH)
		{
            // FIXME mod and fmod are different when value are negative
			c = sample_4p(mod(sample_4a(uv) * 16, 1.0f / 16));
		}
		else if(PS_FMT == FMT_8)
		{
			c = sample_4p(sample_4a(uv));
		}
		else
		{
			c = sample_4c(uv);
		}

		if(PS_LTF != 0)
		{
			t = mix(mix(c[0], c[1], dd.x), mix(c[2], c[3], dd.x), dd.y);
		}
		else
		{
			t = c[0];
		}
	}
	
	if(PS_FMT == FMT_32)
	{
        ;
	}
	else if(PS_FMT == FMT_24)
	{
        // FIXME GLSL any only support bvec so try to mix it with notEqual
        bvec3 rgb_check = notEqual( t.rgb, vec3(0.0f, 0.0f, 0.0f) );
		t.a = ( (PS_AEM == 0) || any(rgb_check)  ) ? TA.x : 0.0f;
	}
	else if(PS_FMT == FMT_16)
	{
		// a bit incompatible with up-scaling because the 1 bit alpha is interpolated
        // FIXME GLSL any only support bvec so try to mix it with notEqual
        bvec3 rgb_check = notEqual( t.rgb, vec3(0.0f, 0.0f, 0.0f) );
		t.a = t.a >= 0.5 ? TA.y : ( (PS_AEM == 0) || any(rgb_check) ) ? TA.x : 0.0f; 
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
	float alpha = sample_rt(PSin.tp.xy).a;
	float alpha0x80 = 128. / 255;

	if (PS_DATE == 1 && alpha >= alpha0x80)
		discard;
	else if (PS_DATE == 2 && alpha < alpha0x80)
		discard;
#endif
}

void atst(vec4 c)
{
	float a = trunc(c.a * 255);
	
	if(PS_ATST == 0) // never
	{
		discard;
	}
	else if(PS_ATST == 1) // always
	{
		// nothing to do
	}
	else if(PS_ATST == 2 || PS_ATST == 3) // l, le
	{
		if ((AREF - a) < 0.0f)
            discard;
	}
	else if(PS_ATST == 4) // e
	{
        if ((0.5f - abs(a - AREF)) < 0.0f)
            discard;
	}
	else if(PS_ATST == 5 || PS_ATST == 6) // ge, g
	{
        if ((a-AREF) < 0.0f)
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

	vec4 t = sample_color(PSin.t.xy, PSin.t.w);

	vec4 c = tfx(t, PSin.c);

	atst(c);

	c = fog(c, PSin.t.z);

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
	//FIXME
#if 1
	vec4 c = ps_color();

    // FIXME: I'm not sure about the value of others field
	// output.c1 = c.a * 2; // used for alpha blending
    SV_Target0 = vec4(c.a*2, c.a*2, c.a*2, c.a * 2);

	if(PS_AOUT != 0) // 16 bit output
	{
		float a = 128.0f / 255; // alpha output will be 0x80
		
		c.a = (PS_FBA != 0) ? a : step(0.5, c.a) * a;
	}
	else if(PS_FBA != 0)
	{
		if(c.a < 0.5) c.a += 0.5;
	}

    SV_Target1 = c;

    //SV_Target0 = vec4(1.0f,0.0f,0.0f, 1.0f);
    //SV_Target1 = vec4(0.0f,1.0f,0.0f, 1.0f);
#else
    SV_Target = vec4(1.0f,0.0f,0.0f, 1.0f);
#endif
}
#endif
