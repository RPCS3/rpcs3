#ifdef SHADER_MODEL // make safe to include in resource file to enforce dependency
#define FMT_32 0
#define FMT_24 1
#define FMT_16 2
#define FMT_8H 3
#define FMT_4HL 4
#define FMT_4HH 5
#define FMT_8 6

#if SHADER_MODEL >= 0x400

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

struct VS_INPUT
{
	uint2 p : POSITION0;
	uint z : POSITION1;
	float2 t : TEXCOORD0;
	float q : TEXCOORD1;
	float4 c : COLOR0;
	float4 f : COLOR1;
};

struct VS_OUTPUT
{
	float4 p : SV_Position;
	float4 t : TEXCOORD0;
#if VS_RTCOPY
	float4 tp : TEXCOORD1;
#endif
	float4 c : COLOR0;
};

struct PS_INPUT
{
	float4 p : SV_Position;
	float4 t : TEXCOORD0;
#if PS_DATE > 0
	float4 tp : TEXCOORD1;
#endif
	float4 c : COLOR0;
};

struct PS_OUTPUT
{
	float4 c0 : SV_Target0;
	float4 c1 : SV_Target1;
};

Texture2D<float4> Texture : register(t0);
Texture2D<float4> Palette : register(t1);
Texture2D<float4> RTCopy : register(t2);
SamplerState TextureSampler : register(s0);
SamplerState PaletteSampler : register(s1);
SamplerState RTCopySampler : register(s2);

cbuffer cb0
{
	float4 VertexScale;
	float4 VertexOffset;
	float2 TextureScale;
};

cbuffer cb1
{
	float3 FogColor;
	float AREF;
	float4 HalfTexel;
	float4 WH;
	float4 MinMax;
	float2 MinF;
	float2 TA;
	uint4 MskFix;
};

float4 sample_c(float2 uv)
{
	return Texture.Sample(TextureSampler, uv);
}

float4 sample_p(float u)
{
	return Palette.Sample(PaletteSampler, u);
}

float4 sample_rt(float2 uv)
{
	return RTCopy.Sample(RTCopySampler, uv);
}

#elif SHADER_MODEL <= 0x300

#ifndef VS_BPPZ
#define VS_BPPZ 0
#define VS_TME 1
#define VS_FST 1
#define VS_LOGZ 1
#endif

#ifndef PS_FST
#define PS_FST 0
#define PS_WMS 0
#define PS_WMT 0
#define PS_FMT FMT_8
#define PS_AEM 0
#define PS_TFX 0
#define PS_TCC 0
#define PS_ATST 4
#define PS_FOG 0
#define PS_CLR1 0
#define PS_RT 0
#define PS_LTF 0
#define PS_COLCLIP 0
#define PS_DATE 0
#endif

struct VS_INPUT
{
	float4 p : POSITION0; 
	float2 t : TEXCOORD0;
	float4 c : COLOR0;
	float4 f : COLOR1;
};

struct VS_OUTPUT
{
	float4 p : POSITION;
	float4 t : TEXCOORD0;
#if VS_RTCOPY
	float4 tp : TEXCOORD1;
#endif
	float4 c : COLOR0;
};

struct PS_INPUT
{
	float4 t : TEXCOORD0;
#if PS_DATE > 0
	float4 tp : TEXCOORD1;
#endif
	float4 c : COLOR0;
};

sampler Texture : register(s0);
sampler Palette : register(s1);
sampler RTCopy : register(s2);
sampler1D UMSKFIX : register(s3);
sampler1D VMSKFIX : register(s4);

float4 vs_params[3];

#define VertexScale vs_params[0]
#define VertexOffset vs_params[1]
#define TextureScale vs_params[2].xy

float4 ps_params[7];

#define FogColor	ps_params[0].bgr
#define AREF		ps_params[0].a
#define HalfTexel	ps_params[1]
#define WH			ps_params[2]
#define MinMax		ps_params[3]
#define MinF		ps_params[4].xy
#define TA			ps_params[4].zw

float4 sample_c(float2 uv)
{
	return tex2D(Texture, uv);
}

float4 sample_p(float u)
{
	return tex2D(Palette, u);
}

float4 sample_rt(float2 uv)
{
	return tex2D(RTCopy, uv);
}

#endif

float4 wrapuv(float4 uv)
{
	if(PS_WMS == PS_WMT)
	{
/*
		if(PS_WMS == 0)
		{
			uv = frac(uv);
		}
		else if(PS_WMS == 1)
		{
			uv = saturate(uv);
		}
		else
*/ 
		if(PS_WMS == 2)
		{
			uv = clamp(uv, MinMax.xyxy, MinMax.zwzw);
		}
		else if(PS_WMS == 3)
		{
			#if SHADER_MODEL >= 0x400
			uv = (float4)(((int4)(uv * WH.xyxy) & MskFix.xyxy) | MskFix.zwzw) / WH.xyxy;
			#elif SHADER_MODEL <= 0x300
			uv.x = tex1D(UMSKFIX, uv.x);
			uv.y = tex1D(VMSKFIX, uv.y);
			uv.z = tex1D(UMSKFIX, uv.z);
			uv.w = tex1D(VMSKFIX, uv.w);
			#endif
		}
	}
	else
	{
/*	
		if(PS_WMS == 0)
		{
			uv.xz = frac(uv.xz);
		}
		else if(PS_WMS == 1)
		{
			uv.xz = saturate(uv.xz);
		}
		else 
*/		
		if(PS_WMS == 2)
		{
			uv.xz = clamp(uv.xz, MinMax.xx, MinMax.zz);
		}
		else if(PS_WMS == 3)
		{
			#if SHADER_MODEL >= 0x400
			uv.xz = (float2)(((int2)(uv.xz * WH.xx) & MskFix.xx) | MskFix.zz) / WH.xx;
			#elif SHADER_MODEL <= 0x300
			uv.x = tex1D(UMSKFIX, uv.x);
			uv.z = tex1D(UMSKFIX, uv.z);
			#endif
		}
/*
		if(PS_WMT == 0)
		{
			uv.yw = frac(uv.yw);
		}
		else if(PS_WMT == 1)
		{
			uv.yw = saturate(uv.yw);
		}
		else 
*/
		if(PS_WMT == 2)
		{
			uv.yw = clamp(uv.yw, MinMax.yy, MinMax.ww);
		}
		else if(PS_WMT == 3)
		{
			#if SHADER_MODEL >= 0x400
			uv.yw = (float2)(((int2)(uv.yw * WH.yy) & MskFix.yy) | MskFix.ww) / WH.yy;
			#elif SHADER_MODEL <= 0x300
			uv.y = tex1D(VMSKFIX, uv.y);
			uv.w = tex1D(VMSKFIX, uv.w);
			#endif
		}
	}
	
	return uv;
}

float2 clampuv(float2 uv)
{
	if(PS_WMS == 2 && PS_WMT == 2) 
	{
		uv = clamp(uv, MinF, MinMax.zw);
	}
	else if(PS_WMS == 2)
	{
		uv.x = clamp(uv.x, MinF.x, MinMax.z);
	}
	else if(PS_WMT == 2)
	{
		uv.y = clamp(uv.y, MinF.y, MinMax.w);
	}
	
	return uv;
}

float4x4 sample_4c(float4 uv)
{
	float4x4 c;
	
	c[0] = sample_c(uv.xy);
	c[1] = sample_c(uv.zy);
	c[2] = sample_c(uv.xw);
	c[3] = sample_c(uv.zw);

	return c;
}

float4 sample_4a(float4 uv)
{
	float4 c;

	c.x = sample_c(uv.xy).a;
	c.y = sample_c(uv.zy).a;
	c.z = sample_c(uv.xw).a;
	c.w = sample_c(uv.zw).a;
	
	#if SHADER_MODEL <= 0x300
	if(PS_RT) c *= 128.0f / 255;
	#endif

	return c;
}

float4x4 sample_4p(float4 u)
{
	float4x4 c;
	
	c[0] = sample_p(u.x);
	c[1] = sample_p(u.y);
	c[2] = sample_p(u.z);
	c[3] = sample_p(u.w);

	return c;
}

float4 sample(float2 st, float q)
{
	if(!PS_FST)
	{
		st /= q;
	}
	
	float4 t;
/*	
	if(PS_FMT <= FMT_16 && PS_WMS < 2 && PS_WMT < 2)
	{
		t = sample_c(st);
	}
*/
	if(PS_FMT <= FMT_16 && PS_WMS < 3 && PS_WMT < 3)
	{
		t = sample_c(clampuv(st));
	}
	else
	{
		float4 uv;
		float2 dd;
		
		if(PS_LTF)
		{
			uv = st.xyxy + HalfTexel;
			dd = frac(uv.xy * WH.zw); 
		}
		else
		{
			uv = st.xyxy;
		}
		
		uv = wrapuv(uv);

		float4x4 c;

		if(PS_FMT == FMT_8H)
		{
			c = sample_4p(sample_4a(uv));
		}
		else if(PS_FMT == FMT_4HL)
		{
			c = sample_4p(fmod(sample_4a(uv), 1.0f / 16));
		}
		else if(PS_FMT == FMT_4HH)
		{
			c = sample_4p(fmod(sample_4a(uv) * 16, 1.0f / 16));
		}
		else if(PS_FMT == FMT_8)
		{
			c = sample_4p(sample_4a(uv));
		}
		else
		{
			c = sample_4c(uv);
		}

		if(PS_LTF)
		{
			t = lerp(lerp(c[0], c[1], dd.x), lerp(c[2], c[3], dd.x), dd.y);
		}
		else
		{
			t = c[0];
		}
	}
	
	if(PS_FMT == FMT_32)
	{
		#if SHADER_MODEL <= 0x300
		if(PS_RT) t.a *= 128.0f / 255;
		#endif
	}
	else if(PS_FMT == FMT_24)
	{
		t.a = !PS_AEM || any(t.rgb) ? TA.x : 0;
	}
	else if(PS_FMT == FMT_16)
	{
		// a bit incompatible with up-scaling because the 1 bit alpha is interpolated
	
		t.a = t.a >= 0.5 ? TA.y : !PS_AEM || any(t.rgb) ? TA.x : 0; 
	}
	
	return t;
}

float4 tfx(float4 t, float4 c)
{
	if(PS_TFX == 0)
	{
		if(PS_TCC) 
		{
			c = c * t * 255.0f / 128;
		}
		else
		{
			c.rgb = c.rgb * t.rgb * 255.0f / 128;
		}
	}
	else if(PS_TFX == 1)
	{
		if(PS_TCC) 
		{
			c = t;
		}
		else
		{
			c.rgb = t.rgb;
		}
	}
	else if(PS_TFX == 2)
	{
		c.rgb = c.rgb * t.rgb * 255.0f / 128 + c.a;

		if(PS_TCC) 
		{
			c.a += t.a;
		}
	}
	else if(PS_TFX == 3)
	{
		c.rgb = c.rgb * t.rgb * 255.0f / 128 + c.a;

		if(PS_TCC) 
		{
			c.a = t.a;
		}
	}
	
	return saturate(c);
}

void datst(PS_INPUT input)
{
#if PS_DATE > 0
	float alpha = sample_rt(input.tp.xy).a;
#if SHADER_MODEL >= 0x400
	float alpha0x80 = 128. / 255;
#else
	float alpha0x80 = 1;
#endif

	if (PS_DATE == 1 && alpha >= alpha0x80)
		discard;
	else if (PS_DATE == 2 && alpha < alpha0x80)
		discard;
#endif
}

void atst(float4 c)
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
		clip(AREF - a);
	}
	else if(PS_ATST == 4) // e
	{
		clip(0.5f - abs(a - AREF));
	}
	else if(PS_ATST == 5 || PS_ATST == 6) // ge, g
	{
		clip(a - AREF);
	}
	else if(PS_ATST == 7) // ne
	{
		clip(abs(a - AREF) - 0.5f);
	}
}

float4 fog(float4 c, float f)
{
	if(PS_FOG)
	{
		c.rgb = lerp(FogColor, c.rgb, f);
	}

	return c;
}

float4 ps_color(PS_INPUT input)
{
	datst(input);

	float4 t = sample(input.t.xy, input.t.w);

	float4 c = tfx(t, input.c);

	atst(c);

	c = fog(c, input.t.z);

	if (PS_COLCLIP == 2)
	{
		c.rgb = 256./255. - c.rgb;
	}
	if (PS_COLCLIP > 0)
	{
		c.rgb *= c.rgb < 128./255;
	}

	if(PS_CLR1) // needed for Cd * (As/Ad/F + 1) blending modes
	{
		c.rgb = 1; 
	}

	return c;
}

#if SHADER_MODEL >= 0x400

VS_OUTPUT vs_main(VS_INPUT input)
{
	if(VS_BPPZ == 1) // 24
	{
		input.z = input.z & 0xffffff; 
	}
	else if(VS_BPPZ == 2) // 16
	{
		input.z = input.z & 0xffff;
	}

	VS_OUTPUT output;
	
	// pos -= 0.05 (1/320 pixel) helps avoiding rounding problems (integral part of pos is usually 5 digits, 0.05 is about as low as we can go)
	// example: ceil(afterseveralvertextransformations(y = 133)) => 134 => line 133 stays empty
	// input granularity is 1/16 pixel, anything smaller than that won't step drawing up/left by one pixel
	// example: 133.0625 (133 + 1/16) should start from line 134, ceil(133.0625 - 0.05) still above 133
	
	float4 p = float4(input.p, input.z, 0) - float4(0.05f, 0.05f, 0, 0); 

	output.p = p * VertexScale - VertexOffset;
#if VS_RTCOPY
	output.tp = (p * VertexScale - VertexOffset) * float4(0.5, -0.5, 0, 0) + 0.5;
#endif

	if(VS_TME)
	{
		if(VS_FST)
		{
			output.t.xy = input.t * TextureScale;
			output.t.w = 1.0f;
		}
		else
		{
			output.t.xy = input.t;
			output.t.w = input.q;
		}
	}
	else
	{
		output.t.xy = 0;
		output.t.w = 1.0f;
	}

	output.c = input.c;
	output.t.z = input.f.a;

	return output;
}

#if GS_PRIM == 0

[maxvertexcount(1)]
void gs_main(point VS_OUTPUT input[1], inout PointStream<VS_OUTPUT> stream)
{
	stream.Append(input[0]);
}

#elif GS_PRIM == 1

[maxvertexcount(2)]
void gs_main(line VS_OUTPUT input[2], inout LineStream<VS_OUTPUT> stream)
{
	#if GS_IIP == 0
	input[0].c = input[1].c;
	#endif

	stream.Append(input[0]);
	stream.Append(input[1]);
}

#elif GS_PRIM == 2

[maxvertexcount(3)]
void gs_main(triangle VS_OUTPUT input[3], inout TriangleStream<VS_OUTPUT> stream)
{
	#if GS_IIP == 0
	input[0].c = input[2].c;
	input[1].c = input[2].c;
	#endif

	stream.Append(input[0]);
	stream.Append(input[1]);
	stream.Append(input[2]);
}

#elif GS_PRIM == 3

[maxvertexcount(4)]
void gs_main(line VS_OUTPUT input[2], inout TriangleStream<VS_OUTPUT> stream)
{
	input[0].p.z = input[1].p.z;
	input[0].t.zw = input[1].t.zw;

	#if GS_IIP == 0
	input[0].c = input[1].c;
	#endif

	VS_OUTPUT lb = input[1];

	lb.p.x = input[0].p.x;
	lb.t.x = input[0].t.x;

	VS_OUTPUT rt = input[1];

	rt.p.y = input[0].p.y;
	rt.t.y = input[0].t.y;

	stream.Append(input[0]);
	stream.Append(lb);
	stream.Append(rt);
	stream.Append(input[1]);
}

#endif

PS_OUTPUT ps_main(PS_INPUT input)
{
	float4 c = ps_color(input);

	PS_OUTPUT output;

	output.c1 = c.a * 2; // used for alpha blending

	if(PS_AOUT) // 16 bit output
	{
		float a = 128.0f / 255; // alpha output will be 0x80
		
		c.a = PS_FBA ? a : step(0.5, c.a) * a;
	}
	else if(PS_FBA)
	{
		if(c.a < 0.5) c.a += 0.5;
	}

	output.c0 = c;

	return output;
}

#elif SHADER_MODEL <= 0x300

VS_OUTPUT vs_main(VS_INPUT input)
{
	if(VS_BPPZ == 1) // 24
	{
		input.p.z = fmod(input.p.z, 0x1000000); 
	}
	else if(VS_BPPZ == 2) // 16
	{
		input.p.z = fmod(input.p.z, 0x10000);
	}

	VS_OUTPUT output;
	
	// pos -= 0.05 (1/320 pixel) helps avoiding rounding problems (integral part of pos is usually 5 digits, 0.05 is about as low as we can go)
	// example: ceil(afterseveralvertextransformations(y = 133)) => 134 => line 133 stays empty
	// input granularity is 1/16 pixel, anything smaller than that won't step drawing up/left by one pixel
	// example: 133.0625 (133 + 1/16) should start from line 134, ceil(133.0625 - 0.05) still above 133

	float4 p = input.p - float4(0.05f, 0.05f, 0, 0);

	output.p = p * VertexScale - VertexOffset;
#if VS_RTCOPY
	output.tp = (p * VertexScale - VertexOffset) * float4(0.5, -0.5, 0, 0) + 0.5;
#endif

	if(VS_LOGZ)
	{
		output.p.z = log2(1.0f + input.p.z) / 32;
	}
	
	if(VS_TME)
	{
		if(VS_FST)
		{
			output.t.xy = input.t * TextureScale;
			output.t.w = 1.0f;
		}
		else
		{
			output.t.xy = input.t;
			output.t.w = input.p.w;
		}
	}
	else
	{
		output.t.xy = 0;
		output.t.w = 1.0f;
	}

	output.c = input.c;
	output.t.z = input.f.a;
	
	return output;
}

float4 ps_main(PS_INPUT input) : COLOR
{
	float4 c = ps_color(input);

	c.a *= 2;

	return c;
}

#endif
#endif
