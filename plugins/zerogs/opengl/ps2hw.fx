// Cg Shaders for PS2 GS emulation

// divides by z for every pixel, instead of in vertex shader
// fixes kh textures
#define PERSPECTIVE_CORRECT_TEX

//#define TEST_AEM // tests AEM for black pixels
//#define REGION_REPEAT // set if texture wrapping mode is region repeat
//#define WRITE_DEPTH // set if depth is also written in a MRT
//#define ACCURATE_DECOMPRESSION // set for less capable hardware ATI Radeon 9000 series
//#define EXACT_COLOR	// make sure the output color is clamped to 1/255 boundaries (for alpha testing)

#ifdef PERSPECTIVE_CORRECT_TEX
#define TEX_XY tex.xy/tex.z
#define TEX_DECL float3
#else
#define TEX_XY tex.xy
#define TEX_DECL float2
#endif

#ifdef WRITE_DEPTH
#define DOZWRITE(x) x
#else
#define DOZWRITE(x)
#endif

#include "ps2hw_ctx.fx"

// used to get the tiled offset into a page given the linear offset
uniform samplerRECT g_sSrcFinal : register(s2);
uniform sampler2D g_sBlocks : register(s3);
uniform sampler2D g_sBilinearBlocks : register(s4);
uniform sampler2D g_sConv16to32 : register(s4);
uniform sampler3D g_sConv32to16 : register(s4);
uniform sampler2D g_sBitwiseANDX : register(s5);
uniform sampler2D g_sBitwiseANDY : register(s6);
uniform samplerRECT g_sInterlace : register(s7);

// used only on rare cases where the render target is PSMT8H
uniform sampler2D g_sCLUT : register(s2);

// global pixel shader constants
uniform float4 g_fInvTexDims : register(c22); // similar to g_fClutOff
uniform float4 g_fFogColor : register(c23);

// used for rectblitting
uniform float4 g_fBitBltZ : register(c24);

uniform half4 g_fOneColor : register(c25); // col*.xxxy+.zzzw

// vertex shader constants
uniform float4 g_fBitBltPos : register(c4);
uniform float4 g_fZ : register(c5);				// transforms d3dcolor z into float z
uniform float4 g_fZNorm : register(c6);
uniform float4 g_fBitBltTex : register(c7);

// pixel shader consts
// .z is used for the addressing fn
uniform half4 g_fExactColor : register(c27) = half4(0.5,0.5/256.0f,0,1/255.0f);
uniform float4 g_fBilinear : register(c28) = float4(-0.7f, -0.65f, 0.9,1/32767.0f);
uniform float4 g_fZBias : register(c29) = half4(1.0f/256.0f, 1.0004f, 1, 0.5); // also for vs
uniform float4 g_fc0 : register(c30) = float4(0,1, 0.001, 0.5f); // also for vs
uniform float4 g_fMult : register(c31) = float4(1/1024.0f, 0.2f/1024.0f, 1/128.0f, 1/512.0f);

// vertex shader consts
uniform float4 g_fBitBltTrans : register(c31) = float4(0.5f, -0.5f, 0.5, 0.5 + 0.4/416.0f);

// given a local tex coord, returns the coord in the memory
float2 ps2memcoord(float2 realtex)
{
	float4 off;

	// block off
	realtex.xy = realtex.xy * g_fTexDims.xy + g_fTexDims.zw;
	realtex.xy = (realtex.xy - frac(realtex.xy)) * g_fMult.zw;
	float2 fblock = frac(realtex.xy);
	off.xy = realtex.xy-fblock.xy;

#ifdef ACCURATE_DECOMPRESSION
	off.zw = tex2D(g_sBlocks, g_fTexBlock.xy*fblock + g_fTexBlock.zw).ar;
	off.x = dot(off.xyw, g_fTexOffset.xyw);
	float f = frac(off.x);
	float fadd = g_fTexOffset.z * off.z;
	off.w = off.x + fadd;
	off.x = frac(f + fadd);
	off.w -= off.x;
#else
	off.z = tex2D(g_sBlocks, g_fTexBlock.xy*fblock + g_fTexBlock.zw).a;

	// combine the two
    off.x = dot(off.xyz, g_fTexOffset.xyz)+g_fTexOffset.w;
	off.x = modf(off.x, off.w);
#endif

	off.xy = off.xw * g_fPageOffset.zy + g_fPageOffset.wx;
    //off.y = off.w * g_fPageOffset.y + g_fPageOffset.x;
	return off.xy;
}

// find all texcoords for bilinear filtering
// assume that orgtex are already on boundaries
void ps2memcoord4(float4 orgtex, out float4 off0, out float4 off1)
{
	//float4 off0, off1, off2, off3;
	float4 realtex;

	// block off
	realtex = (orgtex * g_fTexDims.xyxy + g_fTexDims.zwzw);// * g_fMult.zwzw;
	float4 fblock = frac(realtex.xyzw);
	float4 ftransblock = g_fTexBlock.xyxy*fblock + g_fTexBlock.zwzw;
	realtex -= fblock;

	float4 transvals = g_fTexOffset.x * realtex.xzxz + g_fTexOffset.y * realtex.yyww + g_fTexOffset.w;

	float4 colors;// = tex2D(g_sBilinearBlocks, ftransblock.xy);

	// this is faster on ffx ingame
	colors.x = tex2D(g_sBlocks, ftransblock.xy).a;
	colors.y = tex2D(g_sBlocks, ftransblock.zy).a;
	colors.z = tex2D(g_sBlocks, ftransblock.xw).a;
	colors.w = tex2D(g_sBlocks, ftransblock.zw).a;

	float4 fr, rem;

#ifdef ACCURATE_DECOMPRESSION
	fr = frac(transvals);
	float4 fadd = colors * g_fTexOffset.z;
	rem = transvals + fadd;
	fr = frac(fr + fadd);
	rem -= fr;
#else
	transvals += colors * g_fTexOffset.z;

	fr = modf(transvals, rem);
#endif

	rem = rem * g_fPageOffset.y + g_fPageOffset.x;
	fr = fr * g_fPageOffset.z + g_fPageOffset.w;

	// combine
	off0 = g_fc0.yxyx * fr.xxyy + g_fc0.xyxy * rem.xxyy;
	off1 = g_fc0.yxyx * fr.zzww + g_fc0.xyxy * rem.zzww;
}

void ps2memcoord4_fast(float4 orgtex, out float4 off0, out float4 off1)
{
	float4 realtex;

	realtex = (orgtex * g_fTexDims.xyxy + g_fTexDims.zwzw);// * g_fMult.zwzw;
	float4 fblock = frac(realtex.xyzw);
	float2 ftransblock = g_fTexBlock.xy*fblock.xy + g_fTexBlock.zw;
	realtex -= fblock;

	float4 transvals = g_fTexOffset.x * realtex.xzxz + g_fTexOffset.y * realtex.yyww + g_fTexOffset.w;

	float4 colors = tex2D(g_sBilinearBlocks, ftransblock.xy);
	float4 fr, rem;

#ifdef ACCURATE_DECOMPRESSION
	fr = frac(transvals);
	float4 fadd = colors * g_fTexOffset.z;
	rem = transvals + fadd;
	fr = frac(fr + fadd);
	rem -= fr;
#else
	transvals += colors * g_fTexOffset.z;

	fr = modf(transvals, rem);
#endif

	rem = rem * g_fPageOffset.y + g_fPageOffset.x;
    fr = fr * g_fPageOffset.z;

	off0 = g_fc0.yxyx * fr.xxyy + g_fc0.xyxy * rem.xxyy;
	off1 = g_fc0.yxyx * fr.zzww + g_fc0.xyxy * rem.zzww;
}

// Wrapping modes
#if defined(REPEAT)

float2 ps2addr(float2 coord)
{
	return frac(coord.xy);
}

#elif defined(CLAMP)

float2 ps2addr(float2 coord)
{
	return clamp(coord.xy, g_fClampExts.xy, g_fClampExts.zw);
}

#elif defined(REGION_REPEAT)

// computes the local tex coord along with addressing modes
float2 ps2addr(float2 coord)
{
	float2 final = frac(clamp(coord.xy, g_fClampExts.xy, g_fClampExts.zw));

	if( TexWrapMode.x > g_fBilinear.z ) // region repeat mode for x (umsk&x)|ufix
		final.x = tex2D(g_sBitwiseANDX, abs(coord.x)*TexWrapMode.zz).x * g_fClampExts.x + g_fClampExts.z;
	if( TexWrapMode.y > g_fBilinear.z ) // region repeat mode for x (vmsk&x)|vfix
		final.y = tex2D(g_sBitwiseANDY, abs(coord.y)*TexWrapMode.ww).x * g_fClampExts.y + g_fClampExts.w;

	return final;
}

#else

float2 ps2addr(float2 coord)
{
	return frac(clamp(coord.xy, g_fClampExts.xy, g_fClampExts.zw));
}

#endif

half4 tex2DPS_32(float2 tex0)
{
	return texRECT(g_sMemory, ps2memcoord(tex0).xy);
}

// use when texture is not tiled
half4 tex2DPS_tex32(float2 tex0)
{
	return texRECT(g_sMemory, g_fTexDims.xy*tex0+g_fTexDims.zw)*g_fZBias.zzzw+g_fPageOffset.w;
}

// use when texture is not tiled
half4 tex2DPS_clut32(float2 tex0)
{
	float index = texRECT(g_sMemory, g_fTexDims.xy*tex0+g_fTexDims.zw).a+g_fPageOffset.w;
	return tex2D(g_sCLUT, index*g_fExactColor.xz+g_fExactColor.yz);
}

// use when texture is not tiled and converting from 32bit to 16bit
// don't convert on the block level, only on the column level
// so every other 8 pixels, use the upper bits instead of lower
half4 tex2DPS_tex32to16(float2 tex0)
{
	bool upper = false;
	tex0.y += g_fPageOffset.z;
	float2 ffrac = fmod(tex0, g_fTexOffset.xy);
	tex0.xy = g_fc0.ww * (tex0.xy + ffrac);
	if( ffrac.x > g_fTexOffset.z ) {
		tex0.x -= g_fTexOffset.z;
		upper = true;
	}
	if( ffrac.y >= g_fTexOffset.w ) {
		tex0.y -= g_fTexOffset.w;
		tex0.x += g_fc0.w;
	}

	half4 color = texRECT(g_sMemory, g_fTexDims.xy*tex0+g_fTexDims.zw)*g_fZBias.zzzw+g_fPageOffset.w;
	float2 uv = upper ? color.xw : color.zy;
	return tex2D(g_sConv16to32, uv+g_fPageOffset.xy);
}

// used when a 16 bit texture is used an 8h
half4 tex2DPS_tex16to8h(float2 tex0)
{
	float4 final;
	float2 ffrac = fmod(tex0+g_fPageOffset.zw, g_fTexOffset.xy);
	tex0.xy = g_fPageOffset.xy * tex0.xy - ffrac * g_fc0.yw;

	if( ffrac.x > g_fTexOffset.x*g_fc0.w )
		tex0.x += g_fTexOffset.x*g_fc0.w;
	if( tex0.x >= g_fc0.y ) tex0 += g_fTexOffset.zw;

	float4 upper = texRECT(g_sMemory, g_fTexDims.xy*tex0+g_fTexDims.zw);

	// only need alpha
	float index = tex3D(g_sConv32to16, upper.zyx-g_fc0.z).y + upper.w*g_fc0.w*g_fc0.w;
	return tex2D(g_sCLUT, index+g_fExactColor.yz);
}

//half4 f;
//f.w = old.y > (127.2f/255.0f) ? 1 : 0;
//old.y -= 0.5f * f.w;
//f.xyz = frac(old.yyx*half3(2.002*255.0f/256.0f, 64.025f*255.0f/256.0f, 8.002*255.0f/256.0f));
//f.y += old.x * (0.25f*255.0f/256.0f);

////////////////////////////////
// calculates the texture color
////////////////////////////////

#define decl_ps2shade(num) \
decl_ps2shade_##num(_32) \
decl_ps2shade_##num(_tex32) \
decl_ps2shade_##num(_clut32) \
decl_ps2shade_##num(_tex32to16) \
decl_ps2shade_##num(_tex16to8h) \

// nearest
#define decl_ps2shade_0(bit) \
float4 ps2shade0##bit( TEX_DECL tex) \
{ \
    return tex2DPS##bit( ps2addr(TEX_XY)); \
} \

// do fast memcoord4 calcs when textures behave well
#ifdef REPEAT
#define PS2MEMCOORD4 ps2memcoord4
#else
#define PS2MEMCOORD4 ps2memcoord4
#endif

#define decl_BilinearFilter(bit, addrfn) \
half4 BilinearFilter##bit(float2 tex0) \
{ \
	float4 off0, off1; \
	float4 ftex; \
	float2 ffrac; \
	ftex.xy = tex0 + g_fBilinear.xy * g_fRealTexDims.zw; \
	ffrac = frac(ftex.xy*g_fRealTexDims.xy); \
	ftex.xy -= ffrac.xy * g_fRealTexDims.zw; \
	\
 	ftex.zw = ps2addr(ftex.xy + g_fRealTexDims.zw); \
 	ftex.xy = ps2addr(ftex.xy); \
 	\
 	PS2MEMCOORD4(ftex, off0, off1); \
 	half4 c0 = texRECT(g_sMemory, off0.xy); \
	half4 c1 = texRECT(g_sMemory, off0.zw); \
	half4 c2 = texRECT(g_sMemory, off1.xy); \
	half4 c3 = texRECT(g_sMemory, off1.zw); \
	return lerp( lerp(c0, c1, ffrac.x), lerp(c2, c3, ffrac.x), ffrac.y ); \
} \

decl_BilinearFilter(_32, ps2addr)
decl_BilinearFilter(_tex32, ps2addr)
decl_BilinearFilter(_clut32, ps2addr)
decl_BilinearFilter(_tex32to16, ps2addr)
decl_BilinearFilter(_tex16to8h, ps2addr)

//TODO! For mip maps, only apply when LOD >= 0
// lcm == 0, LOD = log(1/Q)*L + K, lcm == 1, LOD = K

// bilinear
#define decl_ps2shade_1(bit) \
half4 ps2shade1##bit(TEX_DECL tex) \
{ \
	return BilinearFilter##bit(TEX_XY); \
} \

// nearest, mip nearest
#define decl_ps2shade_2(bit) \
half4 ps2shade2##bit(TEX_DECL tex) \
{ \
    return tex2DPS##bit( ps2addr(TEX_XY)); \
} \

// nearest, mip linear
#define decl_ps2shade_3(bit) \
half4 ps2shade3##bit(TEX_DECL tex) \
{ \
    return tex2DPS##bit(ps2addr(TEX_XY)); \
} \

// linear, mip nearest
#define decl_ps2shade_4(bit) \
half4 ps2shade4##bit(TEX_DECL tex) \
{ \
    return BilinearFilter##bit(TEX_XY); \
} \

// linear, mip linear
#define decl_ps2shade_5(bit) \
half4 ps2shade5##bit(TEX_DECL tex) \
{ \
    return BilinearFilter##bit(TEX_XY); \
} \

decl_ps2shade(0)
decl_ps2shade(1)
decl_ps2shade(2)
decl_ps2shade(3)
decl_ps2shade(4)
decl_ps2shade(5)

half4 ps2CalcShade(half4 texcol, half4 color)
{
#ifdef TEST_AEM
	if( dot(texcol.xyzw, g_fTestBlack.xyzw) <= g_fc0.z )
		texcol.w = g_fc0.x;
	else
#endif
		texcol.w = texcol.w * fTexAlpha.y + fTexAlpha.x;

	texcol = texcol * (fTexAlpha2.zzzw * color + fTexAlpha2.xxxy) + fTexAlpha.zzzw * color.wwww;

	return texcol;
}

// final ops on the color
#ifdef EXACT_COLOR

half4 ps2FinalColor(half4 col)
{
	// g_fOneColor has to scale by 255
	half4 temp = col * g_fOneColor.xxxy + g_fOneColor.zzzw;
	temp.w = floor(temp.w)*g_fExactColor.w;
	return temp;
}

#else
half4 ps2FinalColor(half4 col)
{
	return col * g_fOneColor.xxxy + g_fOneColor.zzzw;
}
#endif

////////////////
// Techniques //
////////////////

// technique to copy a rectangle from source to target
struct VSOUT_
{
    float4 pos : POSITION;
    half4 color : COLOR0;
	DOZWRITE(float4 z : TEXCOORD0;)
};

struct VSOUT_T
{
    float4 pos : POSITION;
    half4 color : COLOR0;
    TEX_DECL tex : TEXCOORD0;
	DOZWRITE(float4 z : TEXCOORD1;)
};

struct VSOUT_F
{
    float4 pos : POSITION;
    half4 color : COLOR0;
	float fog : TEXCOORD0;
	DOZWRITE(float4 z : TEXCOORD1;)
};

struct VSOUT_TF
{
    float4 pos : POSITION;
    half4 color : COLOR0;
    TEX_DECL tex : TEXCOORD0;
    half fog : TEXCOORD1;
    DOZWRITE(float4 z : TEXCOORD2;)
};

// just smooth shadering
VSOUT_ RegularVS(float4 pos : POSITION,
				half4 color : COLOR0,
				float4 z : COLOR1
				)
{
    VSOUT_ o;

	o.pos.xy = pos.xy*g_fPosXY.xy+g_fPosXY.zw;
	o.pos.z = log(g_fc0.y+dot(g_fZ, z.zyxw))*g_fZNorm.x+g_fZNorm.y;
	o.pos.w = g_fc0.y; // 1
	o.color = color;

	DOZWRITE(o.z = z*g_fZBias.x+g_fZBias.y; o.z.w = g_fc0.y;)
    return o;
}

void RegularPS(VSOUT_ i, out float4 c0 : COLOR0
#ifdef WRITE_DEPTH
	, out float4 c1 : COLOR1
#endif
	)
{
	// whenever outputting depth, make sure to mult by 255/256 and 1
	c0 = ps2FinalColor(i.color);
	DOZWRITE(c1 = i.z;)
}

// diffuse texture mapping
VSOUT_T TextureVS(float4 pos : POSITION,
                half4 color : COLOR0,
				float4 z : COLOR1,
                float3 tex0 : TEXCOORD0)
{
    VSOUT_T o;
	o.pos.xy = pos.xy*g_fPosXY.xy+g_fPosXY.zw;
	o.pos.z = log(g_fc0.y+dot(g_fZ, z.zyxw))*g_fZNorm.x + g_fZNorm.y;
	o.pos.w = g_fc0.y;
 	o.color = color;
 	DOZWRITE(o.z = z*g_fZBias.x+g_fZBias.y; o.z.w = g_fc0.y;)
#ifdef PERSPECTIVE_CORRECT_TEX
    o.tex = tex0;
#else
	o.tex = tex0.xy/tex0.z;
#endif
    return o;
}

#ifdef WRITE_DEPTH

#define DECL_TEXPS(num, bit) \
void Texture##num##bit##PS(VSOUT_T i, out half4 c0 : COLOR0, out float4 c1 : COLOR1) \
{ \
    c0 = ps2FinalColor(ps2CalcShade(ps2shade##num##bit(i.tex), i.color)); \
    c1 = i.z; \
} \

#else

#define DECL_TEXPS(num, bit) \
void Texture##num##bit##PS(VSOUT_T i, out half4 c0 : COLOR0) \
{ \
    c0 = ps2FinalColor(ps2CalcShade(ps2shade##num##bit(i.tex), i.color)); \
} \

#endif

#define DECL_TEXPS_(num) \
DECL_TEXPS(num, _32) \
DECL_TEXPS(num, _tex32) \
DECL_TEXPS(num, _clut32) \
DECL_TEXPS(num, _tex32to16) \
DECL_TEXPS(num, _tex16to8h) \

DECL_TEXPS_(0)
DECL_TEXPS_(1)
DECL_TEXPS_(2)
DECL_TEXPS_(3)
DECL_TEXPS_(4)
DECL_TEXPS_(5)

VSOUT_F RegularFogVS(float4 pos : POSITION,
				half4 color : COLOR0,
				float4 z : COLOR1)
{
    VSOUT_F o;

	o.pos.xy = pos.xy*g_fPosXY.xy+g_fPosXY.zw;
	o.pos.z = log(g_fc0.y+dot(g_fZ, z.zyxw))*g_fZNorm.x+g_fZNorm.y;
	o.pos.w = g_fc0.y;
	DOZWRITE(o.z = z*g_fZBias.x+g_fZBias.y; o.z.w = g_fc0.y;)
	o.color = color;
    o.fog = pos.z*g_fBilinear.w;
    return o;
}

void RegularFogPS(VSOUT_F i, out half4 c0 : COLOR0
#ifdef WRITE_DEPTH
	, out float4 c1 : COLOR1
#endif
	)
{
	half4 c;
	c.xyz = lerp(g_fFogColor.xyz, i.color.xyz, i.fog);
	c.w = i.color.w;
	c0 = ps2FinalColor(c);
   	DOZWRITE(c1 = i.z;)
}

VSOUT_TF TextureFogVS(float4 pos : POSITION,
				half4 color : COLOR0,
				float4 z : COLOR1,
				float3 tex0 : TEXCOORD0)
{
    VSOUT_TF o;

	o.pos.xy = pos.xy*g_fPosXY.xy+g_fPosXY.zw;
	o.pos.z = log(g_fc0.y+dot(g_fZ, z.zyxw))*g_fZNorm.x+g_fZNorm.y;
	o.pos.w = g_fc0.y;
    o.color = color;
    o.fog = pos.z*g_fBilinear.w;
    DOZWRITE(o.z = z*g_fZBias.x+g_fZBias.y; o.z.w = g_fc0.y;)
#ifdef PERSPECTIVE_CORRECT_TEX
    o.tex = tex0;
#else
	o.tex = tex0.xy/tex0.z;
#endif
    return o;
}

#ifdef WRITE_DEPTH

#define DECL_TEXFOGPS(num, bit) \
void TextureFog##num##bit##PS(VSOUT_TF i, out half4 c0 : COLOR0, out float4 c1 : COLOR1 ) \
{ \
	half4 c = ps2CalcShade(ps2shade##num##bit(i.tex), i.color); \
	c.xyz = lerp(g_fFogColor.xyz, c.xyz, i.fog); \
	c0 = ps2FinalColor(c); \
   	c1 = i.z; \
} \

#else

#define DECL_TEXFOGPS(num, bit) \
void TextureFog##num##bit##PS(VSOUT_TF i, out half4 c0 : COLOR0) \
{ \
	half4 c = ps2CalcShade(ps2shade##num##bit(i.tex), i.color); \
	c.xyz = lerp(g_fFogColor.xyz, c.xyz, i.fog); \
	c0 = ps2FinalColor(c); \
} \

#endif

#define DECL_TEXFOGPS_(num) \
DECL_TEXFOGPS(num, _32) \
DECL_TEXFOGPS(num, _tex32) \
DECL_TEXFOGPS(num, _clut32) \
DECL_TEXFOGPS(num, _tex32to16) \
DECL_TEXFOGPS(num, _tex16to8h) \

DECL_TEXFOGPS_(0)
DECL_TEXFOGPS_(1)
DECL_TEXFOGPS_(2)
DECL_TEXFOGPS_(3)
DECL_TEXFOGPS_(4)
DECL_TEXFOGPS_(5)

//-------------------------------------------------------
// Techniques not related to the main primitive commands
half4 BilinearBitBlt(float2 tex0)
{
	float4 ftex;
	float2 ffrac;

	ffrac.xy = frac(tex0*g_fRealTexDims.xy);
	ftex.xy = tex0 - ffrac.xy * g_fRealTexDims.zw;
	ftex.zw = ftex.xy + g_fRealTexDims.zw;

	float4 off0, off1;
	ps2memcoord4_fast(ftex, off0, off1);
	half4 c0 = texRECT(g_sMemory, off0.xy);
	half4 c1 = texRECT(g_sMemory, off0.zw);
	half4 c2 = texRECT(g_sMemory, off1.xy);
	half4 c3 = texRECT(g_sMemory, off1.zw);

	return lerp( lerp(c0, c1, ffrac.x), lerp(c2, c3, ffrac.x), ffrac.y );
}

void BitBltVS(in float4 pos : POSITION,
				in half4 tex0 : COLOR1,
                in float3 tex : TEXCOORD0,
				out float4 opos : POSITION,
                out float2 otex0 : TEXCOORD0,
				out float2 ointerpos : TEXCOORD1)
{
    opos.xy = pos.xy * g_fBitBltPos.xy + g_fBitBltPos.zw;
   	ointerpos = opos.xy * g_fBitBltTrans.xy + g_fBitBltTrans.zw;
    opos.zw = g_fc0.xy;
    otex0 = tex.xy * g_fBitBltTex.xy + g_fBitBltTex.zw;
}

half4 BitBltPS(in float2 tex0 : TEXCOORD0) : COLOR
{
	return texRECT(g_sMemory, ps2memcoord(tex0).xy)*g_fOneColor.xxxy;
}

// used when AA
half4 BitBltAAPS(in float2 tex0 : TEXCOORD0) : COLOR
{
	return BilinearBitBlt(tex0)*g_fOneColor.xxxy;
}

void BitBltDepthPS(in float2 tex0 : TEXCOORD0,
					out float4 c : COLOR0,
					out float depth : DEPTH)
{
	c = texRECT(g_sMemory, ps2memcoord(tex0));

	depth = log(g_fc0.y+dot(c, g_fBitBltZ))*g_fOneColor.w;
	c += g_fZBias.y;
}

void BitBltDepthMRTPS(in float2 tex0 : TEXCOORD0,
					out half4 c0 : COLOR0,
					out float4 c1 : COLOR1,
					out float depth : DEPTH)
{
	c1 = texRECT(g_sMemory, ps2memcoord(tex0));

	depth = log(g_fc0.y+dot(c1, g_fBitBltZ))*g_fOneColor.w;
	c1 += g_fZBias.y;
	c0 = g_fc0.x;
}

/*static const float BlurKernel[9] = {
	0.027601,
	0.066213,
	0.123701,
	0.179952,
	0.205065,
	0.179952,
	0.123701,
	0.066213,
	0.027601
};*/

half4 BilinearFloat16(float2 tex0)
{
	/*float4 ffrac, ftex;
	ffrac.xy = frac(tex0);
	ftex.xy = (tex0 - ffrac.xy) * g_fInvTexDims.xy + g_fInvTexDims.zw;
	ftex.zw = ftex.xy + g_fInvTexDims.xy;

	half4 c0 = texRECT(g_sSrcFinal, ftex.xy);
	half4 c1 = texRECT(g_sSrcFinal, ftex.zy);
	half4 c2 = texRECT(g_sSrcFinal, ftex.xw);
	half4 c3 = texRECT(g_sSrcFinal, ftex.zw);

	return lerp( lerp(c0, c1, ffrac.x), lerp(c2, c3, ffrac.x), ffrac.y );*/
	return texRECT(g_sSrcFinal, tex0.xy);
//	return 0.55f * texRECT(g_sSrcFinal, tex0.xy) +
//			0.15f * texRECT(g_sSrcFinal, tex0.xy+g_fInvTexDims.xz) +
//			0.15f * texRECT(g_sSrcFinal, tex0.xy+g_fInvTexDims.zy) +
//			0.15f * texRECT(g_sSrcFinal, tex0.xy+g_fInvTexDims.xy);
}

half4 CRTCTargInterPS(in float2 tex0 : TEXCOORD0, in float2 ointerpos : TEXCOORD1) : COLOR
{
	float finter = texRECT(g_sInterlace, ointerpos.yy).x;
	clip(finter * g_fOneColor.z + g_fOneColor.w);

	float4 c = BilinearFloat16(tex0);
	c.w = g_fc0.w*c.w * g_fOneColor.x + g_fOneColor.y;
	return c;
}

half4 CRTCTargPS(in float2 tex0 : TEXCOORD0) : COLOR
{
	float4 c = BilinearFloat16(tex0);
	c.w = g_fc0.w*c.w * g_fOneColor.x + g_fOneColor.y;
	return c;
}

half4 CRTCInterPS(in float2 tex0 : TEXCOORD0, in float2 ointerpos : TEXCOORD1) : COLOR
{
	float2 filtcoord = (tex0-frac(tex0))*g_fInvTexDims.xy+g_fInvTexDims.zw;
	float finter = texRECT(g_sInterlace, ointerpos.yy).x;
	clip(finter * g_fOneColor.z + g_fOneColor.w);

	half4 c = BilinearBitBlt(filtcoord);
	c.w = c.w * g_fOneColor.x + g_fOneColor.y;

	return c;
}

// simpler
half4 CRTCInterPS_Nearest(in float2 tex0 : TEXCOORD0, in float2 ointerpos : TEXCOORD1) : COLOR
{
    float finter = texRECT(g_sInterlace, ointerpos.yy).x;
	clip(finter * g_fOneColor.z + g_fOneColor.w);

	half4 c = texRECT(g_sMemory, ps2memcoord(tex0).xy);
	c.w = c.w * g_fOneColor.x + g_fOneColor.y;
	return c;
}

half4 CRTCPS(in float2 tex0 : TEXCOORD0) : COLOR
{
	float2 filtcoord = (tex0/*-frac(tex0)*/)*g_fInvTexDims.xy+g_fInvTexDims.zw;
	half4 c = BilinearBitBlt(filtcoord);
	c.w = c.w * g_fOneColor.x + g_fOneColor.y;

	return c;
}

// simpler
half4 CRTCPS_Nearest(in float2 tex0 : TEXCOORD0) : COLOR
{
	half4 c = texRECT(g_sMemory, ps2memcoord(tex0).xy);
	c.w = c.w * g_fOneColor.x + g_fOneColor.y;
	return c;
}

half4 CRTC24InterPS(in float2 tex0 : TEXCOORD0, in float2 ointerpos : TEXCOORD1) : COLOR
{
	float2 filtcoord = (tex0-frac(tex0))*g_fInvTexDims.xy+g_fInvTexDims.zw;
	float finter = texRECT(g_sInterlace, ointerpos.yy).x;
	clip(finter * g_fOneColor.z + g_fOneColor.w);

	half4 c = texRECT(g_sMemory, ps2memcoord(filtcoord).xy).x;
	c.w = c.w * g_fOneColor.x + g_fOneColor.y;

	return c;
}

half4 CRTC24PS(in float2 tex0 : TEXCOORD0) : COLOR
{
	float2 filtcoord = (tex0-frac(tex0))*g_fInvTexDims.xy+g_fInvTexDims.zw;
	half4 c = texRECT(g_sMemory, ps2memcoord(filtcoord).xy).x;
	c.w = c.w * g_fOneColor.x + g_fOneColor.y;

	return c;
}

half4 ZeroPS() : COLOR
{
	return g_fOneColor.x;
}

half4 BaseTexturePS(in float2 tex0 : TEXCOORD0) : COLOR
{
	return texRECT(g_sSrcFinal, tex0) * g_fOneColor;
}

half4 Convert16to32PS(float2 tex0 : TEXCOORD0) : COLOR
{
	float4 final;
	float2 ffrac = fmod(tex0+g_fTexDims.zw, g_fTexOffset.xy);
	tex0.xy = g_fTexDims.xy * tex0.xy - ffrac * g_fc0.yw;

	if( ffrac.x > g_fTexOffset.x*g_fc0.w )
		tex0.x += g_fTexOffset.x*g_fc0.w;
	if( tex0.x >= g_fc0.y ) tex0 += g_fTexOffset.zw;

	float4 lower = texRECT(g_sSrcFinal, tex0);
	float4 upper = texRECT(g_sSrcFinal, tex0+g_fPageOffset.xy);

	final.zy = tex3D(g_sConv32to16, lower.zyx).xy + lower.ww*g_fPageOffset.zw;
	final.xw = tex3D(g_sConv32to16, upper.zyx).xy + upper.ww*g_fPageOffset.zw;

	return final;
}

// use when texture is not tiled and converting from 32bit to 16bit
// don't convert on the block level, only on the column level
// so every other 8 pixels, use the upper bits instead of lower
half4 Convert32to16PS(float2 tex0 : TEXCOORD0) : COLOR
{
	bool upper = false;
	float2 ffrac = fmod(tex0+g_fTexDims.zw, g_fTexOffset.xy);
	tex0.xy = g_fc0.ww * (tex0.xy + ffrac);
	if( ffrac.x > g_fTexOffset.z ) {
		tex0.x -= g_fTexOffset.z;
		upper = true;
	}
	if( ffrac.y >= g_fTexOffset.w ) {
		tex0.y -= g_fTexOffset.w;
		tex0.x += g_fc0.w;
	}

	half4 color = texRECT(g_sSrcFinal, tex0*g_fTexDims.xy)*g_fc0.yyyw;
	float2 uv = upper ? color.xw : color.zy;
	return tex2D(g_sConv16to32, uv*g_fPageOffset.xy+g_fPageOffset.zw)*g_fTexDims.xxxy;
}
