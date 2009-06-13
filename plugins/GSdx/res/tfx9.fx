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

#define FogColor	ps_params[0].bgr
#define AREF		ps_params[0].a
#define HalfTexel	ps_params[1]
#define WH			ps_params[2].xy
#define TA0			ps_params[2].z
#define TA1			ps_params[2].w
#define MinMax		ps_params[3]
#define MinMaxF		ps_params[4]

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
#define ATST 4
#define FOG 0
#define CLR1 0
#define RT 0
#define LTF 0
#endif

sampler Texture : register(s0);
sampler1D Palette : register(s1);
sampler1D UMSKFIX : register(s2);
sampler1D VMSKFIX : register(s3);

float4 wrapuv(float4 uv)
{
	if(WMS == WMT)
	{
		if(WMS == 0)
		{
			uv = frac(uv);
		}
		else if(WMS == 1)
		{
			uv = saturate(uv);
		}
		else if(WMS == 2)
		{
			uv = clamp(uv, MinMax.xyxy, MinMax.zwzw);
		}
		else if(WMS == 3)
		{
			uv.x = tex1D(UMSKFIX, uv.x);
			uv.y = tex1D(VMSKFIX, uv.y);
			uv.z = tex1D(UMSKFIX, uv.z);
			uv.w = tex1D(VMSKFIX, uv.w);
		}
	}
	else
	{
		if(WMS == 0)
		{
			uv.xz = frac(uv.xz);
		}
		else if(WMS == 1)
		{
			uv.xz = saturate(uv.xz);
		}
		else if(WMS == 2)
		{
			uv.xz = clamp(uv.xz, MinMax.xx, MinMax.zz);
		}
		else if(WMS == 3)
		{
			uv.x = tex1D(UMSKFIX, uv.x);
			uv.z = tex1D(UMSKFIX, uv.z);
		}

		if(WMT == 0)
		{
			uv.yw = frac(uv.yw);
		}
		else if(WMT == 1)
		{
			uv.yw = saturate(uv.yw);
		}
		else if(WMT == 2)
		{
			uv.yw = clamp(uv.yw, MinMax.yy, MinMax.ww);
		}
		else if(WMT == 3)
		{
			uv.y = tex1D(VMSKFIX, uv.y);
			uv.w = tex1D(VMSKFIX, uv.w);
		}
	}
	
	return uv;
}

float2 clampuv(float2 uv)
{
	if(WMS == 2 && WMT == 2) 
	{
		uv = clamp(uv, MinMaxF.xy, MinMaxF.zw);
	}
	else if(WMS == 2)
	{
		uv.x = clamp(uv.x, MinMaxF.x, MinMaxF.z);
	}
	else if(WMT == 2)
	{
		uv.y = clamp(uv.y, MinMaxF.y, MinMaxF.w);
	}
	
	return uv;
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
		t = tex2D(Texture, tc);
	}
*/
	if(BPP < 3 && WMS < 3 && WMT < 3)
	{
		t = tex2D(Texture, clampuv(tc));
	}
	else
	{
		float4 uv2 = tc.xyxy + HalfTexel;
		float2 dd = frac(uv2.xy * WH); 
		float4 uv = wrapuv(uv2);

		float4 t00, t01, t10, t11;

		if(BPP == 3) // 8HP ln
		{
			float4 a;

			a.x = tex2D(Texture, uv.xy).a;
			a.y = tex2D(Texture, uv.zy).a;
			a.z = tex2D(Texture, uv.xw).a;
			a.w = tex2D(Texture, uv.zw).a;

			if(RT == 1) a *= 0.5;
			
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

		if(LTF)
		{
			t = lerp(lerp(t00, t01, dd.x), lerp(t10, t11, dd.x), dd.y);
		}
		else
		{
			t = t00;
		}
	}
	
	if(BPP == 0) // 32
	{
		if(RT == 1) t.a *= 0.5;
	}
	else if(BPP == 1) // 24
	{
		t.a = AEM == 0 || any(t.rgb) ? TA0 : 0;
	}
	else if(BPP == 2) // 16
	{
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
		c.rgb = lerp(FogColor, c.rgb, f);
	}

	return c;
}

float4 ps_main(PS_INPUT input) : COLOR
{
	float4 t = sample(input.t.xy, input.t.w);

	float4 c = tfx(t, input.c);
	
	atst(c);

	c = fog(c, input.t.z);

	if(CLR1 == 1) // needed for Cd * (As/Ad/F + 1) blending modes
	{
		c.rgb = 1; 
	}

	c.a *= 2;

	return c;
}
