/*===============================================================================*\
|#########################	   [GSdx FX 2.00 Revised]     ########################|
|###########################	    By Asmodean	        ##########################|
||																				 ||
||		  This program is free software; you can redistribute it and/or			 ||
||		  modify it under the terms of the GNU General Public License			 ||
||		  as published by the Free Software Foundation; either version 2		 ||
||		  of the License, or (at your option) any later version.				 ||
||																				 ||
||		  This program is distributed in the hope that it will be useful,		 ||
||		  but WITHOUT ANY WARRANTY; without even the implied warranty of		 ||
||		  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the			 ||
||		  GNU General Public License for more details. (c)2014					 ||
||																				 ||
|#################################################################################|
\*===============================================================================*/
#include "GSdx_FX_Settings.ini"

/*------------------------------------------------------------------------------
							 [GLOBALS|FUNCTIONS]
------------------------------------------------------------------------------*/

Texture2D Texture : register(t0);
SamplerState TextureSampler : register(s0)
{
	Filter = Anisotropic;
	MaxAnisotropy = 16;
	AddressU = Clamp;
	AddressV = Clamp;
	MipLODBias = 0;
};

cbuffer cb0
{
	float4 _rcpFrame : VIEWPORT : register(c0);
	static const float GammaConst = 2.2;
};

struct VS_INPUT
{
	float4 p : POSITION;
	float2 t : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 p : SV_Position;
	float2 t : TEXCOORD0;
};

struct PS_OUTPUT
{
	float4 c : SV_Target0;
};

float TrueLuminance(float3 color)
{
	float maxRGB;
	float minRGB;
	float r = color.x;
	float g = color.y;
	float b = color.z;

	if (r >= g) { maxRGB = r; }
	if (r >= b) { maxRGB = r; }
	if (g >= r) { maxRGB = g; }
	if (g >= b) { maxRGB = g; }
	if (b >= r) { maxRGB = b; }
	if (b >= g) { maxRGB = b; }

	if (r <= g) { minRGB = r; }
	if (r <= b) { minRGB = r; }
	if (g <= r) { minRGB = g; }
	if (g <= b) { minRGB = g; }
	if (b <= r) { minRGB = b; }
	if (b <= g) { minRGB = b; }

	float lumin = ((maxRGB + minRGB)/2);
	return lumin;
}

float RGBLuminance(float3 color)
{
	const float3 lumCoeff = float3(0.2126729, 0.7151522, 0.0721750);
	return dot(color.rgb, lumCoeff);
}

float3 RGBGammaToLinear(float3 color, float gamma)
{
	color = abs(color);
	color.r = (color.r <= 0.0404482362771082) ? saturate(color.r / 12.92) : 
	saturate(pow((color.r + 0.055) / 1.055, gamma));

	color.g = (color.g <= 0.0404482362771082) ? saturate(color.g / 12.92) : 
	saturate(pow((color.g + 0.055) / 1.055, gamma));

	color.b = (color.b <= 0.0404482362771082) ? saturate(color.b / 12.92) : 
	saturate(pow((color.b + 0.055) / 1.055, gamma));

	return color;
}

float3 LinearToRGBGamma(float3 color, float gamma)
{
	color = abs(color);
	color.r = (color.r <= 0.00313066844250063) ? saturate(color.r * 12.92) : 1.055 * 
	saturate(pow(color.r, 1.0 / gamma)) - 0.055;

	color.g = (color.g <= 0.00313066844250063) ? saturate(color.g * 12.92) : 1.055 * 
	saturate(pow(color.g, 1.0 / gamma)) - 0.055;

	color.b = (color.b <= 0.00313066844250063) ? saturate(color.b * 12.92) : 1.055 * 
	saturate(pow(color.b, 1.0 / gamma)) - 0.055;

	return color;
}

#define PixelSize float2(_rcpFrame.x, _rcpFrame.y)
#define GammaCorrection(color, gamma) pow(color, gamma)
#define InverseGammaCorrection(color, gamma) pow(color, 1.0/gamma)

/*------------------------------------------------------------------------------
							[VERTEX CODE SECTION]
------------------------------------------------------------------------------*/

VS_OUTPUT vs_main(VS_INPUT input)
{	
	VS_OUTPUT output;  

	output.p = input.p;
	output.t = input.t;

	return output;
}

/*------------------------------------------------------------------------------
						 [GAMMA PREPASS CODE SECTION]
------------------------------------------------------------------------------*/

float4 PreGammaPass(float4 color, float2 uv0)
{	
	color = Texture.Sample(TextureSampler, uv0);
	color.rgb = RGBGammaToLinear(color.rgb, GammaConst);
	color.rgb = LinearToRGBGamma(color.rgb, GammaConst);
	color.a = RGBLuminance(color.rgb);

	return color;
}

/*------------------------------------------------------------------------------
                             [FXAA CODE SECTION]
------------------------------------------------------------------------------*/

#if (UHQ_FXAA == 1)
#if (SHADER_MODEL >= 0x500)
	#define FXAA_HLSL_5 1
	#define FXAA_GATHER4_ALPHA 1
#elif (SHADER_MODEL >= 0x400)
	#define FXAA_HLSL_4 1
	#define FXAA_GATHER4_ALPHA 0
#else
	#define FXAA_HLSL_3 1
	#define FXAA_GATHER4_ALPHA 0
#endif

#if (FxaaQuality == 4)
	#define FxaaEdgeThreshold 0.033
	#define FxaaEdgeThresholdMin 0.0
	#define FXAA_QUALITY__PS 14
	#define FXAA_QUALITY__P0 1.0
	#define FXAA_QUALITY__P1 1.0
	#define FXAA_QUALITY__P2 1.0
	#define FXAA_QUALITY__P3 1.0
	#define FXAA_QUALITY__P4 1.0
	#define FXAA_QUALITY__P5 1.5
	#define FXAA_QUALITY__P6 2.0
	#define FXAA_QUALITY__P7 2.0
	#define FXAA_QUALITY__P8 2.0
	#define FXAA_QUALITY__P9 2.0
	#define FXAA_QUALITY__P10 4.0
	#define FXAA_QUALITY__P11 8.0
	#define FXAA_QUALITY__P12 8.0

#elif (FxaaQuality == 3)
	#define FxaaEdgeThreshold 0.125
	#define FxaaEdgeThresholdMin 0.0312
    #define FXAA_QUALITY__PS 12
    #define FXAA_QUALITY__P0 1.0
    #define FXAA_QUALITY__P1 1.5
    #define FXAA_QUALITY__P2 2.0
    #define FXAA_QUALITY__P3 2.0
    #define FXAA_QUALITY__P4 2.0
    #define FXAA_QUALITY__P5 2.0
    #define FXAA_QUALITY__P6 2.0
    #define FXAA_QUALITY__P7 2.0
    #define FXAA_QUALITY__P8 2.0
    #define FXAA_QUALITY__P9 2.0
    #define FXAA_QUALITY__P10 4.0
    #define FXAA_QUALITY__P11 8.0

#elif (FxaaQuality == 2)
	#define FxaaEdgeThreshold 0.166
	#define FxaaEdgeThresholdMin 0.0625
	#define FXAA_QUALITY__PS 10
    #define FXAA_QUALITY__P0 1.0
    #define FXAA_QUALITY__P1 1.5
    #define FXAA_QUALITY__P2 2.0
    #define FXAA_QUALITY__P3 2.0
    #define FXAA_QUALITY__P4 2.0
    #define FXAA_QUALITY__P5 2.0
    #define FXAA_QUALITY__P6 2.0
    #define FXAA_QUALITY__P7 2.0
    #define FXAA_QUALITY__P8 4.0
    #define FXAA_QUALITY__P9 8.0

#elif (FxaaQuality == 1)
	#define FxaaEdgeThreshold 0.250
	#define FxaaEdgeThresholdMin 0.0833
	#define FXAA_QUALITY__PS 8
    #define FXAA_QUALITY__P0 1.0
    #define FXAA_QUALITY__P1 1.5
    #define FXAA_QUALITY__P2 2.0
    #define FXAA_QUALITY__P3 2.0
    #define FXAA_QUALITY__P4 2.0
    #define FXAA_QUALITY__P5 2.0
    #define FXAA_QUALITY__P6 4.0
    #define FXAA_QUALITY__P7 12.0
#endif

#if (FXAA_HLSL_5 == 1)
    struct FxaaTex { SamplerState smpl; Texture2D tex; };
    #define FxaaTexTop(t, p) t.tex.SampleLevel(t.smpl, p, 0.0)
    #define FxaaTexOff(t, p, o, r) t.tex.SampleLevel(t.smpl, p, 0.0, o)
    #define FxaaTexAlpha4(t, p) t.tex.GatherAlpha(t.smpl, p)
    #define FxaaTexOffAlpha4(t, p, o) t.tex.GatherAlpha(t.smpl, p, o)
	#define FxaaDiscard clip(-1)
	#define FxaaSat(x) saturate(x)

#elif (FXAA_HLSL_4 == 1)
    struct FxaaTex { SamplerState smpl; Texture2D tex; };
    #define FxaaTexTop(t, p) t.tex.SampleLevel(t.smpl, p, 0.0)
    #define FxaaTexOff(t, p, o, r) t.tex.SampleLevel(t.smpl, p, 0.0, o)
	#define FxaaDiscard clip(-1)
	#define FxaaSat(x) saturate(x)

#elif (FXAA_HLSL_3 == 1)
	#define FxaaTex sampler2D
	#define int2 float2
	#define FxaaSat(x) saturate(x)
	#define FxaaTexTop(t, p) tex2Dlod(t, float4(p, 0.0, 0.0))
	#define FxaaTexOff(t, p, o, r) tex2Dlod(t, float4(p + (o * r), 0, 0))
#endif

float FxaaLuma(float4 rgba)
{ 
	rgba.w = RGBLuminance(rgba.xyz);
	return rgba.w; 
}

float4 FxaaPixelShader(float2 pos, FxaaTex tex, float2 fxaaRcpFrame, float fxaaSubpix, float fxaaEdgeThreshold, float fxaaEdgeThresholdMin)
{
    float2 posM;
    posM.x = pos.x;
    posM.y = pos.y;

    #if (FXAA_GATHER4_ALPHA == 1)
	float4 rgbyM = FxaaTexTop(tex, posM);
	float4 luma4A = FxaaTexAlpha4(tex, posM);
	float4 luma4B = FxaaTexOffAlpha4(tex, posM, int2(-1, -1));
	rgbyM.w = RGBLuminance(rgbyM.xyz);

	#define lumaM rgbyM.w
	#define lumaE luma4A.z
	#define lumaS luma4A.x
	#define lumaSE luma4A.y
	#define lumaNW luma4B.w
	#define lumaN luma4B.z
	#define lumaW luma4B.x
    #else

	float4 rgbyM = FxaaTexTop(tex, posM);
	rgbyM.w = RGBLuminance(rgbyM.xyz);
	#define lumaM rgbyM.w

	float lumaS = FxaaLuma(FxaaTexOff(tex, posM, int2( 0, 1), fxaaRcpFrame.xy));
	float lumaE = FxaaLuma(FxaaTexOff(tex, posM, int2( 1, 0), fxaaRcpFrame.xy));
	float lumaN = FxaaLuma(FxaaTexOff(tex, posM, int2( 0,-1), fxaaRcpFrame.xy));
	float lumaW = FxaaLuma(FxaaTexOff(tex, posM, int2(-1, 0), fxaaRcpFrame.xy));
    #endif

    float maxSM = max(lumaS, lumaM);
    float minSM = min(lumaS, lumaM);
    float maxESM = max(lumaE, maxSM);
    float minESM = min(lumaE, minSM);
    float maxWN = max(lumaN, lumaW);
    float minWN = min(lumaN, lumaW);
    float rangeMax = max(maxWN, maxESM);
    float rangeMin = min(minWN, minESM);
    float rangeMaxScaled = rangeMax * fxaaEdgeThreshold;
    float range = rangeMax - rangeMin;
    float rangeMaxClamped = max(fxaaEdgeThresholdMin, rangeMaxScaled);
    bool earlyExit = range < rangeMaxClamped;

    #if (FxaaEarlyExit == 1)
	if(earlyExit)
		{ return rgbyM; }
	#endif

    #if (FXAA_GATHER4_ALPHA == 0)
	float lumaNW = FxaaLuma(FxaaTexOff(tex, posM, int2(-1,-1), fxaaRcpFrame.xy));
	float lumaSE = FxaaLuma(FxaaTexOff(tex, posM, int2( 1, 1), fxaaRcpFrame.xy));
	float lumaNE = FxaaLuma(FxaaTexOff(tex, posM, int2( 1,-1), fxaaRcpFrame.xy));
	float lumaSW = FxaaLuma(FxaaTexOff(tex, posM, int2(-1, 1), fxaaRcpFrame.xy));
	#else
	float lumaNE = FxaaLuma(FxaaTexOff(tex, posM, int2( 1,-1), fxaaRcpFrame.xy));
	float lumaSW = FxaaLuma(FxaaTexOff(tex, posM, int2(-1, 1), fxaaRcpFrame.xy));
	#endif

    float lumaNS = lumaN + lumaS;
    float lumaWE = lumaW + lumaE;
    float subpixRcpRange = 1.0/range;
    float subpixNSWE = lumaNS + lumaWE;
    float edgeHorz1 = (-2.0 * lumaM) + lumaNS;
    float edgeVert1 = (-2.0 * lumaM) + lumaWE;

    float lumaNESE = lumaNE + lumaSE;
    float lumaNWNE = lumaNW + lumaNE;
    float edgeHorz2 = (-2.0 * lumaE) + lumaNESE;
    float edgeVert2 = (-2.0 * lumaN) + lumaNWNE;

    float lumaNWSW = lumaNW + lumaSW;
    float lumaSWSE = lumaSW + lumaSE;
    float edgeHorz4 = (abs(edgeHorz1) * 2.0) + abs(edgeHorz2);
    float edgeVert4 = (abs(edgeVert1) * 2.0) + abs(edgeVert2);
    float edgeHorz3 = (-2.0 * lumaW) + lumaNWSW;
    float edgeVert3 = (-2.0 * lumaS) + lumaSWSE;
    float edgeHorz = abs(edgeHorz3) + edgeHorz4;
    float edgeVert = abs(edgeVert3) + edgeVert4;

    float subpixNWSWNESE = lumaNWSW + lumaNESE;
    float lengthSign = fxaaRcpFrame.x;
    bool horzSpan = edgeHorz >= edgeVert;
    float subpixA = subpixNSWE * 2.0 + subpixNWSWNESE;

    if(!horzSpan) lumaN = lumaW;
    if(!horzSpan) lumaS = lumaE;
    if(horzSpan) lengthSign = fxaaRcpFrame.y;
    float subpixB = (subpixA * (1.0/12.0)) - lumaM;

    float gradientN = lumaN - lumaM;
    float gradientS = lumaS - lumaM;
    float lumaNN = lumaN + lumaM;
    float lumaSS = lumaS + lumaM;
    bool pairN = abs(gradientN) >= abs(gradientS);
    float gradient = max(abs(gradientN), abs(gradientS));
    if(pairN) lengthSign = -lengthSign;
    float subpixC = FxaaSat(abs(subpixB) * subpixRcpRange);

    float2 posB;
    posB.x = posM.x;
    posB.y = posM.y;
    float2 offNP;
    offNP.x = (!horzSpan) ? 0.0 : fxaaRcpFrame.x;
    offNP.y = ( horzSpan) ? 0.0 : fxaaRcpFrame.y;
    if(!horzSpan) posB.x += lengthSign * 0.5;
    if( horzSpan) posB.y += lengthSign * 0.5;

    float2 posN;
    posN.x = posB.x - offNP.x * FXAA_QUALITY__P0;
    posN.y = posB.y - offNP.y * FXAA_QUALITY__P0;
    float2 posP;
    posP.x = posB.x + offNP.x * FXAA_QUALITY__P0;
    posP.y = posB.y + offNP.y * FXAA_QUALITY__P0;
    float subpixD = ((-2.0)*subpixC) + 3.0;
    float lumaEndN = FxaaLuma(FxaaTexTop(tex, posN));
    float subpixE = subpixC * subpixC;
    float lumaEndP = FxaaLuma(FxaaTexTop(tex, posP));

    if(!pairN) lumaNN = lumaSS;
    float gradientScaled = gradient * 1.0/4.0;
    float lumaMM = lumaM - lumaNN * 0.5;
    float subpixF = subpixD * subpixE;
    bool lumaMLTZero = lumaMM < 0.0;

    lumaEndN -= lumaNN * 0.5;
    lumaEndP -= lumaNN * 0.5;
    bool doneN = abs(lumaEndN) >= gradientScaled;
    bool doneP = abs(lumaEndP) >= gradientScaled;
    if(!doneN) posN.x -= offNP.x * FXAA_QUALITY__P1;
    if(!doneN) posN.y -= offNP.y * FXAA_QUALITY__P1;
    bool doneNP = (!doneN) || (!doneP);
    if(!doneP) posP.x += offNP.x * FXAA_QUALITY__P1;
    if(!doneP) posP.y += offNP.y * FXAA_QUALITY__P1;

	if(doneNP) {
	if(!doneN) lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
	if(!doneP) lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
	if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
	if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
	doneN = abs(lumaEndN) >= gradientScaled;
	doneP = abs(lumaEndP) >= gradientScaled;
	if(!doneN) posN.x -= offNP.x * FXAA_QUALITY__P2;
	if(!doneN) posN.y -= offNP.y * FXAA_QUALITY__P2;
	doneNP = (!doneN) || (!doneP);
	if(!doneP) posP.x += offNP.x * FXAA_QUALITY__P2;
	if(!doneP) posP.y += offNP.y * FXAA_QUALITY__P2;

	#if (FXAA_QUALITY__PS > 3)
	if(doneNP) {
	if(!doneN) lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
	if(!doneP) lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
	if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
	if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
	doneN = abs(lumaEndN) >= gradientScaled;
	doneP = abs(lumaEndP) >= gradientScaled;
	if(!doneN) posN.x -= offNP.x * FXAA_QUALITY__P3;
	if(!doneN) posN.y -= offNP.y * FXAA_QUALITY__P3;
	doneNP = (!doneN) || (!doneP);
	if(!doneP) posP.x += offNP.x * FXAA_QUALITY__P3;
	if(!doneP) posP.y += offNP.y * FXAA_QUALITY__P3;

	#if (FXAA_QUALITY__PS > 4)
	if(doneNP) {
	if(!doneN) lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
	if(!doneP) lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
	if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
	if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
	doneN = abs(lumaEndN) >= gradientScaled;
	doneP = abs(lumaEndP) >= gradientScaled;
	if(!doneN) posN.x -= offNP.x * FXAA_QUALITY__P4;
	if(!doneN) posN.y -= offNP.y * FXAA_QUALITY__P4;
	doneNP = (!doneN) || (!doneP);
	if(!doneP) posP.x += offNP.x * FXAA_QUALITY__P4;
	if(!doneP) posP.y += offNP.y * FXAA_QUALITY__P4;

	#if (FXAA_QUALITY__PS > 5)
	if(doneNP) {
	if(!doneN) lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
	if(!doneP) lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
	if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
	if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
	doneN = abs(lumaEndN) >= gradientScaled;
	doneP = abs(lumaEndP) >= gradientScaled;
	if(!doneN) posN.x -= offNP.x * FXAA_QUALITY__P5;
	if(!doneN) posN.y -= offNP.y * FXAA_QUALITY__P5;
	doneNP = (!doneN) || (!doneP);
	if(!doneP) posP.x += offNP.x * FXAA_QUALITY__P5;
	if(!doneP) posP.y += offNP.y * FXAA_QUALITY__P5;

	#if (FXAA_QUALITY__PS > 6)
	if(doneNP) {
	if(!doneN) lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
	if(!doneP) lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
	if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
	if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
	doneN = abs(lumaEndN) >= gradientScaled;
	doneP = abs(lumaEndP) >= gradientScaled;
	if(!doneN) posN.x -= offNP.x * FXAA_QUALITY__P6;
	if(!doneN) posN.y -= offNP.y * FXAA_QUALITY__P6;
	doneNP = (!doneN) || (!doneP);
	if(!doneP) posP.x += offNP.x * FXAA_QUALITY__P6;
	if(!doneP) posP.y += offNP.y * FXAA_QUALITY__P6;

	#if (FXAA_QUALITY__PS > 7)
	if(doneNP) {
	if(!doneN) lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
	if(!doneP) lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
	if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
	if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
	doneN = abs(lumaEndN) >= gradientScaled;
	doneP = abs(lumaEndP) >= gradientScaled;
	if(!doneN) posN.x -= offNP.x * FXAA_QUALITY__P7;
	if(!doneN) posN.y -= offNP.y * FXAA_QUALITY__P7;
	doneNP = (!doneN) || (!doneP);
	if(!doneP) posP.x += offNP.x * FXAA_QUALITY__P7;
	if(!doneP) posP.y += offNP.y * FXAA_QUALITY__P7;

	#if (FXAA_QUALITY__PS > 8)
	if(doneNP) {
	if(!doneN) lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
	if(!doneP) lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
	if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
	if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
	doneN = abs(lumaEndN) >= gradientScaled;
	doneP = abs(lumaEndP) >= gradientScaled;
	if(!doneN) posN.x -= offNP.x * FXAA_QUALITY__P8;
	if(!doneN) posN.y -= offNP.y * FXAA_QUALITY__P8;
	doneNP = (!doneN) || (!doneP);
	if(!doneP) posP.x += offNP.x * FXAA_QUALITY__P8;
	if(!doneP) posP.y += offNP.y * FXAA_QUALITY__P8;

	#if (FXAA_QUALITY__PS > 9)
	if(doneNP) {
	if(!doneN) lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
	if(!doneP) lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
	if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
	if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
	doneN = abs(lumaEndN) >= gradientScaled;
	doneP = abs(lumaEndP) >= gradientScaled;
	if(!doneN) posN.x -= offNP.x * FXAA_QUALITY__P9;
	if(!doneN) posN.y -= offNP.y * FXAA_QUALITY__P9;
	doneNP = (!doneN) || (!doneP);
	if(!doneP) posP.x += offNP.x * FXAA_QUALITY__P9;
	if(!doneP) posP.y += offNP.y * FXAA_QUALITY__P9;

	#if (FXAA_QUALITY__PS > 10)
	if(doneNP) {
	if(!doneN) lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
	if(!doneP) lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
	if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
	if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
	doneN = abs(lumaEndN) >= gradientScaled;
	doneP = abs(lumaEndP) >= gradientScaled;
	if(!doneN) posN.x -= offNP.x * FXAA_QUALITY__P10;
	if(!doneN) posN.y -= offNP.y * FXAA_QUALITY__P10;
	doneNP = (!doneN) || (!doneP);
	if(!doneP) posP.x += offNP.x * FXAA_QUALITY__P10;
	if(!doneP) posP.y += offNP.y * FXAA_QUALITY__P10;

	#if (FXAA_QUALITY__PS > 11)
	if(doneNP) {
	if(!doneN) lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
	if(!doneP) lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
	if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
	if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
	doneN = abs(lumaEndN) >= gradientScaled;
	doneP = abs(lumaEndP) >= gradientScaled;
	if(!doneN) posN.x -= offNP.x * FXAA_QUALITY__P11;
	if(!doneN) posN.y -= offNP.y * FXAA_QUALITY__P11;
	doneNP = (!doneN) || (!doneP);
	if(!doneP) posP.x += offNP.x * FXAA_QUALITY__P11;
	if(!doneP) posP.y += offNP.y * FXAA_QUALITY__P11;

	#if (FXAA_QUALITY__PS > 12)
	if(doneNP) {
	if(!doneN) lumaEndN = FxaaLuma(FxaaTexTop(tex, posN.xy));
	if(!doneP) lumaEndP = FxaaLuma(FxaaTexTop(tex, posP.xy));
	if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
	if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
	doneN = abs(lumaEndN) >= gradientScaled;
	doneP = abs(lumaEndP) >= gradientScaled;
	if(!doneN) posN.x -= offNP.x * FXAA_QUALITY__P12;
	if(!doneN) posN.y -= offNP.y * FXAA_QUALITY__P12;
	doneNP = (!doneN) || (!doneP);
	if(!doneP) posP.x += offNP.x * FXAA_QUALITY__P12;
	if(!doneP) posP.y += offNP.y * FXAA_QUALITY__P12;
	}
	#endif
	}
	#endif
	}
	#endif
	}
	#endif
	}
	#endif
	}
	#endif
	}
	#endif
	}
	#endif
	}
	#endif
	}
	#endif
	}

    float dstN = posM.x - posN.x;
    float dstP = posP.x - posM.x;
    if(!horzSpan) dstN = posM.y - posN.y;
    if(!horzSpan) dstP = posP.y - posM.y;

    bool goodSpanN = (lumaEndN < 0.0) != lumaMLTZero;
    float spanLength = (dstP + dstN);
    bool goodSpanP = (lumaEndP < 0.0) != lumaMLTZero;
    float spanLengthRcp = 1.0/spanLength;

    bool directionN = dstN < dstP;
    float dst = min(dstN, dstP);
    bool goodSpan = directionN ? goodSpanN : goodSpanP;
    float subpixG = subpixF * subpixF;
    float pixelOffset = (dst * (-spanLengthRcp)) + 0.5;
    float subpixH = subpixG * fxaaSubpix;

    float pixelOffsetGood = goodSpan ? pixelOffset : 0.0;
    float pixelOffsetSubpix = max(pixelOffsetGood, subpixH);
    if(!horzSpan) posM.x += pixelOffsetSubpix * lengthSign;
    if( horzSpan) posM.y += pixelOffsetSubpix * lengthSign;

	return float4(FxaaTexTop(tex, posM).xyz, lumaM);
}

float4 FxaaPass(float4 FxaaColor : COLOR0, float2 uv0 : TEXCOORD0) : SV_Target0
{
	FxaaTex tex;
	tex.tex = Texture;
	tex.smpl = TextureSampler;
	
	Texture.GetDimensions(PixelSize.x, PixelSize.y);
	FxaaColor = FxaaPixelShader(uv0, tex, 1.0/PixelSize.xy, FxaaSubpixMax, FxaaEdgeThreshold, FxaaEdgeThresholdMin);

	return FxaaColor;
}
#endif

/*------------------------------------------------------------------------------
                       [TEXTURE FILTERING FUNCTIONS]
------------------------------------------------------------------------------*/

float BSpline(float x)
{
	float f = x;
	
	if(f < 0.0)
	{
		f = -f;
	}
	if(f >= 0.0 && f <= 1.0)
	{
		return (2.0 / 3.0) + (0.5) * (f* f * f) - (f*f);
	}
	else if(f > 1.0 && f <= 2.0)
	{
		return 1.0 / 6.0 * pow(( 2.0 - f  ), 3.0);
	}
	return 1.0;
}

float CatMullRom(float x)
{
    float b = 0.0;
    float c = 0.5;
    float f = x;

    if( f < 0.0 )
    {
        f = -f;
    }
    if( f < 1.0 )
    {
        return ((12.0 - 9.0 * b - 6.0 * c) * (f * f * f) +
				(-18.0 + 12.0 * b + 6.0 * c) * (f * f) +
				(6.0 - 2.0 * b) ) / 6.0;
    }
    else if( f >= 1.0 && f < 2.0 )
    {
        return (( -b - 6.0 * c) * (f * f * f)
            + 	(6.0 * b + 30.0 * c) * (f *f) +
				(- (12.0 * b) - 48.0 * c ) * f +
				8.0 * b + 24.0 * c)/ 6.0;
    }
    else
    {
        return 0.0;
    }
}

float Bell(float x)
{
	float f = (x / 2.0) * 1.5;

	if(f > -1.5 && f < -0.5)
	{
		return(0.5 * pow(f + 1.5, 2.0));
	}
	else if(f > -0.5 && f < 0.5)
	{
		return 3.0 / 4.0 - (f * f);
	}
	else if(( f > 0.5 && f < 1.5 ))
	{
		return(0.5 * pow(f - 1.5, 2.0));
	}
	return 0.0;
}

float Triangular(float x)
{
	x = x / 2.0;

	if(x < 0.0)
	{
		return (x + 1.0);
	}
	else
	{
		return (1.0 - x);
	}
	return 0.0;
}

float Cubic(float x)
{
	float x2 = x * x;
	float x3 = x2 * x;

	float cx =  -x3 + 3.0 * x2 - 3.0 * x + 1.0;
	float cy =  3.0 * x3 - 6.0 * x2 + 4.0;
	float cz = -3.0 * x3 + 3.0 * x2 + 3.0 * x + 1.0;
	float cw =  x3;

	return (lerp(cx, cy, 0.5) + lerp(cz, cw, 0.5)) / 6.0;
}

/*------------------------------------------------------------------------------
                       [BILINEAR FILTERING CODE SECTION]
------------------------------------------------------------------------------*/

#if (BILINEAR_FILTERING == 1)
float4 SampleBiLinear(SamplerState texSample, float2 uv0 : TEXCOORD0) : SV_Target0
{
	Texture.GetDimensions(PixelSize.x, PixelSize.y);
	
	float texelSizeX = 1.0 / PixelSize.x;
	float texelSizeY = 1.0 / PixelSize.y;

	int nX = int(uv0.x * PixelSize.x);
	int nY = int(uv0.y * PixelSize.y);

	float2 uvCoord = float2((float(nX) + OffsetAmount) / PixelSize.x,
							(float(nY) + OffsetAmount) / PixelSize.y);

	// Take nearest two data in current row.
    float4 SampleA = Texture.Sample(texSample, uvCoord);
    float4 SampleB = Texture.Sample(texSample, uvCoord + float2(texelSizeX, 0.0));

	// Take nearest two data in bottom row.
    float4 SampleC = Texture.Sample(texSample, uvCoord + float2(0.0, texelSizeY));
    float4 SampleD = Texture.Sample(texSample, uvCoord + float2(texelSizeX , texelSizeY));

    float LX = frac(uv0.x * PixelSize.x); //Get Interpolation factor for X direction.

	// Interpolate in X direction.
    float4 InterpolateA = lerp(SampleA, SampleB, LX); //Top row in X direction.
    float4 InterpolateB = lerp(SampleC, SampleD, LX); //Bottom row in X direction.

    float LY = frac(uv0.y * PixelSize.y); //Get Interpolation factor for Y direction.

    return lerp(InterpolateA, InterpolateB, LY); //Interpolate in Y direction.
}

float4 BiLinearPass(float4 color : COLOR0, float2 uv0 : TEXCOORD0)
{
   float4 bilinear = SampleBiLinear(TextureSampler, uv0);
   color = lerp(color, bilinear, FilterStrength);
   
   return color;
}
#endif

/*------------------------------------------------------------------------------
                       [BICUBIC FILTERING CODE SECTION]
------------------------------------------------------------------------------*/

#if (BICUBIC_FILTERING == 1)
float4 BiCubicFilter(SamplerState texSample, float2 uv0 : TEXCOORD0) : SV_Target0
{
	Texture.GetDimensions(PixelSize.x, PixelSize.y);
	
	float texelSizeX = 1.0 / PixelSize.x;
	float texelSizeY = 1.0 / PixelSize.y;

	float4 nSum = (float4)0.0;
	float4 nDenom = (float4)0.0;

	float a = frac(uv0.x * PixelSize.x);
	float b = frac(uv0.y * PixelSize.y);

	int nX = int(uv0.x * PixelSize.x);
	int nY = int(uv0.y * PixelSize.y);
	
	float2 uvCoord = float2( float(nX) / PixelSize.x + PixelOffset / PixelSize.x,
							 float(nY) / PixelSize.y + PixelOffset / PixelSize.y );

    for(int m = -1; m <= 2; m++)
    {
        for(int n = -1; n <= 2; n++)
        {
			float4 Samples = Texture.Sample(texSample, uvCoord +
			float2(texelSizeX * float(m), texelSizeY * float(n)));

			float vc1  = Interpolation(float(m) - a);
			float4 vecCoeff1 = float4(vc1, vc1, vc1, vc1);

			float vc2 = Interpolation(-(float(n) - b));
			float4 vecCoeff2 = float4(vc2, vc2, vc2, vc2);

            nSum = nSum + (Samples * vecCoeff2 * vecCoeff1);
            nDenom = nDenom + (vecCoeff2 * vecCoeff1);
        }
    }
	return nSum / nDenom;
}

float4 BiCubicPass(float4 color : COLOR0, float2 uv0 : TEXCOORD0) : SV_Target0
{
   float4 bicubic = BiCubicFilter(TextureSampler, uv0);
   color = lerp(color, bicubic, BicubicStrength);
   return color;
}
#endif

/*------------------------------------------------------------------------------
                       [GAUSSIAN FILTERING CODE SECTION]
------------------------------------------------------------------------------*/

#if (GAUSSIAN_FILTERING == 1)
float4 GaussianPass(float4 color : COLOR0, float2 uv0 : TEXCOORD0) : SV_Target0
{
	Texture.GetDimensions(PixelSize.x, PixelSize.y);
	
	float2 dx = float2(1.0 / PixelSize.x * GaussianSpread, 0.0);
	float2 dy = float2(0.0, 1.0 / PixelSize.y * GaussianSpread);

	float2 dx2 = 2.0 * dx;
	float2 dy2 = 2.0 * dy;
	
	float4 gaussian = Texture.Sample(TextureSampler, uv0);

	gaussian += Texture.Sample(TextureSampler, uv0 -dx2 +dy2); 
	gaussian += Texture.Sample(TextureSampler, uv0 -dx +dy2); 
	gaussian += Texture.Sample(TextureSampler, uv0 +dy2); 
	gaussian += Texture.Sample(TextureSampler, uv0 +dx +dy2); 
	gaussian += Texture.Sample(TextureSampler, uv0 +dx2 +dy2); 

	gaussian += Texture.Sample(TextureSampler, uv0 -dx2 +dy);
	gaussian += Texture.Sample(TextureSampler, uv0 -dx +dy);
	gaussian += Texture.Sample(TextureSampler, uv0 +dy);
	gaussian += Texture.Sample(TextureSampler, uv0 +dx +dy);
	gaussian += Texture.Sample(TextureSampler, uv0 +dx2 +dy);

	gaussian += Texture.Sample(TextureSampler, uv0 -dx2);
	gaussian += Texture.Sample(TextureSampler, uv0 -dx);	
	gaussian += Texture.Sample(TextureSampler, uv0 +dx);
	gaussian += Texture.Sample(TextureSampler, uv0 +dx2);

	gaussian += Texture.Sample(TextureSampler, uv0 -dx2 -dy);
	gaussian += Texture.Sample(TextureSampler, uv0 -dx -dy);
	gaussian += Texture.Sample(TextureSampler, uv0 -dy);
	gaussian += Texture.Sample(TextureSampler, uv0 +dx -dy);
	gaussian += Texture.Sample(TextureSampler, uv0 +dx2 -dy);

	gaussian += Texture.Sample(TextureSampler, uv0 -dx2 -dy2);
	gaussian += Texture.Sample(TextureSampler, uv0 -dx -dy2);
	gaussian += Texture.Sample(TextureSampler, uv0 -dy2);
	gaussian += Texture.Sample(TextureSampler, uv0 +dx -dy2);
	gaussian += Texture.Sample(TextureSampler, uv0 +dx2 -dy2);

	gaussian /= 25.0;

	color = lerp(color, gaussian, FilterAmount);

	return color;
}
#endif

/*------------------------------------------------------------------------------
						[GAMMA CORRECTION CODE SECTION]
------------------------------------------------------------------------------*/

#if (GAMMA_CORRECTION == 1)
float4 PostGammaPass(float4 color : COLOR0, float2 uv0 : TEXCOORD0) : SV_Target0
{
	color.rgb = RGBGammaToLinear(color.rgb, GammaConst);
	color.rgb = LinearToRGBGamma(color.rgb, Gamma);
	color.a = RGBLuminance(color.rgb);

	return color;
}
#endif

/*------------------------------------------------------------------------------
                        [TEXTURE SHARPEN CODE SECTION]
------------------------------------------------------------------------------*/

#if (TEXTURE_SHARPENING == 1)
#define px 1.0 / PixelSize.x
#define py 1.0 / PixelSize.y
#define SLumCoeff float3(0.2126729, 0.7151522, 0.0721750)

#if(SharpeningType == 2)
float4 SampleBiCubic(SamplerState texSample, float2 uv0)
{
	Texture.GetDimensions(PixelSize.x, PixelSize.y);
	
	float texelSizeX = 1.0 / PixelSize.x * SharpenBias;
	float texelSizeY = 1.0 / PixelSize.y * SharpenBias;
	
	float4 nSum = (float4)0.0;
	float4 nDenom = (float4)0.0;

    float a = frac(uv0.x * PixelSize.x);
    float b = frac(uv0.y * PixelSize.y);

	int nX = int(uv0.x * PixelSize.x);
	int nY = int(uv0.y * PixelSize.y);
	
	float2 uvCoord = float2(float(nX) / PixelSize.x, float(nY) / PixelSize.y);

    for(int m = -1; m <= 2; m++)
    {
        for(int n = -1; n <= 2; n++)
        {
			float4 Samples = Texture.Sample(texSample, uvCoord +
			float2(texelSizeX * float(m), texelSizeY * float(n)));

			float vc1  = Cubic(float(m) - a);
			float4 vecCoeff1 = float4(vc1, vc1, vc1, vc1);

			float vc2 = Cubic(-(float(n) - b));
			float4 vecCoeff2 = float4(vc2, vc2, vc2, vc2);

            nSum = nSum + (Samples * vecCoeff2 * vecCoeff1);
            nDenom = nDenom + (vecCoeff2 * vecCoeff1);
        }
    }
	return nSum / nDenom;
}

float4 TexSharpenPass(float4 color : COLOR0, float2 uv0 : TEXCOORD0) : SV_Target0 
{
	float3 calcSharpen = (SLumCoeff * SharpenStrength);

	float4 blurredColor = SampleBiCubic(TextureSampler, uv0);
	float3 sharpenedColor = (color.rgb - blurredColor.rgb);

	float sharpenLuma = dot(sharpenedColor, calcSharpen);
	sharpenLuma = clamp(sharpenLuma, -SharpenClamp, SharpenClamp);

	color.rgb = color.rgb + sharpenLuma;
	color.a = RGBLuminance(color.rgb);
	
	#if (DebugSharpen == 1)
		color = saturate(0.5f + (sharpenLuma * 4)).rrrr;
	#endif
	
	return saturate(color);
}
#else

float4 TexSharpenPass(float4 color : COLOR0, float2 uv0 : TEXCOORD0) : SV_Target0 
{	
	float3 blurredColor;
	
	Texture.GetDimensions(PixelSize.x, PixelSize.y);

	blurredColor  = Texture.SampleLevel(TextureSampler, uv0 + float2(-px,py) * SharpenBias, 0.0).rgb;	//North West
	blurredColor += Texture.SampleLevel(TextureSampler, uv0 + float2(px,-py) * SharpenBias, 0.0).rgb;	//South East
	blurredColor += Texture.SampleLevel(TextureSampler, uv0 + float2(-px,-py) * SharpenBias, 0.0).rgb;	//South West
	blurredColor += Texture.SampleLevel(TextureSampler, uv0 + float2(px,py) * SharpenBias, 0.0).rgb;	//North East

	blurredColor += Texture.SampleLevel(TextureSampler, uv0 + float2(0.0,py) * SharpenBias, 0.0).rgb;	//North
	blurredColor += Texture.SampleLevel(TextureSampler, uv0 + float2(0.0,-py) * SharpenBias, 0.0).rgb;	//South
	blurredColor += Texture.SampleLevel(TextureSampler, uv0 + float2(-px,0.0) * SharpenBias, 0.0).rgb;	//West
	blurredColor += Texture.SampleLevel(TextureSampler, uv0 + float2(px,0.0) * SharpenBias, 0.0).rgb;	//East
	
	blurredColor /= 8.0;

	float3 sharpenedColor = color.rgb - blurredColor;
	float3 calcSharpen = (SLumCoeff * SharpenStrength);

	float sharpenLuma = dot(sharpenedColor, calcSharpen);
	sharpenLuma = clamp(sharpenLuma, -SharpenClamp, SharpenClamp);

	color.rgb = color.rgb + sharpenLuma;
	color.a = RGBLuminance(color.rgb);
	
	#if (DebugSharpen == 1)
		color = saturate(0.5f + (sharpenLuma * 4)).rrrr;
	#endif
	
	return saturate(color);
}
#endif
#endif

/*------------------------------------------------------------------------------
							[VIBRANCE CODE SECTION]
------------------------------------------------------------------------------*/

#if (PIXEL_VIBRANCE == 1)
float4 VibrancePass(float4 color : COLOR0, float2 uv0 : TEXCOORD0) : SV_Target0 
{
	float luma = RGBLuminance(color.rgb);
	
	float colorMax = max(color.r, max(color.g, color.b));
	float colorMin = min(color.r, min(color.g, color.b));
	
	float colorSaturation = colorMax - colorMin;

	color.rgb = lerp(luma, color.rgb, (1.0 + (Vibrance * (1.0 - (sign(Vibrance) * colorSaturation)))));
	color.a = RGBLuminance(color.rgb);

	return saturate(color); //Debug: return colorSaturation.xxxx;
}
#endif

/*------------------------------------------------------------------------------
						   [BLOOM PASS CODE SECTION]
------------------------------------------------------------------------------*/

#if (BLENDED_BLOOM == 1)
float3 BlendBloom(float3 color, float3 bloom)
{
	color = (bloom + bloom) - (bloom * bloom);	
	return color;
}

float3 BlendScreen(float3 color, float3 bloom)
{
	return (color + bloom) - (color * bloom);
}

float3 BlendAddLight(float3 color, float3 bloom)
{
	return color + bloom;
}

float3 BlendOverlay(float3 color, float3 bloom)
{
	return float3((bloom.x <= 0.5) ? (2.0 * color.x * bloom.x) 
				: (1.0 - 2.0 * (1.0 - bloom.x) * (1.0 - color.x)),
				(bloom.y <= 0.5) ? (2.0 * color.y * bloom.y) 
				: (1.0 - 2.0 * (1.0 - bloom.y) * (1.0 - color.y)),
				(bloom.z <= 0.5) ? (2.0 * color.z * bloom.z) 
				: (1.0 - 2.0 * (1.0 - bloom.z) * (1.0 - color.z)));
}

float4 BloomPass(float4 color : COLOR0, float2 uv0 : TEXCOORD0) : SV_Target0 
{
	float4 bloom;

	float2 dx = float2(1.0 / PixelSize.x * BlendSpread, 0.0);
	float2 dy = float2(0.0, 1.0 / PixelSize.y * BlendSpread);

	float2 dx2 = 2.0 * dx;
	float2 dy2 = 2.0 * dy;

	float4 bloomBlend = color * 0.22520613262190495;

	bloomBlend += 0.002589001911021066 * Texture.Sample(TextureSampler, uv0 -dx2 +dy2); 
	bloomBlend += 0.010778807494659370 * Texture.Sample(TextureSampler, uv0 -dx +dy2); 
	bloomBlend += 0.024146616900339800 * Texture.Sample(TextureSampler, uv0 +dy2); 
	bloomBlend += 0.010778807494659370 * Texture.Sample(TextureSampler, uv0 +dx +dy2); 
	bloomBlend += 0.002589001911021066 * Texture.Sample(TextureSampler, uv0 +dx2 +dy2); 

	bloomBlend += 0.010778807494659370 * Texture.Sample(TextureSampler, uv0 -dx2 +dy);
	bloomBlend += 0.044875475183061630 * Texture.Sample(TextureSampler, uv0 -dx +dy);
	bloomBlend += 0.100529757860782610 * Texture.Sample(TextureSampler, uv0 +dy);
	bloomBlend += 0.044875475183061630 * Texture.Sample(TextureSampler, uv0 +dx +dy);
	bloomBlend += 0.010778807494659370 * Texture.Sample(TextureSampler, uv0 +dx2 +dy);

	bloomBlend += 0.024146616900339800 * Texture.Sample(TextureSampler, uv0 -dx2);
	bloomBlend += 0.100529757860782610 * Texture.Sample(TextureSampler, uv0 -dx);	
	bloomBlend += 0.100529757860782610 * Texture.Sample(TextureSampler, uv0 +dx);
	bloomBlend += 0.024146616900339800 * Texture.Sample(TextureSampler, uv0 +dx2);

	bloomBlend += 0.010778807494659370 * Texture.Sample(TextureSampler, uv0 -dx2 -dy);
	bloomBlend += 0.044875475183061630 * Texture.Sample(TextureSampler, uv0 -dx -dy);
	bloomBlend += 0.100529757860782610 * Texture.Sample(TextureSampler, uv0 -dy);
	bloomBlend += 0.044875475183061630 * Texture.Sample(TextureSampler, uv0 +dx -dy);
	bloomBlend += 0.010778807494659370 * Texture.Sample(TextureSampler, uv0 +dx2 -dy);

	bloomBlend += 0.002589001911021066 * Texture.Sample(TextureSampler, uv0 -dx2 -dy2);
	bloomBlend += 0.010778807494659370 * Texture.Sample(TextureSampler, uv0 -dx -dy2);
	bloomBlend += 0.024146616900339800 * Texture.Sample(TextureSampler, uv0 -dy2);
	bloomBlend += 0.010778807494659370 * Texture.Sample(TextureSampler, uv0 +dx -dy2);
	bloomBlend += 0.002589001911021066 * Texture.Sample(TextureSampler, uv0 +dx2 -dy2);

	bloomBlend = lerp(color, bloomBlend, BlendPower);
	bloom.rgb = BloomType(color.rgb, bloomBlend.rgb);	

	bloom.r = bloom.r * 1.010778807494659370;
	color.a = RGBLuminance(color.rgb);
	bloom.a = RGBLuminance(bloom.rgb);
	
	#if (BloomMixType == 1)
		color = lerp(color, bloom, BloomPower);
	#elif (BloomMixType == 2)
		color = (lerp(color, bloom, BloomPower) + lerp(bloom, bloomBlend, BloomPower)) / 2.0;
	#elif (BloomMixType == 3)
		color = lerp(color, bloom, lerp(color.a * 0.5, bloom.a, BloomPower));
	#endif

	return saturate(color);
}
#endif

/*------------------------------------------------------------------------------
               [COLOR CORRECTION/TONE MAPPING PASS CODE SECTION]
------------------------------------------------------------------------------*/

#if (SCENE_TONEMAPPING == 1)
float YXYLuminance (float3 YXY)
{
	return (-0.9692660 * YXY.x)
		  + (1.8760108 * YXY.y)
		  + (0.0415560 * YXY.z);
}

float3 FilmicTonemap(float3 x)
{
	float A = 0.10;
	float B = 0.36;
	float C = 0.10;
	float D = 0.30;
	float E = 0.02;
	float F = 0.30;

    return ((x*(A*x+C*B)+D*E) / (x*(A*x+B)+D*F))- E / F;
}

float3 ColorCorrection(float3 color)
{
	float X = 1.0 / (1.0 + exp(RedCurve / 2.0));
	float Y = 1.0 / (1.0 + exp(GreenCurve / 2.0));
	float Z = 1.0 / (1.0 + exp(BlueCurve / 2.0));

	color.r = (1.0 / (1.0 + exp(- RedCurve
	* (color.r - 0.5))) - X) / (1.0 - 2.0 * X);
	color.g = (1.0 / (1.0 + exp(- GreenCurve
	* (color.g - 0.5))) - Y) / (1.0 - 2.0 * Y);
	color.b = (1.0 / (1.0 + exp(- BlueCurve
	* (color.b - 0.5))) - Z) / (1.0 - 2.0 * Z);
	
	return color;
}

float4 TonemapPass(float4 color : COLOR0, float2 uv0 : TEXCOORD0) : SV_Target0 
{	
	float3 lumScale = 1.0/FilmicTonemap(Luminance);

	color.rgb = ColorCorrection(color.rgb);
	color.rgb = FilmicTonemap(Exposure * color.rgb);
    color.rgb = color.rgb * lumScale;

	const float3 lumCoeff = float3(0.2126729, 0.7151522, 0.0721750);
	
	// RGB -> XYZ conversion
	const float3x3 RGB2XYZ = {0.4124564,  0.3575761,  0.1804375,
							  0.2126729,  0.7151522,  0.0721750,
							  0.0193339,  0.1191920,  0.9503041};
							  
	float3 XYZ = mul(RGB2XYZ, color.rgb);
  
	// XYZ -> Yxy conversion
	float3 Yxy = lumCoeff;

	Yxy.r = XYZ.g;                            	// copy luminance Y
	Yxy.g = XYZ.r / (XYZ.r + XYZ.g + XYZ.b ); 	// x = X / (X + Y + Z)
	Yxy.b = XYZ.g / (XYZ.r + XYZ.g + XYZ.b ); 	// y = Y / (X + Y + Z)
	
	// (Lp) Map average luminance to the middlegrey zone by scaling pixel luminance
	#if (TonemapType == 1)
	float Lp = Yxy.r * Exposure / Luminance;

	#elif (TonemapType == 2)
	float Lp = ((Yxy.r * (YXYLuminance(Yxy.rrr) / 1.5)) +
				(Yxy.g * (YXYLuminance(Yxy.rrr) / 1.5)) +
				(Yxy.b * (YXYLuminance(Yxy.rrr) / 1.5)))*(Exposure / Luminance);
	#endif
	
	// (Ld) Scale all luminance within a displayable range of 0 to 1
	Yxy.r = (Lp * (1.0 + Lp/(WhitePoint * WhitePoint)))/(1.0 + Lp);
  
	// Yxy -> XYZ conversion
	XYZ.r = Yxy.r * Yxy.g / Yxy. b;               	// X = Y * x / y
	XYZ.g = Yxy.r;                                	// copy luminance Y
	XYZ.b = Yxy.r * (1.0 - Yxy.g - Yxy.b) / Yxy.b;	// Z = Y * (1-x-y) / y
	
	// XYZ -> RGB conversion
	const float3x3 XYZ2RGB = {3.2404542, -1.5371385, -0.4985314,
							 -0.9692660,  1.8760108,  0.0415560, 
							  0.0556434, -0.2040259,  1.0572252};

	color.rgb = mul(XYZ2RGB, XYZ);

	color.rgb = RGBGammaToLinear(color.rgb, GammaConst);
	color.rgb = LinearToRGBGamma(color.rgb, ToneAmount);
	color.a = RGBLuminance(color.rgb);

	return saturate(color);
}
#endif

/*------------------------------------------------------------------------------
						[S-CURVE CONTRAST CODE SECTION]
------------------------------------------------------------------------------*/

#if (S_CURVE_CONTRAST == 1)
float4 SCurvePass(float4 color : COLOR0, float2 uv0 : TEXCOORD0) : SV_Target0
{
	float CurveBlend = CurvesContrast;

	#if (CurveType != 2)
		float3 luma = (float3)RGBLuminance(color.rgb);
		float3 chroma = color.rgb - luma;
	#endif

	#if (CurveType == 2)
		float3 x = color.rgb;
	#elif (CurveType == 1)
		float3 x = chroma;
		x = x * 0.5 + 0.5;
	#else
		float3 x = luma;
	#endif

	//S-Curve - Cubic Bezier spline
	float3 a = float3(0.00, 0.00, 0.00);	//start point
	float3 b = float3(0.25, 0.25, 0.25);	//control point 1
	float3 c = float3(0.80, 0.80, 0.80);	//control point 2
	float3 d = float3(1.00, 1.00, 1.00);	//endpoint

	float3 ab = lerp(a, b, x);				//point between a and b (green)
	float3 bc = lerp(b, c, x);				//point between b and c (green)
	float3 cd = lerp(c, d, x);				//point between c and d (green)
	float3 abbc = lerp(ab, bc, x);			//point between ab and bc (blue)
	float3 bccd = lerp(bc, cd, x);			//point between bc and cd (blue)
	float3 dest = lerp(abbc, bccd, x);		//point on the bezier-curve (black)

	x = dest;

	#if (CurveType == 0) //Only Luma
		x = lerp(luma, x, CurveBlend);
		color.rgb = x + chroma;
	#elif (CurveType == 1) //Only Chroma
		x = x * 2 - 1;
		float3 LColor = luma + x;
		color.rgb = lerp(color.rgb, LColor, CurveBlend);
	#elif (CurveType == 2) //Both Luma and Chroma
		float3 LColor = x; 
		color.rgb = lerp(color.rgb, LColor, CurveBlend);
	#endif
	
	color.a = RGBLuminance(color.rgb);
	
	return saturate(color);
}
#endif

/*------------------------------------------------------------------------------
							[CEL SHADING CODE SECTION]
------------------------------------------------------------------------------*/

#if (CEL_SHADING == 1)
#define RoundingOffset float2(0.20, 0.40)

static const int NUM = 9;
static const float3 thresholds = float3(5.0, 8.0, 6.0);

#if (LumaConversion == 1)
#define celLumaCoef float3(0.2126729, 0.7151522, 0.0721750)
#else
#define celLumaCoef float3(0.299, 0.587, 0.114)
#endif

float3 GetYUV(float3 rgb)
{  
	#if (LumaConversion == 1)
	float3x3 RGB2YUV = {0.2126, 0.7152, 0.0722,
					   -0.09991, -0.33609, 0.436,
						0.615, -0.55861, -0.05639};

	#else
	float3x3 RGB2YUV = {0.299, 0.587, 0.114,
					   -0.14713, -0.28886f, 0.436,
						0.615, -0.51499, -0.10001};
	#endif
	
	return mul(RGB2YUV, rgb);
}

float3 GetRGB(float3 yuv)
{
    #if (LumaConversion == 1)
	float3x3 YUV2RGB = {1.000, 0.000, 1.28033,
						1.000, -0.21482, -0.38059,
						1.000, 2.12798, 0.000};

	#else
	float3x3 YUV2RGB = {1.000, 0.000, 1.13983,
						1.000, -0.39465, -0.58060,
						1.000, 2.03211, 0.000};
	#endif
	
	return mul(YUV2RGB, yuv);
}

float GetCelLuminance(float3 rgb)
{
    return dot(rgb, celLumaCoef);
}

float4 CelPass(float4 color : COLOR0, float2 uv0 : TEXCOORD0) : SV_Target0
{
	float3 yuv;
	float3 sum = color.rgb;

	float2 pixel =  float2(1.0/2560.0, 1.0/1440.0) * EdgeThickness;

	float2 c[NUM] = {
	float2(-0.0078125,-0.0078125),
	float2( 0.00 ,    -0.0078125),
	float2( 0.0078125,-0.0078125),
	float2(-0.0078125, 0.00),
	float2( 0.00,      0.00),
	float2( 0.0078125, 0.00),
	float2(-0.0078125, 0.0078125),
	float2( 0.00 ,     0.0078125),
	float2( 0.0078125, 0.0078125)};
	
	float3 col[NUM];
	float lum[NUM];

	for (int i = 0; i < NUM; i++)
	{
		col[i] = Texture.Sample(TextureSampler, uv0 + c[i] * RoundingOffset).rgb;
		
		#if (ColorRounding == 1)
			col[i].r = saturate(round(col[i].r * thresholds.r) / thresholds.r);
			col[i].g = saturate(round(col[i].g * thresholds.g) / thresholds.g);
			col[i].b = saturate(round(col[i].b * thresholds.b) / thresholds.b);
		#endif
		
		lum[i] = GetCelLuminance(col[i].xyz);

		yuv = GetYUV(col[i]);
		
		if (UseYuvLuma == 0)
			{ yuv.r = saturate(round(yuv.r * lum[i]) / thresholds.r + lum[i]); }
		else
			{ yuv.r = saturate(round(yuv.r * thresholds.r) / thresholds.r + lum[i] / (255.0 / 5.0)); }
		
		yuv = GetRGB(yuv);
 
		sum += yuv;
	}
	
	float3 shadedColor = (sum / NUM);

	float edgeX = dot(Texture.Sample(TextureSampler, uv0 + pixel).rgb, celLumaCoef);
	edgeX = dot(float4(Texture.Sample(TextureSampler, uv0 - pixel).rgb, edgeX), float4(celLumaCoef, -1.0));

	float edgeY = dot(Texture.Sample(TextureSampler, uv0 + float2(pixel.x, -pixel.y)).rgb, celLumaCoef);
	edgeY = dot(float4(Texture.Sample(TextureSampler, uv0 + float2(-pixel.x, pixel.y)).rgb, edgeY), float4(celLumaCoef, -1.0));

	float edge = dot(float2(edgeX, edgeY), float2(edgeX, edgeY));

	#if (PaletteType == 1)
		color.rgb = lerp(color.rgb, color.rgb + pow(edge, EdgeFilter) * -EdgeStrength, EdgeStrength);
	#elif (PaletteType == 2)
		color.rgb = lerp(color.rgb + pow(edge, EdgeFilter) * -EdgeStrength, shadedColor, 0.33);
	#elif (PaletteType == 3)
		color.rgb = lerp(shadedColor + edge * -EdgeStrength, pow(edge, EdgeFilter) * -EdgeStrength + color.rgb, 0.5);
	#endif
	
	color.a = RGBLuminance(color.rgb);

	return saturate(color);
}
#endif

/*------------------------------------------------------------------------------
						[COLOR GRADING CODE SECTION]
------------------------------------------------------------------------------*/

#if (COLOR_GRADING == 1)
float RGBCVtoHUE(float3 RGB, float C, float V)
{
    float3 Delta = (V - RGB) / C;

    Delta.rgb -= Delta.brg;
    Delta.rgb += float3(2,4,6);
    Delta.brg = step(V, RGB) * Delta.brg;

    float H;
    H = max(Delta.r, max(Delta.g, Delta.b));
    return frac(H / 6);
}
 
float3 RGBtoHSV(float3 RGB)
{
    float3 HSV = 0;
    HSV.z = max(RGB.r, max(RGB.g, RGB.b));
    float M = min(RGB.r, min(RGB.g, RGB.b));
    float C = HSV.z - M;

    if (C != 0)
    {
        HSV.x = RGBCVtoHUE(RGB, C, HSV.z);
        HSV.y = C / HSV.z;
    }

    return HSV;
}
 
float3 HUEtoRGB(float H)
{
    float R = abs(H * 6 - 3) - 1;
    float G = 2 - abs(H * 6 - 2);
    float B = 2 - abs(H * 6 - 4);

    return saturate(float3(R,G,B));
}
 
float3 HSVtoRGB(float3 HSV)
{
    float3 RGB = HUEtoRGB(HSV.x);
    return ((RGB - 1) * HSV.y + 1) * HSV.z;
}
 
float3 HSVComplement(float3 HSV)
{
	float3 complement = HSV;
	complement.x -= 0.5;

	if (complement.x < 0.0) { complement.x += 1.0; }
	return(complement);
}

float HueLerp(float h1, float h2, float v)
{
    float d = abs(h1 - h2);

    if(d <= 0.5)
    {
        return lerp(h1, h2, v);
    }
    else if(h1 < h2)
    {
        return frac(lerp((h1 + 1.0), h2, v));
    }
    else
    {
        return frac(lerp(h1, (h2 + 1.0), v));
    }
}

float4 ColorGrading(float4 color : COLOR0, float2 uv0 : TEXCOORD0) : SV_Target0
{
	float3 guide = float3(RedGrading, GreenGrading, BlueGrading);
    float amount = GradingStrength;
    float correlation = Correlation;
    float concentration = 2.00;

    float3 colorHSV = RGBtoHSV(color.rgb);
    float3 huePoleA = RGBtoHSV(guide);
    float3 huePoleB = HSVComplement(huePoleA);
 
    float dist1 = abs(colorHSV.x - huePoleA.x); if (dist1 > 0.5) dist1 = 1.0 - dist1;
    float dist2 = abs(colorHSV.x - huePoleB.x); if (dist2 > 0.5) dist2 = 1.0 - dist2;
 
    float descent = smoothstep(0.0, correlation, colorHSV.y);
 
    float3 HSVColor = colorHSV;

    if(dist1 < dist2)
    {
        float c = descent * amount * (1.0 - pow((dist1 * 2.0), 1.0 / concentration));
        HSVColor.x = HueLerp(colorHSV.x, huePoleA.x, c);
        HSVColor.y = lerp(colorHSV.y, huePoleA.y, c);
    }
    else
    {
        float c = descent * amount * (1.0 - pow((dist2 * 2.0), 1.0 / concentration));
        HSVColor.x = HueLerp(colorHSV.x, huePoleB.x, c);
        HSVColor.y = lerp(colorHSV.y, huePoleB.y, c);
    }
 
    color.rgb = HSVtoRGB(HSVColor);
	color.a = RGBLuminance(color.rgb);
	
	return saturate(color);
}
#endif

/*------------------------------------------------------------------------------
                           [SCANLINES CODE SECTION]
------------------------------------------------------------------------------*/

#if (SCANLINES == 1)
float4 ScanlinesPass(float4 color : COLOR0, float2 uv0 : TEXCOORD0, float4 FragCoord : SV_Position) : SV_Target0
{
	
	#if (ScanlineType == 3)
		float amount = ScanlineBrightness;
		float intensity = ScanlineIntensity;
		
		float pos0 = ((uv0.y + 1.0) * 170.0 * amount);
		float pos1 = cos((frac(pos0 * ScanlineScale) - 0.5) * 3.1415926 * intensity) * 1.2;

		color = lerp(float4(0,0,0,0), color, pos1);
	#else
	
	float4 intensity;
	
	#if (ScanlineType == 0)
		if (frac(FragCoord.y * 0.25) > ScanlineScale)
	#elif (ScanlineType == 1)
		if (frac(FragCoord.x * 0.25) > ScanlineScale)
	#elif (ScanlineType == 2)
		if (frac(FragCoord.x * 0.25) > ScanlineScale && frac(FragCoord.y * 0.25) > ScanlineScale)
	#endif
		{
			intensity = float4(0.0, 0.0, 0.0, 0.0);
		}
		else
		{
			intensity = smoothstep(0.2, ScanlineBrightness, color) + normalize(float4(color.xyz, RGBLuminance(color.xyz)));
		}
	
	float level = (4.0-uv0.x) * ScanlineIntensity;
	
	color = intensity * (0.5-level) + color * 1.1;
	#endif

	return color;
}
#endif

/*------------------------------------------------------------------------------
							[VIGNETTE CODE SECTION]
------------------------------------------------------------------------------*/

#if (VIGNETTE == 1)
#define VignetteCenter float2(0.500, 0.500)

float4 VignettePass(float4 color : COLOR0, float2 uv0 : TEXCOORD0) : SV_Target
{
	float2 tc = uv0 - VignetteCenter;

	tc *= float2((PixelSize.x / PixelSize.y), VignetteRatio);
	tc /= VignetteRadius;

	float v = dot(tc, tc);

	color.rgb *= (1.0 + pow(v, VignetteSlope * 0.5) * -VignetteAmount);

	return color;
}
#endif

/*------------------------------------------------------------------------------
						[SUBPIXEL DITHERING CODE SECTION]
------------------------------------------------------------------------------*/

#if (DITHERING == 1)
float4 DitherPass(float4 color : COLOR0, float2 uv0 : TEXCOORD0) : SV_Target
{  
   float ditherSize = 2.0;
   float ditherBits  = 8.0;

   #if DitherMethod == 2 //random subpixel dithering

	float seed = dot(uv0, float2(12.9898,78.233));
	float sine = sin(seed);
	float noise = frac(sine * 43758.5453 + uv0.x);

	float ditherShift = (1.0 / (pow(2.0, ditherBits) - 1.0));
	float ditherHalfShift = (ditherShift * 0.5);
	ditherShift = ditherShift * noise - ditherHalfShift;

	color.rgb += float3(-ditherShift, ditherShift, -ditherShift);
     
   #else //Ordered dithering

     float gridPosition = frac(dot(uv0,(PixelSize.xy / ditherSize)) + (0.5 / ditherSize));
     float ditherShift = (0.75) * (1.0 / (pow(2,ditherBits) - 1.0));

     float3 RGBShift = float3(ditherShift, -ditherShift, ditherShift);
     RGBShift = lerp(2.0 * RGBShift, -2.0 * RGBShift, gridPosition);

     color.rgb += RGBShift;
   #endif

   color.a = RGBLuminance(color.rgb);
   
   return color;
}
#endif

/*------------------------------------------------------------------------------
                      [MAIN() & COMBINE PASS CODE SECTION]
------------------------------------------------------------------------------*/

PS_OUTPUT ps_main(VS_OUTPUT input)
{
	PS_OUTPUT output;

	float4 color = Texture.Sample(TextureSampler, input.t);
	color = PreGammaPass(color, input.t);

	#if (BILINEAR_FILTERING == 1)
		color = BiLinearPass(color, input.t);
	#endif
	
	#if (GAUSSIAN_FILTERING == 1)
		color = GaussianPass(color, input.t);
	#endif
	
	#if (BICUBIC_FILTERING == 1)
		color = BiCubicPass(color, input.t);
	#endif
	
	#if (UHQ_FXAA == 1)
		color = FxaaPass(color, input.t);
	#endif
	
	#if (TEXTURE_SHARPENING == 1)
		color = TexSharpenPass(color, input.t);
	#endif
	
	#if (CEL_SHADING == 1)
		color = CelPass(color, input.t);
	#endif

	#if (SCANLINES == 1)
		color = ScanlinesPass(color, input.t, input.p);
	#endif

	#if (BLENDED_BLOOM == 1)
		color = BloomPass(color, input.t);
	#endif
	
	#if (SCENE_TONEMAPPING == 1)
		color = TonemapPass(color, input.t);
	#endif
	
	#if (PIXEL_VIBRANCE == 1)
		color = VibrancePass(color, input.t);
	#endif
	
	#if (COLOR_GRADING == 1)
		color = ColorGrading(color, input.t);
	#endif
	
	#if (S_CURVE_CONTRAST == 1)
		color = SCurvePass(color, input.t);
	#endif
	
	#if (GAMMA_CORRECTION == 1)
		color = PostGammaPass(color, input.t);
	#endif
	
	#if (VIGNETTE == 1)
		color = VignettePass(color, input.t);
	#endif
	
	#if (DITHERING == 1)
		color = DitherPass(color, input.t);
	#endif

	output.c = color;
	
	return output;
}