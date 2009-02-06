// Builds all possible shader files from ps2hw.fx and stores them in
// a preprocessed database
#include <stdio.h>

#include <windows.h>
#include <d3dx9.h>
#include <assert.h>

#define SAFE_RELEASE(x) { if( (x) != NULL ) { (x)->Release(); x = NULL; } }

#include <map>
#include <vector>

using namespace std;

#include "zerogsshaders.h"

char* srcfilename = "ps2hw.fx";
char* dstfilename = "ps2hw.dat";
DWORD dwFlags = 0;

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

struct SHADERINFO
{
	DWORD type; // 1 - ps, 0 - vs
	LPD3DXBUFFER pbuf;
};

map<DWORD, SHADERINFO> mapShaders;

class ZeroGSShaderInclude : public ID3DXInclude
{
public:
	int context;
	int ps2x; // if 0, ps20 only
	char* pEffectDir;

	STDMETHOD(Open)(D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes)
	{
		const char* pfilename = pFileName;
		char strfile[255];
		if( strstr(pFileName, "ps2hw_ctx") != NULL ) {

			_snprintf(strfile, 255, "%sps2hw_ctx%d.fx", pEffectDir, context);
			pfilename = strfile;
		}
//		else if( strstr(pFileName, "ps2hw_ps2x") != NULL ) {
//			_snprintf(strfile, 255, "%sps2hw_ps2%c.fx", pEffectDir, ps2x?'x':'0');
//			pfilename = strfile;
//		}
		else if( strstr(pFileName, "ps2hw.fx") != NULL ) {
			_snprintf(strfile, 255, "%s%s", pEffectDir, pFileName);
			pfilename = strfile;
		}
		
		FILE* f = fopen(pfilename, "rb");

		if( f == NULL )
			return E_FAIL;

		fseek(f, 0, SEEK_END);
		DWORD size = ftell(f);
		fseek(f, 0, SEEK_SET);
		char* buffer = new char[size+1];
		fread(buffer, size, 1, f);
		buffer[size] = 0;

		*ppData = buffer;
		*pBytes = size;
		fclose(f);

		return S_OK;
	}

	STDMETHOD(Close)(LPCVOID pData)
	{
		delete[] (char*)pData;
		return S_OK;
	}
};

void LoadShader(int index, const char* name, const char* pshader, D3DXMACRO* pmacros, ID3DXInclude* pInclude)
{	
	LPD3DXBUFFER pShader = NULL, pError = NULL;

	HRESULT hr = D3DXCompileShaderFromFile(srcfilename, pmacros, pInclude, name, pshader, dwFlags, &pShader, &pError, NULL);

	if( FAILED(hr) )
	{
		printf("Failed to load %s\n%s\n", name, pError->GetBufferPointer());
		SAFE_RELEASE(pShader);
		SAFE_RELEASE(pError);
		return;
	}

	SAFE_RELEASE(pError);

	if( mapShaders.find(index) != mapShaders.end() ) {
		printf("two shaders share the same index %d\n", index);
		exit(0);
	}

	SHADERINFO info;
	info.type = name[0] == 'p' ? 0x80000000 : 0;
	info.pbuf = pShader;
	mapShaders[index] = info;
}

int main(int argc, char** argv)
{
	printf("usage: [src] [dst] [opts]\n");

	if( argc >= 2 ) srcfilename = argv[1];
	if( argc >= 3 ) dstfilename = argv[2];
	if( argc >= 4 ) {
		dwFlags = atoi(argv[3]);
	}

	FILE* fsrc = fopen(srcfilename, "r");
	if( fsrc == NULL ) {
		printf("cannot open %s\n", srcfilename);
		return 0;
	}
	fclose(fsrc);

	LoadShader(SH_BITBLTVS, "BitBltVS", "vs_2_0", NULL, NULL);
	LoadShader(SH_BITBLTVS|SH_30, "BitBltVS", "vs_3_0", NULL, NULL);
	LoadShader(SH_BITBLTPS, "BitBltPS", "ps_2_0", NULL, NULL);
	LoadShader(SH_BITBLTDEPTHPS, "BitBltDepthPS", "ps_2_0", NULL, NULL);
	LoadShader(SH_BITBLTDEPTHMRTPS, "BitBltDepthMRTPS", "ps_2_0", NULL, NULL);
    LoadShader(SH_BITBLTDEPTHTEXPS, "BitBltDepthTexPS", "ps_2_0", NULL, NULL);
	LoadShader(SH_BITBLTDEPTHTEXMRTPS, "BitBltDepthTexMRTPS", "ps_2_0", NULL, NULL);
	LoadShader(SH_CRTCTARGPS, "CRTCTargPS", "ps_2_0", NULL, NULL);
	LoadShader(SH_CRTCPS, "CRTCPS", "ps_2_0", NULL, NULL);
	LoadShader(SH_CRTC24PS, "CRTC24PS", "ps_2_0", NULL, NULL);
	LoadShader(SH_ZEROPS, "ZeroPS", "ps_2_0", NULL, NULL);
	LoadShader(SH_ZEROPS|SH_30, "ZeroPS", "ps_3_0", NULL, NULL);
	LoadShader(SH_BASETEXTUREPS, "BaseTexturePS", "ps_2_0", NULL, NULL);
	LoadShader(SH_BITBLTAAPS, "BitBltPS", "ps_2_0", NULL, NULL);
	LoadShader(SH_CRTCTARGINTERPS, "CRTCTargInterPS", "ps_2_0", NULL, NULL);
	LoadShader(SH_CRTCINTERPS, "CRTCInterPS", "ps_2_0", NULL, NULL);
	LoadShader(SH_CRTC24INTERPS, "CRTC24InterPS", "ps_2_0", NULL, NULL);
    LoadShader(SH_CONVERT16TO32PS, "Convert16to32PS", "ps_2_0", NULL, NULL);
    LoadShader(SH_CONVERT32TO16PS, "Convert32to16PS", "ps_2_0", NULL, NULL);

	const int vsshaders[4] = { SH_REGULARVS, SH_TEXTUREVS, SH_REGULARFOGVS, SH_TEXTUREFOGVS };
	const char* pvsshaders[4] = { "RegularVS", "TextureVS", "RegularFogVS", "TextureFogVS" };

	// load the texture shaders
	char str[255], strdir[255];

	strcpy(strdir, srcfilename);
	int i = (int)strlen(strdir);
	while(i > 0) {
		if( strdir[i-1] == '/' || strdir[i-1] == '\\' )
			break;
		--i;
	}
	
	strdir[i] = 0;

	ZeroGSShaderInclude inc;
	inc.pEffectDir = strdir;

	D3DXMACRO macros[10];
	memset(macros, 0, sizeof(macros));

	macros[0].Name = "WRITE_DEPTH";
	macros[0].Definition = "1";	

	for(i = 0; i < ARRAYSIZE(vsshaders); ++i) {
		for(int vs30 = 0; vs30 < 2; ++vs30 ) {
			for(int writedepth = 0; writedepth < 2; ++writedepth ) {
				inc.context = 0;
				LoadShader(vsshaders[i]|(vs30?SH_30:0)|(writedepth?SH_WRITEDEPTH:0), pvsshaders[i], vs30 ? "vs_3_0" : "vs_2_0", writedepth ? macros : NULL, &inc);
				inc.context = 1;
				LoadShader(vsshaders[i]|(vs30?SH_30:0)|(writedepth?SH_WRITEDEPTH:0)|SH_CONTEXT1, pvsshaders[i], vs30 ? "vs_3_0" : "vs_2_0", writedepth ? macros : NULL, &inc);
			}
		}
	}

	const int psshaders[2] = { SH_REGULARPS, SH_REGULARFOGPS };
	const char* ppsshaders[2] = { "RegularPS", "RegularFogPS" };

	for(i = 0; i < ARRAYSIZE(psshaders); ++i) {
		for(int ps30 = 0; ps30 < 2; ++ps30 ) {
			for(int writedepth = 0; writedepth < 2; ++writedepth ) {
				LoadShader(psshaders[i]|(ps30?SH_30:0)|(writedepth?SH_WRITEDEPTH:0), ppsshaders[i], ps30 ? "ps_3_0" : "ps_2_0", writedepth ? macros : NULL, NULL);
			}
		}
	}

	printf("creating shaders, note that ps2hw_ctx0.fx, and ps2hw_ctx1.fx are required\n");
		
	for(int texwrap = 0; texwrap < NUM_TEXWRAPS; ++texwrap ) {

		int macroindex = 0;
		memset(macros, 0, sizeof(macros));

		if( g_pPsTexWrap[texwrap] != NULL ) {
			macros[0].Name = g_pPsTexWrap[texwrap];
			macros[0].Definition = "1";
			macroindex++;
		}

		for(int context = 0; context < 2; ++context) {
			inc.context = context;

			for(int texfilter = 0; texfilter < NUM_FILTERS; ++texfilter) {
				for(int fog = 0; fog < 2; ++fog ) {
					for(int writedepth = 0; writedepth < 2; ++writedepth ) {

						if( writedepth ) {
							macros[macroindex].Name = "WRITE_DEPTH";
							macros[macroindex].Definition = "1";
							macroindex++;
						}
						else {
							macros[macroindex].Name = NULL;
							macros[macroindex].Definition = NULL;
						}

						for(int testaem = 0; testaem < 2; ++testaem ) {

							if( testaem ) {
								macros[macroindex].Name = "TEST_AEM";
								macros[macroindex].Definition = "1";
								macroindex++;
							}
							else {
								macros[macroindex].Name = NULL;
								macros[macroindex].Definition = NULL;
							}

							for(int exactcolor = 0; exactcolor < 2; ++exactcolor ) {
								
								if( exactcolor ) {
									macros[macroindex].Name = "EXACT_COLOR";
									macros[macroindex].Definition = "1";
									macroindex++;
								}
								else {
									macros[macroindex].Name = NULL;
									macros[macroindex].Definition = NULL;
								}

								// 32
								sprintf(str, "Texture%s%d_32PS", fog?"Fog":"", texfilter);
								inc.ps2x = 0;

								if( macros[macroindex].Name != NULL )
									printf("error[%d] %s: %d %d %d %d!\n", macroindex, macros[macroindex].Name, texfilter, fog, writedepth, testaem);

								macros[macroindex].Name = "ACCURATE_DECOMPRESSION";
								macros[macroindex].Definition = "1";

								if( texfilter == 0 && exactcolor == 0 )  {
									LoadShader(GET_SHADER_INDEX(0, texfilter, texwrap, fog, writedepth, testaem, exactcolor, context, SHADER_20), str, g_pShaders[SHADER_20], macros, &inc);
								}

								inc.ps2x = 1;
								LoadShader(GET_SHADER_INDEX(0, texfilter, texwrap, fog, writedepth, testaem, exactcolor, context, SHADER_20b), str, g_pShaders[SHADER_20b], macros, &inc);

								macros[macroindex].Name = NULL;
								macros[macroindex].Definition = NULL;

								LoadShader(GET_SHADER_INDEX(0, texfilter, texwrap, fog, writedepth, testaem, exactcolor, context, SHADER_20a), str, g_pShaders[SHADER_20a], macros, &inc);
								LoadShader(GET_SHADER_INDEX(0, texfilter, texwrap, fog, writedepth, testaem, exactcolor, context, SHADER_30), str, g_pShaders[SHADER_30], macros, &inc);

                                if( texfilter == 0 ) {
    								// tex32
    								sprintf(str, "Texture%s%d_tex32PS", fog?"Fog":"", texfilter);
    								inc.ps2x = 0;
  
    								macros[macroindex].Name = "ACCURATE_DECOMPRESSION";
    								macros[macroindex].Definition = "1";
  
    								if( texfilter == 0 && exactcolor == 0 ) {
    									LoadShader(GET_SHADER_INDEX(1, texfilter, texwrap, fog, writedepth, testaem, exactcolor, context, SHADER_20), str, g_pShaders[SHADER_20], macros, &inc);
    								}
  
    								inc.ps2x = 1;
  
    								LoadShader(GET_SHADER_INDEX(1, texfilter, texwrap, fog, writedepth, testaem, exactcolor, context, SHADER_20b), str, g_pShaders[SHADER_20b], macros, &inc);
  
    								macros[macroindex].Name = NULL;
    								macros[macroindex].Definition = NULL;
  
    								LoadShader(GET_SHADER_INDEX(1, texfilter, texwrap, fog, writedepth, testaem, exactcolor, context, SHADER_20a), str, g_pShaders[SHADER_20a], macros, &inc);
    								LoadShader(GET_SHADER_INDEX(1, texfilter, texwrap, fog, writedepth, testaem, exactcolor, context, SHADER_30), str, g_pShaders[SHADER_30], macros, &inc);
  
    								// clut32
    								sprintf(str, "Texture%s%d_clut32PS", fog?"Fog":"", texfilter);
    								inc.ps2x = 0;
  
    								macros[macroindex].Name = "ACCURATE_DECOMPRESSION";
    								macros[macroindex].Definition = "1";
  
    								if( texfilter == 0 && exactcolor == 0 ) {
    									LoadShader(GET_SHADER_INDEX(2, texfilter, texwrap, fog, writedepth, testaem, exactcolor, context, SHADER_20), str, g_pShaders[SHADER_20], macros, &inc);
    								}
  
    								inc.ps2x = 1;
    
    								LoadShader(GET_SHADER_INDEX(2, texfilter, texwrap, fog, writedepth, testaem, exactcolor, context, SHADER_20b), str, g_pShaders[SHADER_20b], macros, &inc);
    
    								macros[macroindex].Name = NULL;
    								macros[macroindex].Definition = NULL;
    
    								LoadShader(GET_SHADER_INDEX(2, texfilter, texwrap, fog, writedepth, testaem, exactcolor, context, SHADER_20a), str, g_pShaders[SHADER_20a], macros, &inc);
    								LoadShader(GET_SHADER_INDEX(2, texfilter, texwrap, fog, writedepth, testaem, exactcolor, context, SHADER_30), str, g_pShaders[SHADER_30], macros, &inc);
    
                                    // tex32to16
    								sprintf(str, "Texture%s%d_tex32to16PS", fog?"Fog":"", texfilter);
    								inc.ps2x = 0;
    
    								macros[macroindex].Name = "ACCURATE_DECOMPRESSION";
    								macros[macroindex].Definition = "1";
    
    								if( texfilter == 0 && exactcolor == 0 ) {								
    									LoadShader(GET_SHADER_INDEX(3, texfilter, texwrap, fog, writedepth, testaem, exactcolor, context, SHADER_20), str, g_pShaders[SHADER_20], macros, &inc);
    								}
    
    								inc.ps2x = 1;
    
    								LoadShader(GET_SHADER_INDEX(3, texfilter, texwrap, fog, writedepth, testaem, exactcolor, context, SHADER_20b), str, g_pShaders[SHADER_20b], macros, &inc);
    
    								macros[macroindex].Name = NULL;
    								macros[macroindex].Definition = NULL;
    
    								LoadShader(GET_SHADER_INDEX(3, texfilter, texwrap, fog, writedepth, testaem, exactcolor, context, SHADER_20a), str, g_pShaders[SHADER_20a], macros, &inc);
    								LoadShader(GET_SHADER_INDEX(3, texfilter, texwrap, fog, writedepth, testaem, exactcolor, context, SHADER_30), str, g_pShaders[SHADER_30], macros, &inc);
    
                                    // tex16to8h
    								sprintf(str, "Texture%s%d_tex16to8hPS", fog?"Fog":"", texfilter);
    								inc.ps2x = 0;
    
    								macros[macroindex].Name = "ACCURATE_DECOMPRESSION";
    								macros[macroindex].Definition = "1";
    
    								inc.ps2x = 1;
    
    								LoadShader(GET_SHADER_INDEX(4, texfilter, texwrap, fog, writedepth, testaem, exactcolor, context, SHADER_20b), str, g_pShaders[SHADER_20b], macros, &inc);
    
    								macros[macroindex].Name = NULL;
    								macros[macroindex].Definition = NULL;
    
    								LoadShader(GET_SHADER_INDEX(4, texfilter, texwrap, fog, writedepth, testaem, exactcolor, context, SHADER_20a), str, g_pShaders[SHADER_20a], macros, &inc);
    								LoadShader(GET_SHADER_INDEX(4, texfilter, texwrap, fog, writedepth, testaem, exactcolor, context, SHADER_30), str, g_pShaders[SHADER_30], macros, &inc);
                                }
							}

							--macroindex;
							macros[macroindex].Name = NULL;
							macros[macroindex].Definition = NULL;
						}

						--macroindex;
						macros[macroindex].Name = NULL;
						macros[macroindex].Definition = NULL;
					}

					--macroindex;
					macros[macroindex].Name = NULL;
					macros[macroindex].Definition = NULL;
				}
			}
		}
	}

	// create the database
	FILE* fdst = fopen(dstfilename, "wb");
	if( fdst == NULL ) {
		printf("failed to open %s\n", dstfilename);
		return 0;
	}

	DWORD num = (DWORD)mapShaders.size();
	fwrite(&num, 4, 1, fdst);

	i = 0;
	DWORD totaloffset = 4+sizeof(SHADERHEADER)*num;
	for(map<DWORD, SHADERINFO>::iterator it = mapShaders.begin(); it != mapShaders.end(); ++it, ++i) {
		SHADERHEADER h;
		h.index = it->first | it->second.type;
		h.offset = totaloffset;
		h.size = it->second.pbuf->GetBufferSize();

		fseek(fdst, 4+i*sizeof(SHADERHEADER), SEEK_SET);
		fwrite(&h, sizeof(SHADERHEADER), 1, fdst);

		fseek(fdst, totaloffset, SEEK_SET);
		fwrite(it->second.pbuf->GetBufferPointer(), it->second.pbuf->GetBufferSize(), 1, fdst);

		totaloffset += it->second.pbuf->GetBufferSize();
		it->second.pbuf->Release();
	}

	fclose(fdst);

	printf("wrote %s\n", dstfilename);

	return 0;
}
