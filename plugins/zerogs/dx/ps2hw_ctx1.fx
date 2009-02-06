texture g_txMem1;

sampler g_sMemory : register(s1) = sampler_state {
    Texture = <g_txMem1>;
    MipFilter = POINT;
    MinFilter = POINT;
    MagFilter = POINT;
    AddressU = Clamp;
    AddressV = Clamp;
};

// per context pixel shader constants
half4 fTexAlpha2 : register(c3);

float4 g_fTexOffset : register(c5);			// converts the page and block offsets into the mem addr/1024
float4 g_fTexDims : register(c7);				// mult by tex dims when accessing the block texture
float4 g_fTexBlock : register(c9);

float4 g_fClampExts : register(c11);		// if clamping the texture, use (minu, minv, maxu, maxv)
float4 TexWrapMode : register(c13);			// 0 - repeat/clamp, 1 - region rep (use fRegRepMask)

float4 g_fRealTexDims : register(c15); // tex dims used for linear filtering (w,h,1/w,1/h)

// (alpha0, alpha1, 1 if highlight2 and tcc is rgba, 1-y)
half4 g_fTestBlack : register(c17);	// used for aem bit

float4 g_fPageOffset : register(c19);

half4 fTexAlpha : register(c21);

// vertex shader constants
float4 g_fPosXY : register(c3);