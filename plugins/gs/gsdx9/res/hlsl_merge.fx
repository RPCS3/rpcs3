
sampler RAO1 : register(s0);
sampler RAO2 : register(s1);

float4 Params1 : register(c0);

#define BGColor	(Params1.rgb)
#define ALP		(Params1.a)

float4 Params2 : register(c1);

#define AMOD	(Params2[0] >= 0)
#define EN1		(Params2[1])
#define EN2		(Params2[2])
#define MMOD	(Params2[3] >= 0)

float4 Params3 : register(c2);

#define AEM		(Params3[0])
#define TA0		(Params3[1])
#define TA1		(Params3[2])
#define SLBG	(Params3[3] >= 0)

float4 Merge(float4 Color1 : COLOR, float4 Color2 : COLOR) : COLOR
{
	float a = EN1 * (MMOD ? ALP : Color1.a);
	float4 c = lerp(Color2, Color1, a);
	// c.a = AMOD ? Color2.a : Color1.a; // not used
	c.rgba = c.bgra;
	return c;
}

// 16

float4 CorrectTexColor16(float4 TexColor : COLOR)
{
	float A = AEM * !any(TexColor.rgb) * TA0;
	TexColor.a = (TexColor.a < 1.0) ? A : TA1; // < 0.5 ?
	return TexColor;
}

float4 Sample16RAO1(float2 Tex : TEXCOORD0) : COLOR
{
	return EN1 * CorrectTexColor16(tex2D(RAO1, Tex));
}

float4 Sample16RAO2(float2 Tex : TEXCOORD0) : COLOR
{
	return SLBG ? float4(BGColor, 0) : (EN2 * CorrectTexColor16(tex2D(RAO2, Tex)));
}

float4 main0(float2 Tex1 : TEXCOORD0, float2 Tex2 : TEXCOORD1) : COLOR
{
	float4 Color1 = Sample16RAO1(Tex1);
	float4 Color2 = Sample16RAO2(Tex2);
	return Merge(Color1, Color2);
}

// 24

float4 CorrectTexColor24(float4 TexColor : COLOR)
{
	TexColor.a = AEM * !any(TexColor.rgb) * TA0;
	return TexColor;
}

float4 Sample24RAO1(float2 Tex : TEXCOORD0) : COLOR
{
	return EN1 * CorrectTexColor24(tex2D(RAO1, Tex));
}

float4 Sample24RAO2(float2 Tex : TEXCOORD0) : COLOR
{
	return SLBG ? float4(BGColor, 0) : (EN2 * CorrectTexColor24(tex2D(RAO2, Tex)));
}

float4 main1(float2 Tex1 : TEXCOORD0, float2 Tex2 : TEXCOORD1) : COLOR
{
	float4 Color1 = Sample24RAO1(Tex1);
	float4 Color2 = Sample24RAO2(Tex2);
	return Merge(Color1, Color2);
}

// 32

float4 SampleRAO1(float2 Tex : TEXCOORD0) : COLOR
{
	return EN1 * tex2D(RAO1, Tex);
}

float4 SampleRAO2(float2 Tex : TEXCOORD0) : COLOR
{
	return SLBG ? float4(BGColor, 0) : (EN2 * tex2D(RAO2, Tex));
}

float4 main2(float2 Tex1 : TEXCOORD0, float2 Tex2 : TEXCOORD1) : COLOR
{
	float4 Color1 = SampleRAO1(Tex1);
	float4 Color2 = SampleRAO2(Tex2);
	return Merge(Color1, Color2);
}
