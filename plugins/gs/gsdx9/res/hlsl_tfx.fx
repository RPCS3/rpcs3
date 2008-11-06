
sampler Texture : register(s0);
sampler1D Palette : register(s1);

float4 Params0 : register(c0);

#define bTCC	(Params0[0] >= 0)
#define fRT		(Params0[1])
#define TA0		(Params0[2])
#define TA1		(Params0[3])

float2 W_H : register(c1);
float2 RW_RH : register(c2);
float2 RW_ZERO : register(c3);
float2 ZERO_RH : register(c4);

//
// texture sampling
//

float4 SampleTexture_32(in float2 Tex : TEXCOORD0) : COLOR
{
	float4 c = tex2D(Texture, Tex);
	c.a *= fRT;
	return c;	
}

float4 SampleTexture_24(in float2 Tex : TEXCOORD0) : COLOR
{
	float4 c = tex2D(Texture, Tex);
	c.a = TA0;
	// c.a *= fRT; // premultiplied
	return c;	
}

float4 SampleTexture_24AEM(in float2 Tex : TEXCOORD0) : COLOR
{
	float4 c = tex2D(Texture, Tex);
	c.a = any(c.rgb) ? TA0 : 0;
	// c.a *= fRT; // premultiplied
	return c;	
}

float4 SampleTexture_16(in float2 Tex : TEXCOORD0) : COLOR
{
	float4 c = tex2D(Texture, Tex);
	c.a = c.a != 0 ? TA1 : TA0;
	// c.a *= fRT; // premultiplied
	return c;	
}

float4 SampleTexture_16AEM(in float2 Tex : TEXCOORD0) : COLOR
{
	float4 c = tex2D(Texture, Tex);
	c.a = c.a != 0 ? TA1 : any(c.rgb) ? TA0 : 0;
	// c.a *= fRT; // premultiplied
	return c;
}

static const float s_palerr = 0.001/256;

float4 SampleTexture_8P_pt(in float2 Tex : TEXCOORD0) : COLOR
{
	float4 c = tex1D(Palette, tex2D(Texture, Tex).x - s_palerr);
	// c.a *= fRT; // premultiplied
	return c;
}
	
float4 SampleTexture_8P_ln(in float2 Tex : TEXCOORD0) : COLOR
{
	Tex -= 0.5*RW_RH; // ?
	float4 c00 = tex1D(Palette, tex2D(Texture, Tex).x - s_palerr);
	float4 c01 = tex1D(Palette, tex2D(Texture, Tex + RW_ZERO).x - s_palerr);
	float4 c10 = tex1D(Palette, tex2D(Texture, Tex + ZERO_RH).x - s_palerr);
	float4 c11 = tex1D(Palette, tex2D(Texture, Tex + RW_RH).x - s_palerr);
	float2 dd = frac(Tex * W_H); 
	float4 c = lerp(lerp(c00, c01, dd.x), lerp(c10, c11, dd.x), dd.y);
	c.a *= fRT;
	return c;
}

float4 SampleTexture_8HP_pt(in float2 Tex : TEXCOORD0) : COLOR
{
	float4 c = tex1D(Palette, tex2D(Texture, Tex).a - s_palerr);
	c.a *= fRT;
	return c;
}
	
float4 SampleTexture_8HP_ln(in float2 Tex : TEXCOORD0) : COLOR
{
	Tex -= 0.5*RW_RH; // ?
	float4 c00 = tex1D(Palette, tex2D(Texture, Tex).a - s_palerr);
	float4 c01 = tex1D(Palette, tex2D(Texture, Tex + RW_ZERO).a - s_palerr);
	float4 c10 = tex1D(Palette, tex2D(Texture, Tex + ZERO_RH).a - s_palerr);
	float4 c11 = tex1D(Palette, tex2D(Texture, Tex + RW_RH).a - s_palerr);
	float2 dd = frac(Tex * W_H); 
	float4 c = lerp(lerp(c00, c01, dd.x), lerp(c10, c11, dd.x), dd.y);
	c.a *= fRT;
	return c;
}

//
// fog
//

float4 ApplyFog(in float4 Diff : COLOR, in float4 Fog : COLOR) : COLOR
{
	Diff = saturate(Diff);
	Diff.rgb = lerp(Fog.rgb, Diff.rgb, Fog.a);	
	return Diff;
}

//
// tfx
//

float4 tfx0(float4 Diff, float4 TexColor) : COLOR
{
	Diff *= 2;
	Diff.rgb *= TexColor.rgb;
	if(bTCC) Diff.a *= TexColor.a;
	return Diff;
}

float4 tfx1(float4 Diff, float4 TexColor) : COLOR
{
	Diff = TexColor;
	return Diff;
}

float4 tfx2(float4 Diff, float4 TexColor) : COLOR
{
	Diff.rgb *= TexColor.rgb * 2;
	Diff.rgb += Diff.a;
	if(bTCC) Diff.a = Diff.a * 2 + TexColor.a;
	return Diff;
}

float4 tfx3(float4 Diff, float4 TexColor) : COLOR
{
	Diff.rgb *= TexColor.rgb * 2;
	Diff.rgb += Diff.a;
	if(bTCC) Diff.a = TexColor.a;
	return Diff;
}

//
// main tfx 32
//

float4 main_tfx0_32(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx0(Diff, SampleTexture_32(Tex)), Fog);
}

float4 main_tfx1_32(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx1(Diff, SampleTexture_32(Tex)), Fog);
}

float4 main_tfx2_32(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx2(Diff, SampleTexture_32(Tex)), Fog);
}

float4 main_tfx3_32(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx3(Diff, SampleTexture_32(Tex)), Fog);
}

//
// main tfx 24
//

float4 main_tfx0_24(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx0(Diff, SampleTexture_24(Tex)), Fog);
}

float4 main_tfx1_24(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx1(Diff, SampleTexture_24(Tex)), Fog);
}

float4 main_tfx2_24(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx2(Diff, SampleTexture_24(Tex)), Fog);
}

float4 main_tfx3_24(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx3(Diff, SampleTexture_24(Tex)), Fog);
}

//
// main tfx 24 AEM
//

float4 main_tfx0_24AEM(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx0(Diff, SampleTexture_24AEM(Tex)), Fog);
}

float4 main_tfx1_24AEM(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx1(Diff, SampleTexture_24AEM(Tex)), Fog);
}

float4 main_tfx2_24AEM(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx2(Diff, SampleTexture_24AEM(Tex)), Fog);
}

float4 main_tfx3_24AEM(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx3(Diff, SampleTexture_24AEM(Tex)), Fog);
}

//
// main tfx 16
//

float4 main_tfx0_16(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx0(Diff, SampleTexture_16(Tex)), Fog);
}

float4 main_tfx1_16(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx1(Diff, SampleTexture_16(Tex)), Fog);
}

float4 main_tfx2_16(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx2(Diff, SampleTexture_16(Tex)), Fog);
}

float4 main_tfx3_16(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx3(Diff, SampleTexture_16(Tex)), Fog);
}

//
// main tfx 16 AEM
//

float4 main_tfx0_16AEM(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx0(Diff, SampleTexture_16AEM(Tex)), Fog);
}

float4 main_tfx1_16AEM(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx1(Diff, SampleTexture_16AEM(Tex)), Fog);
}

float4 main_tfx2_16AEM(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx2(Diff, SampleTexture_16AEM(Tex)), Fog);
}

float4 main_tfx3_16AEM(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx3(Diff, SampleTexture_16AEM(Tex)), Fog);
}

//
// main tfx 8P pt
//

float4 main_tfx0_8P_pt(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx0(Diff, SampleTexture_8P_pt(Tex)), Fog);
}

float4 main_tfx1_8P_pt(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx1(Diff, SampleTexture_8P_pt(Tex)), Fog);
}

float4 main_tfx2_8P_pt(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx2(Diff, SampleTexture_8P_pt(Tex)), Fog);
}

float4 main_tfx3_8P_pt(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx3(Diff, SampleTexture_8P_pt(Tex)), Fog);
}

//
// main tfx 8P ln
//

float4 main_tfx0_8P_ln(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx0(Diff, SampleTexture_8P_ln(Tex)), Fog);
}

float4 main_tfx1_8P_ln(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx1(Diff, SampleTexture_8P_ln(Tex)), Fog);
}

float4 main_tfx2_8P_ln(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx2(Diff, SampleTexture_8P_ln(Tex)), Fog);
}

float4 main_tfx3_8P_ln(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx3(Diff, SampleTexture_8P_ln(Tex)), Fog);
}

//
// main tfx 8HP pt
//

float4 main_tfx0_8HP_pt(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx0(Diff, SampleTexture_8HP_pt(Tex)), Fog);
}

float4 main_tfx1_8HP_pt(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx1(Diff, SampleTexture_8HP_pt(Tex)), Fog);
}

float4 main_tfx2_8HP_pt(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx2(Diff, SampleTexture_8HP_pt(Tex)), Fog);
}

float4 main_tfx3_8HP_pt(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx3(Diff, SampleTexture_8HP_pt(Tex)), Fog);
}

//
// main tfx 8HP ln
//

float4 main_tfx0_8HP_ln(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx0(Diff, SampleTexture_8HP_ln(Tex)), Fog);
}

float4 main_tfx1_8HP_ln(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx1(Diff, SampleTexture_8HP_ln(Tex)), Fog);
}

float4 main_tfx2_8HP_ln(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx2(Diff, SampleTexture_8HP_ln(Tex)), Fog);
}

float4 main_tfx3_8HP_ln(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(tfx3(Diff, SampleTexture_8HP_ln(Tex)), Fog);
}

//
// main notfx
//

float4 main_notfx(float4 Diff : COLOR0, float4 Fog : COLOR1, float2 Tex : TEXCOORD0) : COLOR
{
	return ApplyFog(Diff, Fog);
}

//
// main 8P -> 32
//

float4 main_8PTo32(float4 Diff : COLOR0, float2 Tex : TEXCOORD0) : COLOR
{
	return tex1D(Palette, tex2D(Texture, Tex).x - s_palerr);
}
