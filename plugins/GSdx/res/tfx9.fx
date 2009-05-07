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

float4 vs_params[3];

#define VertexScale vs_params[0]
#define VertexOffset vs_params[1]
#define TextureScale vs_params[2].xy

#ifndef VS_BPPZ
#define VS_BPPZ 0
#define VS_TME 1
#define VS_FST 1
#define VS_LOGZ 1
#endif

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

	output.p = input.p * VertexScale - VertexOffset;

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

float4 ps_params[5];

#define FogColor	ps_params[0].bgra
#define MINU		ps_params[1].x
#define MAXU		ps_params[1].y
#define MINV		ps_params[1].z
#define MAXV		ps_params[1].w
#define UMSK		ps_params[2].x
#define UFIX		ps_params[2].y
#define VMSK		ps_params[2].z
#define VFIX		ps_params[2].w
#define TA0			ps_params[3].x
#define TA1			ps_params[3].y
#define AREF		ps_params[3].z
#define WH			ps_params[4].xy
#define rWrH		ps_params[4].zw

struct PS_INPUT
{
	float4 t : TEXCOORD0;
	float4 c : COLOR0;
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
#define ATST 0
#define FOG 0
#define CLR1 0
#define RT 0
#endif

sampler Texture : register(s0);
sampler1D Palette : register(s1);
sampler1D UMSKFIX : register(s2);
sampler1D VMSKFIX : register(s3);

float repeatu(float tc)
{
	return WMS == 3 ? tex1D(UMSKFIX, tc) : tc;
}

float repeatv(float tc)
{
	return WMT == 3 ? tex1D(VMSKFIX, tc) : tc;
}

float4 sample(float2 tc)
{
	float4 t;
	
	// if(WMS >= 2 || WMT >= 2)
	if(WMS >= 3 || WMT >= 3)
	{
		tc -= rWrH / 2;	

		int4 itc = tc.xyxy * WH.xyxy;
		
		float4 tc01;
		
		tc01.x = repeatu(itc.x);
		tc01.y = repeatv(itc.y);
		tc01.z = repeatu(itc.z + 1);
		tc01.w = repeatv(itc.w + 1);
	
		tc01 *= rWrH.xyxy;

		float4 t00 = tex2D(Texture, tc01.xy);
		float4 t01 = tex2D(Texture, tc01.zy);
		float4 t10 = tex2D(Texture, tc01.xw);
		float4 t11 = tex2D(Texture, tc01.zw);
	
		float2 dd = frac(tc * WH); 

		t = lerp(lerp(t00, t01, dd.x), lerp(t10, t11, dd.x), dd.y);
	}
	else
	{
		t = tex2D(Texture, tc);
	}
	
	return t;
}

float4 sample8hp(float2 tc)
{
	tc -= rWrH / 2;	

	float4 tc01;
	
	tc01.x = tc.x;
	tc01.y = tc.y;
	tc01.z = tc.x + rWrH.x; 
	tc01.w = tc.y + rWrH.y;

	float4 t;

	t.x = tex2D(Texture, tc01.xy).a;
	t.y = tex2D(Texture, tc01.zy).a;
	t.z = tex2D(Texture, tc01.xw).a;
	t.w = tex2D(Texture, tc01.zw).a;

	if(RT == 1) t *= 0.5;
	
	float4 t00 = tex1D(Palette, t.x);
	float4 t01 = tex1D(Palette, t.y);
	float4 t10 = tex1D(Palette, t.z);
	float4 t11 = tex1D(Palette, t.w);

	float2 dd = frac(tc * WH); 

	return lerp(lerp(t00, t01, dd.x), lerp(t10, t11, dd.x), dd.y);
}

float4 ps_main(PS_INPUT input) : COLOR
{
	float2 tc = input.t.xy;

	if(FST == 0)
	{
		tc /= input.t.w;
	}

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

		if(RT == 1) t.a *= 0.5;
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
	else if(BPP == 3) // 8HP ln
	{
		t = sample8hp(tc);
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
			clip(0.6f / 256 - abs(c.a - AREF)); // FIXME: 0.5f is too small
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

	if(FOG == 1)
	{
		c.rgb = lerp(FogColor.rgb, c.rgb, input.t.z);
	}

	if(CLR1 == 1) // needed for Cd * (As/Ad/F + 1) blending modes
	{
		c.rgb = 1; 
	}

	c.a *= 2;

	return c;
}
