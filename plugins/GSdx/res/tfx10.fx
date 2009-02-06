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

Texture2D Texture;
Texture2D Palette;
SamplerState TextureSampler;
SamplerState PaletteSampler;

cbuffer cb1
{
	float4 FogColor;
	float MINU;
	float MAXU;
	float MINV;
	float MAXV;
	uint UMSK;
	uint UFIX;
	uint VMSK;
	uint VFIX;
	float TA0;
	float TA1;
	float AREF;
	float _pad;
	float2 WH;
	float2 rWrH;
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
#define WMS 3
#define WMT 3
#define BPP 0
#define AEM 0
#define TFX 0
#define TCC 1
#define ATE 0
#define ATST 2
#define FOG 0
#define CLR1 0
#define FBA 0
#define AOUT 0
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

int repeatu(int tc)
{
	return WMS == 3 ? ((tc & UMSK) | UFIX) : tc;
}

int repeatv(int tc)
{
	return WMT == 3 ? ((tc & VMSK) | VFIX) : tc;
}

float4 sample(float2 tc)
{
	float4 t;
	
	// if(WMS >= 2 || WMT >= 2)
	if(WMS >= 3 || WMT >= 3)
	{
		int4 itc = tc.xyxy * WH.xyxy;
		
		float4 tc01;
		
		tc01.x = repeatu(itc.x);
		tc01.y = repeatv(itc.y);
		tc01.z = repeatu(itc.z + 1);
		tc01.w = repeatv(itc.w + 1);
		
		tc01 *= rWrH.xyxy;

		float4 t00 = Texture.Sample(TextureSampler, tc01.xy);
		float4 t01 = Texture.Sample(TextureSampler, tc01.zy);
		float4 t10 = Texture.Sample(TextureSampler, tc01.xw);
		float4 t11 = Texture.Sample(TextureSampler, tc01.zw);

		float2 dd = frac(tc * WH); 

		t = lerp(lerp(t00, t01, dd.x), lerp(t10, t11, dd.x), dd.y);
	}
	else
	{
		t = Texture.Sample(TextureSampler, tc);
	}
	
	return t;
}

float4 sample8hp(float2 tc)
{
	float4 tc01;
	
	// if(WMS >= 2 || WMT >= 2)
	if(WMS >= 3 || WMT >= 3)
	{
		int4 itc = tc.xyxy * WH.xyxy;
		
		tc01.x = repeatu(itc.x);
		tc01.y = repeatv(itc.y);
		tc01.z = repeatu(itc.z + 1);
		tc01.w = repeatv(itc.w + 1);

		tc01 *= rWrH.xyxy;
	}
	else
	{
		tc01.x = tc.x;
		tc01.y = tc.y;
		tc01.z = tc.x + rWrH.x; 
		tc01.w = tc.y + rWrH.y;
	}

	float4 t;
	
	t.x = Texture.Sample(TextureSampler, tc01.xy).a;
	t.y = Texture.Sample(TextureSampler, tc01.zy).a;
	t.z = Texture.Sample(TextureSampler, tc01.xw).a;
	t.w = Texture.Sample(TextureSampler, tc01.zw).a;
	
	float4 t00 = Palette.Sample(PaletteSampler, t.x);
	float4 t01 = Palette.Sample(PaletteSampler, t.y);
	float4 t10 = Palette.Sample(PaletteSampler, t.z);
	float4 t11 = Palette.Sample(PaletteSampler, t.w);

	float2 dd = frac(tc * WH); 

	return lerp(lerp(t00, t01, dd.x), lerp(t10, t11, dd.x), dd.y);
}

float4 sample16p(float2 tc)
{
	float4 t;
	
	float4 tc01;
	
	// if(WMS >= 2 || WMT >= 2)
	if(WMS >= 3 || WMT >= 3)
	{
		int4 itc = tc.xyxy * WH.xyxy;
		
		tc01.x = repeatu(itc.x);
		tc01.y = repeatv(itc.y);
		tc01.z = repeatu(itc.z + 1);
		tc01.w = repeatv(itc.w + 1);

		tc01 *= rWrH.xyxy;
	}
	else
	{
		tc01.x = tc.x;
		tc01.y = tc.y;
		tc01.z = tc.x + rWrH.x; 
		tc01.w = tc.y + rWrH.y;
	}

	t.x = Texture.Sample(TextureSampler, tc01.xy).r;
	t.y = Texture.Sample(TextureSampler, tc01.zy).r;
	t.z = Texture.Sample(TextureSampler, tc01.xw).r;
	t.w = Texture.Sample(TextureSampler, tc01.zw).r;
			
	uint4 i = t * 65535;

	float4 t00 = Extract16(i.x);
	float4 t01 = Extract16(i.y);
	float4 t10 = Extract16(i.z);
	float4 t11 = Extract16(i.w);

	float2 dd = frac(tc * WH); 

	return Normalize16(lerp(lerp(t00, t01, dd.x), lerp(t10, t11, dd.x), dd.y));
}

PS_OUTPUT ps_main(PS_INPUT input)
{
	float2 tc = input.t.xy;

	if(FST == 0)
	{
		tc /= input.t.w;
	}
	
	tc -= rWrH / 2;
	
	if(WMS == 2)
	{
		tc.x = clamp(tc.x, MINU, MAXU);
	}

	if(WMT == 2)
	{
		tc.y = clamp(tc.y, MINV, MAXV);
	}

	float4 t;

	if(BPP == 0) // 32
	{
		t = sample(tc);
	}
	else if(BPP == 1) // 24
	{
		t = sample(tc);

		t.a = AEM == 0 || any(t.rgb) ? TA0 : 0;
	}
	else if(BPP == 2) // 16
	{
		t = sample(tc);

		t.a = t.a >= 0.5 ? TA1 : AEM == 0 || any(t.rgb) ? TA0 : 0; // a bit incompatible with up-scaling because the 1 bit alpha is interpolated
	}
	else if(BPP == 3) // 8HP / 32-bit palette
	{
		t = sample8hp(tc);
	}
	else if(BPP == 4) // 8HP / 16-bit palette
	{
		// TODO: yuck, just pre-convert the palette to 32-bit
	}
	else if(BPP == 5) // 16P
	{
		t = sample16p(tc);

		t.a = t.a >= 0.5 ? TA1 : AEM == 0 || any(t.rgb) ? TA0 : 0; // a bit incompatible with up-scaling because the 1 bit alpha is interpolated
	}

	float4 c = input.c;

	if(TFX == 0)
	{
		if(TCC == 0) 
		{
			c.rgb = c.rgb * t.rgb * 2;
		}
		else
		{
			c = c * t * 2;
		}
	}
	else if(TFX == 1)
	{
		c = t;
	}
	else if(TFX == 2)
	{
		c.rgb = c.rgb * t.rgb * 2 + c.a;
		
		if(TCC == 1) 
		{
			c.a += t.a;
		}
	}
	else if(TFX == 3)
	{
		c.rgb = c.rgb * t.rgb * 2 + c.a;
		
		if(TCC == 1) 
		{
			c.a = t.a;
		}
	}

	c = saturate(c);

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
			clip(0.9f/256 - abs(c.a - AREF));
		}
		else if(ATST == 5 || ATST == 6) // ge, g
		{
			clip(c.a - AREF);
		}
		else if(ATST == 7) // ne
		{
			clip(abs(c.a - AREF) - 0.9f/256);
		}
	}

	if(FOG == 1)
	{
		c.rgb = lerp(FogColor.rgb, c.rgb, input.t.z);
	}

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
