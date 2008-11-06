#define NUM_FILTERS 2 // texture filtering
#define NUM_TYPES 5 // types of texture read modes
#define NUM_TEXWRAPS 4 // texture wrapping

#define NUM_SHADERS (NUM_FILTERS*NUM_TYPES*NUM_TEXWRAPS*32) // # shaders for a given ps

#define SHADER_20 0
#define SHADER_20a 1
#define SHADER_20b 2
#define SHADER_30 3

// ps == 0, 2.0
// ps == 1, 2.0a, nvidia
// ps == 2, 2.0b, ati
const static char* g_pShaders[4] = { "ps_2_0", "ps_2_a", "ps_2_b", "ps_3_0" };
const static char* g_pPsTexWrap[] = { "REPEAT", "CLAMP", "REGION_REPEAT", NULL };
const static char* g_pTexTypes[] = { "32", "tex32", "clut32", "tex32to16", "tex16to8h" };

#define TEXWRAP_REPEAT 0
#define TEXWRAP_CLAMP 1
#define TEXWRAP_REGION_REPEAT 2
#define TEXWRAP_REPEAT_CLAMP 3

inline int GET_SHADER_INDEX(int type, int texfilter, int texwrap, int fog, int writedepth, int testaem, int exactcolor, int context, int ps)
{
	return type + texfilter*NUM_TYPES + NUM_FILTERS*NUM_TYPES*texwrap + NUM_TEXWRAPS*NUM_FILTERS*NUM_TYPES*(fog+2*writedepth+4*testaem+8*exactcolor+16*context+32*ps);
}

static HRESULT LoadShaderFromType(const char* srcfile, int type, int texfilter, int texwrap, int fog, int writedepth, int testaem, int exactcolor, int ps, DWORD flags,
						   IDirect3DDevice9* pd3dDevice, ID3DXInclude* pInclude, IDirect3DPixelShader9** pps)
{
	assert( texwrap < NUM_TEXWRAPS);
	assert( type < NUM_TYPES );

	char str[255];
	sprintf(str, "Texture%s%d_%sPS", fog?"Fog":"", texfilter, g_pTexTypes[type]);

	LPD3DXBUFFER pShader = NULL, pError = NULL;
	SAFE_RELEASE(*pps);

	vector<D3DXMACRO> macros;
	D3DXMACRO dummy; dummy.Definition = NULL; dummy.Name = NULL;
	if( g_pPsTexWrap[texwrap] != NULL ) {
		D3DXMACRO m;
		m.Name = g_pPsTexWrap[texwrap];
		m.Definition = "1";
		macros.push_back(m);
	}
	if( writedepth ) {
		D3DXMACRO m;
		m.Name = "WRITE_DEPTH";
		m.Definition = "1";
		macros.push_back(m);
	}
	if( testaem ) {
		D3DXMACRO m;
		m.Name = "TEST_AEM";
		m.Definition = "1";
		macros.push_back(m);
	}
	if( exactcolor ) {
		D3DXMACRO m;
		m.Name = "EXACT_COLOR";
		m.Definition = "1";
		macros.push_back(m);
	}

	macros.push_back(dummy);
	HRESULT hr = D3DXCompileShaderFromFile(srcfile, &macros[0], pInclude, str, g_pShaders[ps], flags, &pShader, &pError, NULL);

	if( FAILED(hr) )
	{
		printf("Failed to load %s\n%s\n", str, pError->GetBufferPointer());
		SAFE_RELEASE(pShader);
		SAFE_RELEASE(pError);
		return hr;
	}

	DWORD* ptr = (DWORD*)pShader->GetBufferPointer();
	hr = pd3dDevice->CreatePixelShader(ptr, pps);
	SAFE_RELEASE(pShader);
	SAFE_RELEASE(pError);

	return hr;
}

struct SHADERHEADER
{
	DWORD index, offset, size; // if highest bit of index is set, pixel shader
};

#define SH_30 0x4000 // or for vs3.0 shaders
#define SH_WRITEDEPTH 0x2000 // depth is written
#define SH_CONTEXT1 0x1000 // context1 is used

#define SH_REGULARVS 0x8000
#define SH_TEXTUREVS 0x8001
#define SH_REGULARFOGVS 0x8002
#define SH_TEXTUREFOGVS 0x8003
#define SH_REGULARPS 0x8004
#define SH_REGULARFOGPS 0x8005
#define SH_BITBLTVS 0x8006
#define SH_BITBLTPS 0x8007
#define SH_BITBLTDEPTHPS 0x8009
#define SH_CRTCTARGPS 0x800a
#define SH_CRTCPS 0x800b
#define SH_CRTC24PS 0x800c
#define SH_ZEROPS 0x800e
#define SH_BASETEXTUREPS 0x800f
#define SH_BITBLTAAPS 0x8010
#define SH_CRTCTARGINTERPS 0x8012
#define SH_CRTCINTERPS 0x8013
#define SH_CRTC24INTERPS 0x8014
#define SH_BITBLTDEPTHMRTPS 0x8016
#define SH_BITBLTDEPTHTEXPS 0x8017
#define SH_BITBLTDEPTHTEXMRTPS 0x8018
#define SH_CONVERT16TO32PS 0x8020
#define SH_CONVERT32TO16PS 0x8021
