#if SHADER_MODEL >= 0x400

#ifndef VS_BPPZ
#define VS_BPPZ 0
#define VS_TME 1
#define VS_FST 1
#define VS_PRIM 0
#endif

#ifndef GS_IIP
#define GS_IIP 0
#define GS_PRIM 3
#endif

#ifndef PS_FST
#define PS_FST 0
#define PS_WMS 3
#define PS_WMT 3
#define PS_BPP 0
#define PS_AEM 0
#define PS_TFX 0
#define PS_TCC 1
#define PS_ATE 0
#define PS_ATST 4
#define PS_FOG 0
#define PS_CLR1 0
#define PS_FBA 0
#define PS_AOUT 0
#define PS_LTF 1
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
	float4 c : COLOR0;
};

struct PS_INPUT
{
	float4 p : SV_Position;
	float4 t : TEXCOORD0;
	float4 c : COLOR0;
};

struct PS_OUTPUT
{
	float4 c0 : SV_Target0;
	float4 c1 : SV_Target1;
};

Texture2D<float4> Texture;
Texture2D<float> Palette;
SamplerState TextureSampler;
SamplerState PaletteSampler;

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
	float2 WH;
	float2 TA;
	float4 MinMax;
	float4 MinMaxF;
	uint4 MskFix;
};

#elif SHADER_MODEL <= 0x300

#ifndef VS_BPPZ
#define VS_BPPZ 0
#define VS_TME 1
#define VS_FST 1
#define VS_LOGZ 1
#endif

#ifndef PS_FST
#define PS_FST 0
#define PS_WMS 3
#define PS_WMT 3
#define PS_BPP 0
#define PS_AEM 0
#define PS_TFX 0
#define PS_TCC 1
#define PS_ATE 0
#define PS_ATST 4
#define PS_FOG 0
#define PS_CLR1 0
#define PS_RT 0
#define PS_LTF 0
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
	float4 c : COLOR0;
};

struct PS_INPUT
{
	float4 t : TEXCOORD0;
	float4 c : COLOR0;
};

sampler Texture : register(s0);
sampler1D Palette : register(s1);
sampler1D UMSKFIX : register(s2);
sampler1D VMSKFIX : register(s3);

float4 vs_params[3];

#define VertexScale vs_params[0]
#define VertexOffset vs_params[1]
#define TextureScale vs_params[2].xy

float4 ps_params[5];

#define FogColor	ps_params[0].bgr
#define AREF		ps_params[0].a
#define HalfTexel	ps_params[1]
#define WH			ps_params[2].xy
#define TA0			ps_params[2].z
#define TA1			ps_params[2].w
#define MinMax		ps_params[3]
#define MinMaxF		ps_params[4]

#endif

float4 wrapuv(float4 uv)
{
	if(PS_WMS == PS_WMT)
	{
		if(PS_WMS == 0)
		{
			uv = frac(uv);
		}
		else if(PS_WMS == 1)
		{
			uv = saturate(uv);
		}
		else if(PS_WMS == 2)
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
		if(PS_WMS == 0)
		{
			uv.xz = frac(uv.xz);
		}
		else if(PS_WMS == 1)
		{
			uv.xz = saturate(uv.xz);
		}
		else if(PS_WMS == 2)
		{
			uv.xz = clamp(uv.xz, MinMax.xx, MinMax.zz);
		}
		else if(PS_WMS == 3)
		{
			#if SHADER_MODEL >= 0x400
			uv.xz = (float2)(((int2)(uv * WH.xyxy).xz & MskFix.xx) | MskFix.zz) / WH;
			#elif SHADER_MODEL <= 0x300
			uv.x = tex1D(UMSKFIX, uv.x);
			uv.z = tex1D(UMSKFIX, uv.z);
			#endif
		}

		if(PS_WMT == 0)
		{
			uv.yw = frac(uv.yw);
		}
		else if(PS_WMT == 1)
		{
			uv.yw = saturate(uv.yw);
		}
		else if(PS_WMT == 2)
		{
			uv.yw = clamp(uv.yw, MinMax.yy, MinMax.ww);
		}
		else if(PS_WMT == 3)
		{
			#if SHADER_MODEL >= 0x400
			uv.yw = (float2)(((int2)(uv * WH.xyxy).yw & MskFix.yy) | MskFix.ww) / WH;
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
		uv = clamp(uv, MinMaxF.xy, MinMaxF.zw);
	}
	else if(PS_WMS == 2)
	{
		uv.x = clamp(uv.x, MinMaxF.x, MinMaxF.z);
	}
	else if(PS_WMT == 2)
	{
		uv.y = clamp(uv.y, MinMaxF.y, MinMaxF.w);
	}
	
	return uv;
}

float4 tfx(float4 t, float4 c)
{
	if(PS_TFX == 0)
	{
		if(PS_TCC == 0) 
		{
			c.rgb = c.rgb * t.rgb * 255.0f / 128;
		}
		else
		{
			c = c * t * 255.0f / 128;
		}
	}
	else if(PS_TFX == 1)
	{
		if(PS_TCC == 0) 
		{
			c.rgb = t.rgb;
		}
		else
		{
			c = t;
		}
	}
	else if(PS_TFX == 2)
	{
		c.rgb = c.rgb * t.rgb * 255.0f / 128 + c.a;

		if(PS_TCC == 1) 
		{
			c.a += t.a;
		}
	}
	else if(PS_TFX == 3)
	{
		c.rgb = c.rgb * t.rgb * 255.0f / 128 + c.a;

		if(PS_TCC == 1) 
		{
			c.a = t.a;
		}
	}
	
	return saturate(c);
}

void atst(float4 c)
{
	if(PS_ATE == 1)
	{
		if(PS_ATST == 0)
		{
			discard;
		}
		else if(PS_ATST == 2 || PS_ATST == 3) // l, le
		{
			clip(AREF - c.a);
		}
		else if(PS_ATST == 4) // e
		{
			clip(0.6f / 255 - abs(c.a - AREF)); // FIXME: 0.5f is too small
		}
		else if(PS_ATST == 5 || PS_ATST == 6) // ge, g
		{
			clip(c.a - AREF);
		}
		else if(PS_ATST == 7) // ne
		{
			clip(abs(c.a - AREF) - 0.4f / 255); // FIXME: 0.5f is too much
		}
	}
}

float4 fog(float4 c, float f)
{
	if(PS_FOG == 1)
	{
		c.rgb = lerp(FogColor, c.rgb, f);
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

	if(VS_PRIM == 6) // sprite
	{
		//input.p.xy = (input.p.xy + 15) & ~15; // HACK
	}

	VS_OUTPUT output;
	
	// pos -= 0.05 (1/320 pixel) helps avoiding rounding problems (integral part of pos is usually 5 digits, 0.05 is about as low as we can go)
	// example: ceil(afterseveralvertextransformations(y = 133)) => 134 => line 133 stays empty
	// input granularity is 1/16 pixel, anything smaller than that won't step drawing up/left by one pixel
	// example: 133.0625 (133 + 1/16) should start from line 134, ceil(133.0625 - 0.05) still above 133
	
	float4 p = float4(input.p, input.z, 0) - float4(0.05f, 0.05f, 0, 0); 

	output.p = p * VertexScale - VertexOffset;
	
	if(VS_TME == 1)
	{
		if(VS_FST == 1)
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

float4 sample(float2 tc, float w)
{
	if(PS_FST == 0)
	{
		tc /= w;
	}
	
	float4 t;
/*	
	if(PS_BPP < 3 && PS_WMS < 2 && PS_WMT < 2)
	{
		t = Texture.Sample(TextureSampler, tc);
	}
*/
	if(PS_BPP < 3 && PS_WMS < 3 && PS_WMT < 3)
	{
		t = Texture.Sample(TextureSampler, clampuv(tc));
	}
	else
	{
		float w, h;
		Texture.GetDimensions(w, h);
		
		float4 uv2 = tc.xyxy + HalfTexel;
		float2 dd = frac(uv2.xy * float2(w, h)); 
		float4 uv = wrapuv(uv2);

		float4 t00, t01, t10, t11;

		if(PS_BPP == 3) // 8HP
		{
			float4 a;

			a.x = Texture.Sample(TextureSampler, uv.xy).a;
			a.y = Texture.Sample(TextureSampler, uv.zy).a;
			a.z = Texture.Sample(TextureSampler, uv.xw).a;
			a.w = Texture.Sample(TextureSampler, uv.zw).a;

			t00 = Palette.Sample(PaletteSampler, a.x);
			t01 = Palette.Sample(PaletteSampler, a.y);
			t10 = Palette.Sample(PaletteSampler, a.z);
			t11 = Palette.Sample(PaletteSampler, a.w);
		}
		else
		{
			t00 = Texture.Sample(TextureSampler, uv.xy);
			t01 = Texture.Sample(TextureSampler, uv.zy);
			t10 = Texture.Sample(TextureSampler, uv.xw);
			t11 = Texture.Sample(TextureSampler, uv.zw);
		}

		if(PS_LTF)
		{
			t = lerp(lerp(t00, t01, dd.x), lerp(t10, t11, dd.x), dd.y);
		}
		else
		{
			t = t00;
		}
	}

	if(PS_BPP == 1) // 24
	{
		t.a = PS_AEM == 0 || any(t.rgb) ? TA.x : 0;
	}
	else if(PS_BPP == 2) // 16
	{
		// a bit incompatible with up-scaling because the 1 bit alpha is interpolated
		
		t.a = t.a >= 0.5 ? TA.y : PS_AEM == 0 || any(t.rgb) ? TA.x : 0;
	}
	
	return t;
}

PS_OUTPUT ps_main(PS_INPUT input)
{
	float4 t = sample(input.t.xy, input.t.w);

	float4 c = tfx(t, input.c);

	atst(c);

	c = fog(c, input.t.z);

	if(PS_CLR1 == 1) // needed for Cd * (As/Ad/F + 1) blending modes
	{
		c.rgb = 1; 
	}

	PS_OUTPUT output;

	output.c1 = c.a * 2; // used for alpha blending

	if(PS_AOUT == 1) // 16 bit output
	{
		float a = 128.0f / 255; // alpha output will be 0x80
		
		c.a = PS_FBA == 1 ? a : step(0.5, c.a) * a;
	}
	else if(PS_FBA == 1)
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

	if(VS_LOGZ == 1)
	{
		output.p.z = log2(1.0f + input.p.z) / 32;
	}
	
	if(VS_TME == 1)
	{
		if(VS_FST == 1)
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

float4 sample(float2 tc, float w)
{
	if(PS_FST == 0)
	{
		tc /= w;
	}
	
	float4 t;
/*	
	if(PS_BPP < 3 && PS_WMS < 2 && PS_WMT < 2)
	{
		t = tex2D(Texture, tc);
	}
*/
	if(PS_BPP < 3 && PS_WMS < 3 && PS_WMT < 3)
	{
		t = tex2D(Texture, clampuv(tc));
	}
	else
	{
		float4 uv2 = tc.xyxy + HalfTexel;
		float2 dd = frac(uv2.xy * WH); 
		float4 uv = wrapuv(uv2);

		float4 t00, t01, t10, t11;

		if(PS_BPP == 3) // 8HP
		{
			float4 a;

			a.x = tex2D(Texture, uv.xy).a;
			a.y = tex2D(Texture, uv.zy).a;
			a.z = tex2D(Texture, uv.xw).a;
			a.w = tex2D(Texture, uv.zw).a;

			if(PS_RT == 1) a *= 0.5;
			
			t00 = tex1D(Palette, a.x);
			t01 = tex1D(Palette, a.y);
			t10 = tex1D(Palette, a.z);
			t11 = tex1D(Palette, a.w);
		}
		else
		{
			t00 = tex2D(Texture, uv.xy);
			t01 = tex2D(Texture, uv.zy);
			t10 = tex2D(Texture, uv.xw);
			t11 = tex2D(Texture, uv.zw);
		}

		if(PS_LTF)
		{
			t = lerp(lerp(t00, t01, dd.x), lerp(t10, t11, dd.x), dd.y);
		}
		else
		{
			t = t00;
		}
	}
	
	if(PS_BPP == 0) // 32
	{
		if(PS_RT == 1) t.a *= 0.5;
	}
	else if(PS_BPP == 1) // 24
	{
		t.a = PS_AEM == 0 || any(t.rgb) ? TA0 : 0;
	}
	else if(PS_BPP == 2) // 16
	{
		// a bit incompatible with up-scaling because the 1 bit alpha is interpolated
	
		t.a = t.a >= 0.5 ? TA1 : PS_AEM == 0 || any(t.rgb) ? TA0 : 0; 
	}
	
	return t;
}

float4 ps_main(PS_INPUT input) : COLOR
{
	float4 t = sample(input.t.xy, input.t.w);

	float4 c = tfx(t, input.c);
	
	atst(c);

	c = fog(c, input.t.z);

	if(PS_CLR1 == 1) // needed for Cd * (As/Ad/F + 1) blending modes
	{
		c.rgb = 1; 
	}

	c.a *= 2;

	return c;
}

#endif