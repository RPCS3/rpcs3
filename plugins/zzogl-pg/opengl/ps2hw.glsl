// Cg Shaders for PS2 GS emulation

// divides by z for every pixel, instead of in vertex shader
// fixes kh textures

#extension ARB_texture_rectangle: enable
#define GL_compatibility_profile 1
#define PERSPECTIVE_CORRECT_TEX

// When writting GLSL code we should change variables in code according to denominator
// Not than in and out variables are differ!
// in POSITION  	set by glVertexPointer		goes 	to gl_Vertex;
// out POSITION						goes    to gl_position
// in COLOR0							gl_Color
// out COLOR0							gl_FrontColor
// in TEXCOORD0							gl_MultiTexCoord0
// out TEXCOORD0						gl_TexCoord[0]

//in Fragments:
// in TEXCOORD0							gl_TexCoord[0]
// out COLOR0							gl_FragData[0]

//#define TEST_AEM // tests AEM for black pixels
//#define REGION_REPEAT // set if texture wrapping mode is region repeat
//#define WRITE_DEPTH // set if depth is also written in a MRT
//#define ACCURATE_DECOMPRESSION // set for less capable hardware ATI Radeon 9000 series
//#define EXACT_COLOR	// make sure the output color is clamped to 1/255 boundaries (for alpha testing)

#ifdef PERSPECTIVE_CORRECT_TEX
#define TEX_XY tex.xy/tex.z
#define TEX_DECL vec4
#else
#define TEX_XY tex.xy
#define TEX_DECL vec4
#endif

#ifdef WRITE_DEPTH
#define DOZWRITE(x) x
#else
#define DOZWRITE(x)
#endif

// NVidia CG-data types
#define half2 vec2
#define half3 vec3
#define half4 vec4
#define float2 vec2
#define float3 vec3
#define float4 vec4

// main ps2 memory, each pixel is stored in 32bit color
uniform sampler2DRect g_sMemory[2];

// per context pixel shader constants
uniform half4 fTexAlpha2[2];

uniform float4 g_fTexOffset[2];			// converts the page and block offsets into the mem addr/1024
uniform float4 g_fTexDims[2];				// mult by tex dims when accessing the block texture
uniform float4 g_fTexBlock[2];

uniform float4 g_fClampExts[2];		// if clamping the texture, use (minu, minv, maxu, maxv)
uniform float4 TexWrapMode[2];			// 0 - repeat/clamp, 1 - region rep (use fRegRepMask)

uniform float4 g_fRealTexDims[2]; // tex dims used for linear filtering (w,h,1/w,1/h)

// (alpha0, alpha1, 1 if highlight2 and tcc is rgba, 1-y)
uniform half4 g_fTestBlack[2];	// used for aem bit

uniform float4 g_fPageOffset[2];

uniform half4 fTexAlpha[2];

// vertex shader constants
uniform float4 g_fPosXY[2];

// used to get the tiled offset into a page given the linear offset
uniform sampler2DRect g_sSrcFinal;
uniform sampler2D g_sBlocks;
uniform sampler2D g_sBilinearBlocks;
uniform sampler2D g_sConv16to32;
uniform sampler3D g_sConv32to16;
uniform sampler2DRect g_sBitwiseANDX;
uniform sampler2DRect g_sBitwiseANDY;
uniform sampler2DRect g_sInterlace;

// used only on rare cases where the render target is PSMT8H
uniform sampler2D g_sCLUT;

// global pixel shader constants
uniform float4 g_fInvTexDims; // similar to g_fClutOff
uniform float4 g_fFogColor;

// used for rectblitting
uniform float4 g_fBitBltZ;

uniform half4 g_fOneColor; // col*.xxxy+.zzzw

// vertex shader constants
uniform float4 g_fBitBltPos;
uniform float4 g_fZ;				// transforms d3dcolor z into float z
uniform float4 g_fZNorm;
uniform float4 g_fZMin = float4(0.0f, 1.0f, 0.0f, 0.0f);
uniform float4 g_fBitBltTex;

// pixel shader consts
// .z is used for the addressing fn
uniform half4 g_fExactColor = half4(0.5,0.5/256.0f,0,1/255.0f);
uniform float4 g_fBilinear = float4(-0.7f, -0.65f, 0.9,1/32767.0f);
uniform float4 g_fZBias = half4(1.0f/256.0f, 1.0004f, 1, 0.5); // also for vs
uniform float4 g_fc0 = float4(0,1, 0.001, 0.5f); // also for vs
uniform float4 g_fMult = float4(1/1024.0f, 0.2f/1024.0f, 1/128.0f, 1/512.0f);

// vertex shader consts
uniform float4 g_fBitBltTrans = float4(0.5f, -0.5f, 0.5, 0.5 + 0.4/416.0f);

// given a local tex coord, returns the coord in the memory
float2 ps2memcoord(float2 realtex)
{
	float4 off;

	// block off
	realtex.xy = realtex.xy * g_fTexDims[CTX].xy + g_fTexDims[CTX].zw;
	realtex.xy = (realtex.xy - fract(realtex.xy)) * g_fMult.zw;
	float2 fblock = fract(realtex.xy);
	off.xy = realtex.xy-fblock.xy;

#ifdef ACCURATE_DECOMPRESSION
	off.zw = texture(g_sBlocks, g_fTexBlock[CTX].xy*fblock + g_fTexBlock[CTX].zw).ar;
	off.x = dot(off.xy, g_fTexOffset[CTX].xy);
	float r = g_fTexOffset[CTX].w;
	float f = fract(off.x);
	float fadd = g_fTexOffset[CTX].z * off.z;
	off.w = off.x + fadd + r;
	off.x = fract(f + fadd + r);
	off.w -= off.x ;
#else
	off.z = texture(g_sBlocks, g_fTexBlock[CTX].xy*fblock + g_fTexBlock[CTX].zw).a;

	// combine the two
    off.x = dot(off.xyz, g_fTexOffset[CTX].xyz)+g_fTexOffset[CTX].w;
	off.x = modf(off.x, off.w);
#endif

	off.xy = off.xw * g_fPageOffset[CTX].zy + g_fPageOffset[CTX].wx;
    //off.y = off.w * g_fPageOffset[CTX].y + g_fPageOffset[CTX].x;
	return off.xy;
}

// find all texcoords for bilinear filtering
// assume that orgtex are already on boundaries
void ps2memcoord4(float4 orgtex, out float4 off0, out float4 off1)
{
	//float4 off0, off1, off2, off3;
	float4 realtex;

	// block off
	realtex = (orgtex * g_fTexDims[CTX].xyxy + g_fTexDims[CTX].zwzw);// * g_fMult.zwzw;
	float4 fblock = fract(realtex.xyzw);
	float4 ftransblock = g_fTexBlock[CTX].xyxy*fblock + g_fTexBlock[CTX].zwzw;
	realtex -= fblock;

	float4 transvals = g_fTexOffset[CTX].x * realtex.xzxz + g_fTexOffset[CTX].y * realtex.yyww + g_fTexOffset[CTX].w;

	float4 colors;// = texture(g_sBilinearBlocks, ftransblock.xy);

	// this is faster on ffx ingame
	colors.x = texture(g_sBlocks, ftransblock.xy).a;
	colors.y = texture(g_sBlocks, ftransblock.zy).a;
	colors.z = texture(g_sBlocks, ftransblock.xw).a;
	colors.w = texture(g_sBlocks, ftransblock.zw).a;

	float4 fr, rem;

#ifdef ACCURATE_DECOMPRESSION
	fr = fract(transvals);
	float4 fadd = colors * g_fTexOffset[CTX].z;
	rem = transvals + fadd;
	fr = fract(fr + fadd);
	rem -= fr;
#else
	transvals += colors * g_fTexOffset[CTX].z;

	fr = modf(transvals, rem);
#endif

	rem = rem * g_fPageOffset[CTX].y + g_fPageOffset[CTX].x;
	fr = fr * g_fPageOffset[CTX].z + g_fPageOffset[CTX].w;

	// combine
	off0 = g_fc0.yxyx * fr.xxyy + g_fc0.xyxy * rem.xxyy;
	off1 = g_fc0.yxyx * fr.zzww + g_fc0.xyxy * rem.zzww;
}

void ps2memcoord4_fast(float4 orgtex, out float4 off0, out float4 off1)
{
	float4 realtex;

	realtex = (orgtex * g_fTexDims[CTX].xyxy + g_fTexDims[CTX].zwzw);// * g_fMult.zwzw;
	float4 fblock = fract(realtex.xyzw);
	float2 ftransblock = g_fTexBlock[CTX].xy*fblock.xy + g_fTexBlock[CTX].zw;
	realtex -= fblock;

	float4 transvals = g_fTexOffset[CTX].x * realtex.xzxz + g_fTexOffset[CTX].y * realtex.yyww + g_fTexOffset[CTX].w;

	float4 colors = texture(g_sBilinearBlocks, ftransblock.xy);
	float4 fr, rem;

#ifdef ACCURATE_DECOMPRESSION
	fr = fract(transvals);
	float4 fadd = colors * g_fTexOffset[CTX].z;
	rem = transvals + fadd;
	fr = fract(fr + fadd);
	rem -= fr;
#else
	transvals += colors * g_fTexOffset[CTX].z;

	fr = modf(transvals, rem);
#endif

	rem = rem * g_fPageOffset[CTX].y + g_fPageOffset[CTX].x;
	fr = fr * g_fPageOffset[CTX].z;

	off0 = g_fc0.yxyx * fr.xxyy + g_fc0.xyxy * rem.xxyy;
	off1 = g_fc0.yxyx * fr.zzww + g_fc0.xyxy * rem.zzww;
}

// Wrapping modes
#if defined(REPEAT)

float2 ps2addr(float2 coord)
{
	return fract(coord.xy);
}

#elif defined(CLAMP)

float2 ps2addr(float2 coord)
{
	return clamp(coord.xy, g_fClampExts[CTX].xy, g_fClampExts[CTX].zw);
}

#elif defined(REGION_REPEAT)

// computes the local tex coord along with addressing modes
float2 ps2addr(float2 coord)
{
	float2 final = fract(clamp(coord.xy, g_fClampExts[CTX].xy, g_fClampExts[CTX].zw));

	if( TexWrapMode[CTX].x > g_fBilinear.z ) // region repeat mode for x (umsk&x)|ufix
		final.x = texture(g_sBitwiseANDX, abs(coord.x)*TexWrapMode[CTX].zx).x * g_fClampExts[CTX].x + g_fClampExts[CTX].z;
	if( TexWrapMode[CTX].y > g_fBilinear.z ) // region repeat mode for x (vmsk&x)|vfix
		final.y = texture(g_sBitwiseANDY, abs(coord.y)*TexWrapMode[CTX].wy).x * g_fClampExts[CTX].y + g_fClampExts[CTX].w;

	return final;
}

#else

float2 ps2addr(float2 coord)
{
	return fract(clamp(coord.xy, g_fClampExts[CTX].xy, g_fClampExts[CTX].zw));
}

#endif

half4 tex2DPS_32(float2 tex0)
{
	return texture(g_sMemory[CTX], ps2memcoord(tex0).xy);
}

// use when texture is not tiled -- shader 1
half4 tex2DPS_tex32(float2 tex0)
{
	return texture(g_sMemory[CTX], g_fTexDims[CTX].xy*tex0+g_fTexDims[CTX].zw)*g_fZBias.zzzw+g_fPageOffset[CTX].w;
}

// use when texture is not tiled -- shader 2
half4 tex2DPS_clut32(float2 tex0)
{
	float index = texture(g_sMemory[CTX], g_fTexDims[CTX].xy*tex0+g_fTexDims[CTX].zw).a+g_fPageOffset[CTX].w;
	return texture(g_sCLUT, index*g_fExactColor.xz+g_fExactColor.yz);
}

// Shader 3
// use when texture is not tiled and converting from 32bit to 16bit
// don't convert on the block level, only on the column level
// so every other 8 pixels, use the upper bits instead of lower
half4 tex2DPS_tex32to16(float2 tex0)
{
	bool upper = false;
	tex0.y += g_fPageOffset[CTX].z;
	float2 ffrac = mod(tex0, g_fTexOffset[CTX].xy);
	tex0.xy = g_fc0.ww * (tex0.xy + ffrac);
	if( ffrac.x > g_fTexOffset[CTX].z ) {
		tex0.x -= g_fTexOffset[CTX].z;
		upper = true;
	}
	if( ffrac.y >= g_fTexOffset[CTX].w ) {
		tex0.y -= g_fTexOffset[CTX].w;
		tex0.x += g_fc0.w;
	}

	half4 color = texture(g_sMemory[CTX], g_fTexDims[CTX].xy*tex0+g_fTexDims[CTX].zw)*g_fZBias.zzzw+g_fPageOffset[CTX].w;
	float2 uv = upper ? color.xw : color.zy;
	return texture(g_sConv16to32, uv+g_fPageOffset[CTX].xy);
}

// Shader 4
// used when a 16 bit texture is used an 8h
half4 tex2DPS_tex16to8h(float2 tex0)
{
	float4 final;
	float2 ffrac = mod(tex0+g_fPageOffset[CTX].zw, g_fTexOffset[CTX].xy);
	tex0.xy = g_fPageOffset[CTX].xy * tex0.xy - ffrac * g_fc0.yw;

	if( ffrac.x > g_fTexOffset[CTX].x*g_fc0.w )
		tex0.x += g_fTexOffset[CTX].x*g_fc0.w;
	if( tex0.x >= g_fc0.y ) tex0 += g_fTexOffset[CTX].zw;

	float4 upper = texture(g_sMemory[CTX], g_fTexDims[CTX].xy*tex0+g_fTexDims[CTX].zw);

	// only need alpha
	float index = texture(g_sConv32to16, upper.zyx-g_fc0.z).y + upper.w*g_fc0.w*g_fc0.w;
	return texture(g_sCLUT, index+g_fExactColor.yz);
}

// Shader 5
// used when a 16 bit texture is used a 32bit one
half4 tex2DPS_tex16to32(float2 tex0)
{
	float4 final;
	float2 ffrac = mod(tex0+g_fPageOffset[CTX].zw, g_fTexOffset[CTX].xy);
	//tex0.xy = g_fPageOffset[CTX].xy * tex0.xy - ffrac * g_fc0.yw;
	tex0.y += g_fPageOffset[CTX].y * ffrac.y;

	if( ffrac.x > g_fTexOffset[CTX].z ) {
		tex0.x -= g_fTexOffset[CTX].z;
		tex0.y += g_fTexOffset[CTX].w;
	}

	float fconst = g_fc0.w*g_fc0.w;
	float4 lower = texture(g_sSrcFinal, g_fTexDims[CTX].xy*tex0);
	float4 upper = texture(g_sMemory[CTX], g_fTexDims[CTX].xy*tex0+g_fTexDims[CTX].zw);

	final.zy = texture(g_sConv32to16, lower.zyx).xy + lower.ww*fconst;
	final.xw = texture(g_sConv32to16, upper.zyx).xy + upper.ww*fconst;
	return final;
}

half4 tex2DPS_tex16to32h(float2 tex0)
{
	float4 final =  vec4(0.0, 0.0, 0.0, 0.0);
	return final;
}

//half4 f;
//f.w = old.y > (127.2f/255.0f) ? 1 : 0;
//old.y -= 0.5f * f.w;
//f.xyz = fract(old.yyx*half3(2.002*255.0f/256.0f, 64.025f*255.0f/256.0f, 8.002*255.0f/256.0f));
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
decl_ps2shade_##num(_tex16to32h)

// nearest
#define decl_ps2shade_0(bit) \
float4 ps2shade0##bit( TEX_DECL tex) \
{ \
    return tex2DPS##bit( ps2addr(TEX_XY)); \
}

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
	ftex.xy = tex0 + g_fBilinear.xy * g_fRealTexDims[CTX].zw; \
	ffrac = fract(ftex.xy*g_fRealTexDims[CTX].xy); \
	ftex.xy -= ffrac.xy * g_fRealTexDims[CTX].zw; \
	\
 	ftex.zw = ps2addr(ftex.xy + g_fRealTexDims[CTX].zw); \
 	ftex.xy = ps2addr(ftex.xy); \
 	\
 	PS2MEMCOORD4(ftex, off0, off1); \
	half4 c0 = texture(g_sMemory[CTX], off0.xy); \
	half4 c1 = texture(g_sMemory[CTX], off0.zw); \
	half4 c2 = texture(g_sMemory[CTX], off1.xy); \
	half4 c3 = texture(g_sMemory[CTX], off1.zw); \
	return mix( mix(c0, c1, vec4(ffrac.x)), mix(c2, c3, ffrac.x), vec4(ffrac.y) ); \
}

decl_BilinearFilter(_32, ps2addr)
decl_BilinearFilter(_tex32, ps2addr)
decl_BilinearFilter(_clut32, ps2addr)
decl_BilinearFilter(_tex32to16, ps2addr)
decl_BilinearFilter(_tex16to8h, ps2addr)
decl_BilinearFilter(_tex16to32h, ps2addr)

//TODO! For mip maps, only apply when LOD >= 0
// lcm == 0, LOD = log(1/Q)*L + K, lcm == 1, LOD = K

// bilinear
#define decl_ps2shade_1(bit) \
half4 ps2shade1##bit(TEX_DECL tex) \
{ \
	return BilinearFilter##bit(TEX_XY); \
}

// nearest, mip nearest
#define decl_ps2shade_2(bit) \
half4 ps2shade2##bit(TEX_DECL tex) \
{ \
    return tex2DPS##bit( ps2addr(TEX_XY)); \
}

// nearest, mip linear
#define decl_ps2shade_3(bit) \
half4 ps2shade3##bit(TEX_DECL tex) \
{ \
    return tex2DPS##bit(ps2addr(TEX_XY)); \
}

// linear, mip nearest
#define decl_ps2shade_4(bit) \
half4 ps2shade4##bit(TEX_DECL tex) \
{ \
    return BilinearFilter##bit(TEX_XY); \
}

// linear, mip linear
#define decl_ps2shade_5(bit) \
half4 ps2shade5##bit(TEX_DECL tex) \
{ \
    return BilinearFilter##bit(TEX_XY); \
}

decl_ps2shade(0)
decl_ps2shade(1)
decl_ps2shade(2)
decl_ps2shade(3)
decl_ps2shade(4)
decl_ps2shade(5)


half4 ps2CalcShade(half4 texcol, half4 color)
{
#ifdef TEST_AEM
	if( dot(texcol.xyzw, g_fTestBlack[CTX].xyzw) <= g_fc0.z )
		texcol.w = g_fc0.x;
	else
#endif
		texcol.w = texcol.w * fTexAlpha[CTX].y + fTexAlpha[CTX].x;

	texcol = texcol * (fTexAlpha2[CTX].zzzw * color + fTexAlpha2[CTX].xxxy) + fTexAlpha[CTX].zzzw * color.wwww;

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

#ifdef FRAGMENT_SHADER 			// This is code only for FRAGMENTS (pixel shader)

void RegularPS() {
	// whenever outputting depth, make sure to mult by 255/256 and 1
	gl_FragData[0] = ps2FinalColor(gl_Color);
	DOZWRITE(gl_FragData[1] = gl_TexCoord[0];)
}

#ifdef WRITE_DEPTH

#define DECL_TEXPS(num, bit) \
void Texture##num##bit##PS() \
{ \
	gl_FragData[0] = ps2FinalColor(ps2CalcShade(ps2shade##num##bit(gl_TexCoord[0]), gl_Color)); \
	gl_FragData[1] = gl_TexCoord[1]; \
}

#else

#define DECL_TEXPS(num, bit) \
void Texture##num##bit##PS() \
{ \
	gl_FragData[0] = ps2FinalColor(ps2CalcShade(ps2shade##num##bit(gl_TexCoord[0]), gl_Color)); \
}

#endif


#define DECL_TEXPS_(num) \
DECL_TEXPS(num, _32) \
DECL_TEXPS(num, _tex32) \
DECL_TEXPS(num, _clut32) \
DECL_TEXPS(num, _tex32to16) \
DECL_TEXPS(num, _tex16to8h)

DECL_TEXPS_(0)
DECL_TEXPS_(1)
DECL_TEXPS_(2)
DECL_TEXPS_(3)
DECL_TEXPS_(4)
DECL_TEXPS_(5)

void RegularFogPS() {
	half4 c;
	c.xyz = mix(g_fFogColor.xyz, gl_Color.xyz, vec3(gl_TexCoord[0].x));
	c.w = gl_Color.w;
	gl_FragData[0] = ps2FinalColor(c);
   	DOZWRITE(gl_FragData[1] = gl_TexCoord[1];)
}

#ifdef WRITE_DEPTH

#define DECL_TEXFOGPS(num, bit) \
void TextureFog##num##bit##PS() \
{ \
	half4 c = ps2CalcShade(ps2shade##num##bit(gl_TexCoord[0]), gl_Color); \
	c.xyz = mix(g_fFogColor.xyz, c.xyz, vec3(gl_TexCoord[1].x)); \
	gl_FragData[0] = ps2FinalColor(c); \
   	gl_FragData[1] = gl_TexCoord[2]; \
}

#else

#define DECL_TEXFOGPS(num, bit) \
void TextureFog##num##bit##PS() \
{ \
	half4 c = ps2CalcShade(ps2shade##num##bit(gl_TexCoord[0]), gl_Color); \
	c.xyz = mix(g_fFogColor.xyz, c.xyz, vec3(gl_TexCoord[1].x)); \
	gl_FragData[0] = ps2FinalColor(c); \
}

#endif

#define DECL_TEXFOGPS_(num) \
DECL_TEXFOGPS(num, _32) \
DECL_TEXFOGPS(num, _tex32) \
DECL_TEXFOGPS(num, _clut32) \
DECL_TEXFOGPS(num, _tex32to16) \
DECL_TEXFOGPS(num, _tex16to8h)

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

	ffrac.xy = fract(tex0*g_fRealTexDims[CTX].xy);
	ftex.xy = tex0 - ffrac.xy * g_fRealTexDims[CTX].zw;
	ftex.zw = ftex.xy + g_fRealTexDims[CTX].zw;

	float4 off0, off1;
	ps2memcoord4_fast(ftex, off0, off1);
	half4 c0 = texture(g_sMemory[CTX], off0.xy);
	half4 c1 = texture(g_sMemory[CTX], off0.zw);
	half4 c2 = texture(g_sMemory[CTX], off1.xy);
	half4 c3 = texture(g_sMemory[CTX], off1.zw);

	return mix( mix(c0, c1, vec4(ffrac.x)), mix(c2, c3, vec4(ffrac.x)), vec4(ffrac.y) );
}

void BitBltPS() {
	gl_FragData[0] = texture(g_sMemory[CTX], ps2memcoord(gl_TexCoord[0].xy).xy)*g_fOneColor.xxxy;
}

// used when AA
void BitBltAAPS() {
	gl_FragData[0] = BilinearBitBlt(gl_TexCoord[0].xy) * g_fOneColor.xxxy;
}

void BitBltDepthPS() {
	vec4 data;
	data = texture(g_sMemory[CTX], ps2memcoord(gl_TexCoord[0].xy));
	gl_FragData[0] = data + g_fZBias.y;
	gl_FragDepth   = (log(g_fc0.y + dot(data, g_fBitBltZ)) * g_fOneColor.w) * g_fZMin.y + dot(data, g_fBitBltZ) * g_fZMin.x ;
}

void BitBltDepthMRTPS() {
	vec4 data;
	data = texture(g_sMemory[CTX], ps2memcoord(gl_TexCoord[0].xy));
	gl_FragData[0] = data + g_fZBias.y;
	gl_FragData[1].x = g_fc0.x;
	gl_FragDepth = (log(g_fc0.y + dot(data, g_fBitBltZ)) * g_fOneColor.w) * g_fZMin.y + dot(data, g_fBitBltZ) * g_fZMin.x ;
}

// static const float BlurKernel[9] = {
// 	0.027601,
// 	0.066213,
// 	0.123701,
// 	0.179952,
// 	0.205065,
// 	0.179952,
// 	0.123701,
// 	0.066213,
// 	0.027601
// };

half4 BilinearFloat16(float2 tex0)
{
	return texture(g_sSrcFinal, tex0.xy);
}

void CRTCTargInterPS() {
	float finter = texture(g_sInterlace, gl_TexCoord[1].yy).x * g_fOneColor.z + g_fOneColor.w + g_fc0.w;
	float4 c = BilinearFloat16(gl_TexCoord[0].xy);
	c.w = ( g_fc0.w*c.w * g_fOneColor.x + g_fOneColor.y ) * finter;
	gl_FragData[0] = c;
}

void CRTCTargPS() {
	float4 c = BilinearFloat16(gl_TexCoord[0].xy);
	c.w = g_fc0.w * c.w * g_fOneColor.x + g_fOneColor.y;
	gl_FragData[0] = c;
}

void CRTCInterPS() {
	float finter = texture(g_sInterlace, gl_TexCoord[1].yy).x * g_fOneColor.z + g_fOneColor.w + g_fc0.w;
	float2 filtcoord = trunc(gl_TexCoord[0].xy) * g_fInvTexDims.xy + g_fInvTexDims.zw;
	half4 c = BilinearBitBlt(filtcoord);
	c.w = (c.w * g_fOneColor.x + g_fOneColor.y)*finter;
	gl_FragData[0] = c;
}

// simpler
void CRTCInterPS_Nearest() {
	float finter = texture(g_sInterlace, gl_TexCoord[1].yy).x * g_fOneColor.z + g_fOneColor.w + g_fc0.w;
	half4 c = texture(g_sMemory[CTX], ps2memcoord(gl_TexCoord[0].xy).xy);
	c.w = (c.w * g_fOneColor.x + g_fOneColor.y)*finter;
	gl_FragData[0] = c;
}

void CRTCPS() {
	float2 filtcoord = gl_TexCoord[0].xy * g_fInvTexDims.xy+g_fInvTexDims.zw;
	half4 c = BilinearBitBlt(filtcoord);
	c.w = c.w * g_fOneColor.x + g_fOneColor.y;
	gl_FragData[0] = c;
}

// simpler
void CRTCPS_Nearest() {
	half4 c = texture(g_sMemory[CTX], ps2memcoord(gl_TexCoord[0].xy).xy);
	c.w = c.w * g_fOneColor.x + g_fOneColor.y;
	gl_FragData[0] = c;
}

void CRTC24InterPS() {
	float finter = texture(g_sInterlace, gl_TexCoord[1].yy).x * g_fOneColor.z + g_fOneColor.w + g_fc0.w;
	float2 filtcoord = trunc(gl_TexCoord[0].xy) * g_fInvTexDims.xy + g_fInvTexDims.zw;

	half4 c = texture(g_sMemory[CTX], ps2memcoord(filtcoord).xy);
	c.w = (c.w * g_fOneColor.x + g_fOneColor.y)*finter;
	gl_FragData[0] = c;
}

void CRTC24PS() {
	float2 filtcoord = trunc(gl_TexCoord[0].xy) * g_fInvTexDims.xy + g_fInvTexDims.zw;
	half4 c = texture(g_sMemory[CTX], ps2memcoord(filtcoord).xy);
	c.w = c.w * g_fOneColor.x + g_fOneColor.y;
	gl_FragData[0] = c;
}

void ZeroPS() {
	gl_FragData[0] = g_fOneColor;
}

void BaseTexturePS() {
	gl_FragData[0] = texture(g_sSrcFinal, gl_TexCoord[0].xy) * g_fOneColor;
}

void Convert16to32PS() {
	float4 final;
	float2 ffrac = mod ( gl_TexCoord[0].xy + g_fTexDims[CTX].zw, g_fTexOffset[CTX].xy);
	float2 tex0 = g_fTexDims[CTX].xy * gl_TexCoord[0].xy - ffrac * g_fc0.yw;

	if (ffrac.x > g_fTexOffset[CTX].x*g_fc0.w)
		tex0.x += g_fTexOffset[CTX].x*g_fc0.w;
	if (tex0.x >= g_fc0.y)
		tex0 += g_fTexOffset[CTX].zw;

	float4 lower = texture(g_sSrcFinal, tex0);
	float4 upper = texture(g_sSrcFinal, tex0 + g_fPageOffset[CTX].xy);

	final.zy = texture(g_sConv32to16, lower.zyx).xy + lower.ww*g_fPageOffset[CTX].zw;
	final.xw = texture(g_sConv32to16, upper.zyx).xy + upper.ww*g_fPageOffset[CTX].zw;

	gl_FragData[0]= final;
}

// use when texture is not tiled and converting from 32bit to 16bit
// don't convert on the block level, only on the column level
// so every other 8 pixels, use the upper bits instead of lower
void Convert32to16PS() {
	bool upper = false;
	float2 ffrac = mod(gl_TexCoord[0].xy + g_fTexDims[CTX].zw, g_fTexOffset[CTX].xy);
	float2 tex0 = g_fc0.ww * (gl_TexCoord[0].xy + ffrac);
	if( ffrac.x > g_fTexOffset[CTX].z ) {
		tex0.x -= g_fTexOffset[CTX].z;
		upper = true;
	}
	if( ffrac.y >= g_fTexOffset[CTX].w ) {
		tex0.y -= g_fTexOffset[CTX].w;
		tex0.x += g_fc0.w;
	}

	half4 color = texture(g_sSrcFinal, tex0*g_fTexDims[CTX].xy)*g_fc0.yyyw;
	float2 uv = upper ? color.xw : color.zy;
	gl_FragData[0] = texture(g_sConv16to32, uv*g_fPageOffset[CTX].xy+g_fPageOffset[CTX].zw)*g_fTexDims[CTX].xxxy;
}
#endif 			//FRAGMENT_SHADER

#ifdef VERTEX_SHADER

float4 OutPosition(float4 vertex) {
	float4 Position;
	Position.xy = gl_Vertex.xy * g_fPosXY[CTX].xy + g_fPosXY[CTX].zw;
	Position.z = (log(g_fc0.y + dot(g_fZ, gl_SecondaryColor.zyxw)) * g_fZNorm.x + g_fZNorm.y) * g_fZMin.y + dot(g_fZ, gl_SecondaryColor.zyxw) * g_fZMin.x ;
	Position.w = g_fc0.y;
	return Position;
}

// just smooth shadering
void RegularVS() {
	gl_Position = OutPosition(gl_Vertex);
	gl_FrontColor = gl_Color;
	DOZWRITE(gl_TexCoord[0] = gl_SecondaryColor * g_fZBias.x + g_fZBias.y; gl_TexCoord[0].w = g_fc0.y;)
}

// diffuse texture mapping
void TextureVS() {
	gl_Position = OutPosition(gl_Vertex);
 	gl_FrontColor = gl_Color;
#ifdef PERSPECTIVE_CORRECT_TEX
	gl_TexCoord[0].xyz = gl_MultiTexCoord0.xyz;
#else
	gl_TexCoord[0].xy = gl_MultiTexCoord0.xy/gl_MultiTexCoord0.z;
#endif
 	DOZWRITE(gl_TexCoord[1] = gl_SecondaryColor * g_fZBias.x + g_fZBias.y; gl_TexCoord[1].w = g_fc0.y;)
}

void RegularFogVS() {
	float4 position = OutPosition(gl_Vertex);
	gl_Position = position;
	gl_FrontColor = gl_Color;
    	gl_TexCoord[0].x = position.z * g_fBilinear.w;
	DOZWRITE(gl_TexCoord[1] = gl_SecondaryColor * g_fZBias.x + g_fZBias.y; gl_TexCoord[1].w = g_fc0.y;)
}

void TextureFogVS() {
	gl_Position = OutPosition(gl_Vertex);
	gl_FrontColor = gl_Color;
#ifdef PERSPECTIVE_CORRECT_TEX
    	gl_TexCoord[0].xyz = gl_MultiTexCoord0.xyz;
#else
	gl_TexCoord[0].xy  = gl_MultiTexCoord0.xy / gl_MultiTexCoord0.z;
#endif
	gl_TexCoord[1].x = gl_Vertex.z * g_fBilinear.w;
    	DOZWRITE(gl_TexCoord[2] = gl_SecondaryColor * g_fZBias.x + g_fZBias.y; gl_TexCoord[2].w = g_fc0.y;)
}

void BitBltVS() {
	vec4 position;
	position.xy = gl_Vertex.xy * g_fBitBltPos.xy + g_fBitBltPos.zw;
	position.zw = g_fc0.xy;
	gl_Position = position;

	gl_TexCoord[0].xy = gl_MultiTexCoord0.xy * g_fBitBltTex.xy + g_fBitBltTex.zw;
	gl_TexCoord[1].xy = position.xy * g_fBitBltTrans.xy + g_fBitBltTrans.zw;
}

#endif
