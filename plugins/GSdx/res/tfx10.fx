cbuffer cb0
{
	float4 VertexScale;
	float4 VertexOffset;
	float2 TextureScale;
};

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

#ifndef VS_BPP
#define VS_BPP 0
#define VS_BPPZ 0
#define VS_TME 1
#define VS_FST 1
#define VS_PRIM 0
#endif

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

	float4 p = float4(input.p, input.z, 0);
	
	// FIXME:
	// A litte accuracy problem in many games where the screen is copied in columns and the sides have a half pixel gap for some reason 
	// (probably to avoid page breaks with linear filtering). That half pixel coordinate gets multiplied by 2 (VertexScale) and occasionally 
	// ends on .9999999, which the rasterizer floors to 1 less pixel we need, leaving a visible gap after drawing. 
	
	p += float4(0.5f, 0.5f, 0, 0); // add 1/32 pixel
	
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

#ifndef IIP
#define IIP 0
#define PRIM 3
#endif
	
#if PRIM == 0

[maxvertexcount(1)]
void gs_main(point VS_OUTPUT input[1], inout PointStream<VS_OUTPUT> stream)
{
	stream.Append(input[0]);
}

#elif PRIM == 1

[maxvertexcount(2)]
void gs_main(line VS_OUTPUT input[2], inout LineStream<VS_OUTPUT> stream)
{
	#if IIP == 0
	input[0].c = input[1].c;
	#endif

	stream.Append(input[0]);
	stream.Append(input[1]);
}

#elif PRIM == 2

[maxvertexcount(3)]
void gs_main(triangle VS_OUTPUT input[3], inout TriangleStream<VS_OUTPUT> stream)
{
	#if IIP == 0
	input[0].c = input[2].c;
	input[1].c = input[2].c;
	#endif

	stream.Append(input[0]);
	stream.Append(input[1]);
	stream.Append(input[2]);
}

#elif PRIM == 3

[maxvertexcount(4)]
void gs_main(line VS_OUTPUT input[2], inout TriangleStream<VS_OUTPUT> stream)
{
	input[0].p.z = input[1].p.z;
	input[0].t.zw = input[1].t.zw;

	#if IIP == 0
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

Texture2D<float4> Texture;
Texture2D<float> Palette;
SamplerState TextureSampler;
SamplerState PaletteSampler;

cbuffer cb1
{
	float3 FogColor;
	float AREF;
	float4 HalfTexel;
	float2 WH;
	float TA0;
	float TA1;
	float2 MINUV;
	float2 MAXUV;
	uint2 UVMSK;
	uint2 UVFIX;
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

#ifndef FST
#define FST 0
#define WMS 1
#define WMT 1
#define BPP 0
#define AEM 0
#define TFX 0
#define TCC 1
#define ATE 1
#define ATST 2
#define FOG 1
#define CLR1 0
#define FBA 0
#define AOUT 0
#define LTF 1
#endif

float4 Normalize16(float4 f)
{
	return f / float4(0x001f, 0x03e0, 0x7c00, 0x8000);
}

float4 Extract16(uint i)
{
	float4 f;

	f.r = i & 0x001f;
	f.g = i & 0x03e0;
	f.b = i & 0x7c00;
	f.a = i & 0x8000;

	return f;
}

int2 wrapu(float2 f, int2 i)
{
	if(WMS == 0)
	{
		i = frac(f) * WH.xx;
	}
	else if(WMS == 1)
	{
		i = saturate(f) * WH.xx;
	}
	else if(WMS == 2)
	{
		i = clamp(f, MINUV.xx, MAXUV.xx) * WH.xx;
	}
	else if(WMS == 3)
	{
		i = (i & UVMSK.xx) | UVFIX.xx;
	}
	
	return i;
}

int2 wrapv(float2 f, int2 i)
{
	if(WMT == 0)
	{
		i = frac(f) * WH.yy;
	}
	else if(WMT == 1)
	{
		i = saturate(f) * WH.yy;
	}
	else if(WMT == 2)
	{
		i = clamp(f, MINUV.yy, MAXUV.yy) * WH.yy;
	}
	else if(WMT == 3)
	{
		i = (i & UVMSK.yy) | UVFIX.yy;
	}
	
	return i;
}

int4 wrapuv(float4 f, int4 i)
{
	if(WMT == 0)
	{
		i = frac(f) * WH.xyxy;
	}
	else if(WMT == 1)
	{
		i = saturate(f) * WH.xyxy;
	}
	else if(WMT == 2)
	{
		i = clamp(f, MINUV.xyxy, MAXUV.xyxy) * WH.xyxy;
	}
	else if(WMT == 3)
	{
		i = (i & UVMSK.xyxy) | UVFIX.xyxy;
	}

	return i;
}

int4 wrap(float4 uv, int4 iuv)
{
	if(WMS == WMT)
	{
		iuv = wrapuv(uv, iuv);
	}
	else
	{
		iuv.xz = wrapu(uv.xz, iuv.xz);
		iuv.yw = wrapv(uv.yw, iuv.yw);
	}
	
	return iuv;
}

float4 sample(float2 tc, float w)
{
	if(FST == 0)
	{
		tc /= w;
	}
	
	float4 t;
/*	
	if(BPP < 3 && WMS < 2 && WMT < 2)
	{
		t = Texture.Sample(TextureSampler, tc);
	}
*/
	if(BPP < 3 && WMS < 3 && WMT < 3)
	{
		if(WMS == 2 && WMT == 2) tc = clamp(tc, MINUV.xy, MAXUV.xy);
		else if(WMS == 2) tc.x = clamp(tc.x, MINUV.x, MAXUV.x);
		else if(WMT == 2) tc.y = clamp(tc.y, MINUV.y, MAXUV.y);
	
		t = Texture.Sample(TextureSampler, tc);
	}
	else
	{
		float4 uv = tc.xyxy + HalfTexel;
		float4 uv2 = uv * WH.xyxy;
		float2 dd = frac(uv2.xy); 
		
		int4 iuv = wrap(uv, uv2);

		float4 t00, t01, t10, t11;

		if(BPP == 3) // 8HP + 32-bit palette
		{
			float4 a;

			a.x = Texture.Load(int3(iuv.xy, 0)).a;
			a.y = Texture.Load(int3(iuv.zy, 0)).a;
			a.z = Texture.Load(int3(iuv.xw, 0)).a;
			a.w = Texture.Load(int3(iuv.zw, 0)).a;

			t00 = Palette.Load(a.x);
			t01 = Palette.Load(a.y);
			t10 = Palette.Load(a.z);
			t11 = Palette.Load(a.w);
		}
		else if(BPP == 4) // 8HP + 16-bit palette
		{
			// TODO: yuck, just pre-convert the palette to 32-bit
		}
		else if(BPP == 5) // 16P
		{
			float4 r;

			r.x = Texture.Load(int3(iuv.xy, 0)).r;
			r.y = Texture.Load(int3(iuv.zy, 0)).r;
			r.z = Texture.Load(int3(iuv.xw, 0)).r;
			r.w = Texture.Load(int3(iuv.zw, 0)).r;
			
			uint4 i = r * 65535;

			t00 = Extract16(i.x);
			t01 = Extract16(i.y);
			t10 = Extract16(i.z);
			t11 = Extract16(i.w);
		}
		else
		{
			t00 = Texture.Load(int3(iuv.xy, 0));
			t01 = Texture.Load(int3(iuv.zy, 0));
			t10 = Texture.Load(int3(iuv.xw, 0));
			t11 = Texture.Load(int3(iuv.zw, 0));
		}

		if(LTF)
		{
			t = lerp(lerp(t00, t01, dd.x), lerp(t10, t11, dd.x), dd.y);
		}
		else
		{
			t = t00;
		}
	}

	if(BPP == 1) // 24
	{
		t.a = AEM == 0 || any(t.rgb) ? TA0 : 0;
	}
	else if(BPP == 2 || BPP == 5) // 16 || 16P
	{
		if(BPP == 5)
		{
			t = Normalize16(t);
		}
		
		// a bit incompatible with up-scaling because the 1 bit alpha is interpolated
		
		t.a = t.a >= 0.5 ? TA1 : AEM == 0 || any(t.rgb) ? TA0 : 0;
	}
	
	return t;
}

float4 tfx(float4 t, float4 c)
{
	if(TFX == 0)
	{
		if(TCC == 0) 
		{
			c.rgb = c.rgb * t.rgb * 255.0f / 128;
		}
		else
		{
			c = c * t * 255.0f / 128;
		}
	}
	else if(TFX == 1)
	{
		if(TCC == 0) 
		{
			c.rgb = t.rgb;
		}
		else
		{
			c = t;
		}
	}
	else if(TFX == 2)
	{
		c.rgb = c.rgb * t.rgb * 255.0f / 128 + c.a;

		if(TCC == 1) 
		{
			c.a += t.a;
		}
	}
	else if(TFX == 3)
	{
		c.rgb = c.rgb * t.rgb * 255.0f / 128 + c.a;

		if(TCC == 1) 
		{
			c.a = t.a;
		}
	}
	
	return saturate(c);
}

void atst(float4 c)
{
	if(ATE == 1)
	{
		if(ATST == 0)
		{
			discard;
		}
		else if(ATST == 2 || ATST == 3) // l, le
		{
			clip(AREF - c.a);
		}
		else if(ATST == 4) // e
		{
			clip(0.6f / 255 - abs(c.a - AREF)); // FIXME: 0.5f is too small
		}
		else if(ATST == 5 || ATST == 6) // ge, g
		{
			clip(c.a - AREF);
		}
		else if(ATST == 7) // ne
		{
			clip(abs(c.a - AREF) - 0.4f / 255); // FIXME: 0.5f is too much
		}
	}
}

float4 fog(float4 c, float f)
{
	if(FOG == 1)
	{
		c.rgb = lerp(FogColor.rgb, c.rgb, f);
	}

	return c;
}

PS_OUTPUT ps_main(PS_INPUT input)
{
	float4 t = sample(input.t.xy, input.t.w);

	float4 c = tfx(t, input.c);

	atst(c);

	c = fog(c, input.t.z);

	if(CLR1 == 1) // needed for Cd * (As/Ad/F + 1) blending modes
	{
		c.rgb = 1; 
	}

	PS_OUTPUT output;

	output.c1 = c.a * 2; // used for alpha blending

	if(AOUT == 1) // 16 bit output
	{
		float a = 128.0f / 255; // alpha output will be 0x80
		
		c.a = FBA == 1 ? a : step(0.5, c.a) * a;
	}
	else if(FBA == 1)
	{
		if(c.a < 0.5) c.a += 0.5;
	}

	output.c0 = c;

	return output;
}
