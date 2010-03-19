#define _CRT_SECURE_NO_DEPRECATE

// Builds all possible shader files from ps2hw.fx and stores them in
// a preprocessed database
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <assert.h>

#ifdef _WIN32
#	include <zlib/zlib.h>
#else
#	include <zlib.h>
#endif

#include "zpipe.h"

#include <Cg/cg.h>
#include <Cg/cgGL.h>

#define SAFE_RELEASE(x) { if( (x) != NULL ) { (x)->Release(); x = NULL; } }

#include <map>
#include <vector>

using namespace std;

#include "zerogsshaders.h"

char* srcfilename = "ps2hw.fx";
char* dstfilename = "ps2hw.dat";

#ifndef ArraySize
#define ArraySize(x) (sizeof(x) / sizeof((x)[0]))
#endif

struct SHADERINFO
{
	int type;
	vector<char> buf;
};

map<int, SHADERINFO> mapShaders;
CGcontext g_cgcontext;

void LoadShader(int index, const char* pshader, CGprofile prof, vector<const char*>& vargs, int context)
{
	vector<const char*> realargs;
	realargs.reserve(16);
	realargs.resize(vargs.size());
	if( vargs.size() > 0 )
		memcpy(&realargs[0], &vargs[0], realargs.size() * sizeof(realargs[0]));
	realargs.push_back(context ? "-Ictx1" : "-Ictx0");
	realargs.push_back(NULL);

	CGprogram prog = cgCreateProgramFromFile(g_cgcontext, CG_SOURCE, srcfilename, prof, pshader, &realargs[0]);
	if( !cgIsProgram(prog) ) {
		printf("Failed to load shader %s: \n%s\n", pshader, cgGetLastListing(g_cgcontext));
		return;
	}

	if( mapShaders.find(index) != mapShaders.end() ) {
		printf("error: two shaders share the same index %d\n", index);
		exit(0);
	}

	if( !cgIsProgramCompiled(prog) )
		cgCompileProgram(prog);
	
	const char* pstr = cgGetProgramString(prog, CG_COMPILED_PROGRAM);

	const char* pprog = strstr(pstr, "#program");
	if( pprog == NULL ) {
		printf("program field not found!\n");
		return;
	}
	pprog += 9;
	const char* progend = strchr(pprog, '\r');
	if( progend == NULL ) progend = strchr(pprog, '\n');

	if( progend == NULL ) {
		printf("prog end not found!\n");
		return;
	}

	const char* defname = "main";

	SHADERINFO info;
	info.type = 0;
	info.buf.resize(strlen(pstr)+1);
	
	// change the program name to main
	memset(&info.buf[0], 0, info.buf.size());
	memcpy(&info.buf[0], pstr, pprog-pstr);
	memcpy(&info.buf[pprog-pstr], defname, 4);
	memcpy(&info.buf[pprog-pstr+4], progend, strlen(pstr)-(progend-pstr));

	if( mapShaders.find(index) != mapShaders.end() )
		printf("same shader\n");
	assert( mapShaders.find(index) == mapShaders.end() );
	mapShaders[index] = info;

	cgDestroyProgram(prog);
}

int main(int argc, char** argv)
{
	printf("usage: [src] [dst] [opts]\n");

	if( argc >= 2 ) srcfilename = argv[1];
	if( argc >= 3 ) dstfilename = argv[2];

	FILE* fsrc = fopen(srcfilename, "r");
	if( fsrc == NULL ) {
		printf("cannot open %s\n", srcfilename);
		return 0;
	}
	fclose(fsrc);
	
	g_cgcontext = cgCreateContext();
	if( !cgIsContext(g_cgcontext) ) {
		printf("failed to create cg context\n");
		return -1;
	}

	CGprofile cgvProf = CG_PROFILE_ARBVP1;
	CGprofile cgfProf = CG_PROFILE_ARBFP1;
	if( !cgGLIsProfileSupported(cgvProf) != CG_TRUE ) {
		printf("arbvp1 not supported\n");
		return 0;
	}
	if( !cgGLIsProfileSupported(cgfProf) != CG_TRUE ) {
		printf("arbfp1 not supported\n");
		return 0;
	}

	cgGLEnableProfile(cgvProf);
	cgGLEnableProfile(cgfProf);
	cgGLSetOptimalOptions(cgvProf);
	cgGLSetOptimalOptions(cgfProf);

	vector<const char*> vmacros;

	LoadShader(SH_BITBLTVS, "BitBltVS", cgvProf, vmacros, 0);
	LoadShader(SH_BITBLTPS, "BitBltPS", cgfProf, vmacros, 0);
	LoadShader(SH_BITBLTDEPTHPS, "BitBltDepthPS", cgfProf, vmacros, 0);
	LoadShader(SH_BITBLTDEPTHMRTPS, "BitBltDepthMRTPS", cgfProf, vmacros, 0);
	LoadShader(SH_CRTCTARGPS, "CRTCTargPS", cgfProf, vmacros, 0);
	LoadShader(SH_CRTCPS, "CRTCPS", cgfProf, vmacros, 0);
	LoadShader(SH_CRTC_NEARESTPS, "CRTCPS_Nearest", cgfProf, vmacros, 0);
	LoadShader(SH_CRTC24PS, "CRTC24PS", cgfProf, vmacros, 0);
	LoadShader(SH_ZEROPS, "ZeroPS", cgfProf, vmacros, 0);
	LoadShader(SH_BASETEXTUREPS, "BaseTexturePS", cgfProf, vmacros, 0);
	LoadShader(SH_BITBLTAAPS, "BitBltPS", cgfProf, vmacros, 0);
	LoadShader(SH_CRTCTARGINTERPS, "CRTCTargInterPS", cgfProf, vmacros, 0);
	LoadShader(SH_CRTCINTERPS, "CRTCInterPS", cgfProf, vmacros, 0);
	LoadShader(SH_CRTCINTER_NEARESTPS, "CRTCInterPS_Nearest", cgfProf, vmacros, 0);
	LoadShader(SH_CRTC24INTERPS, "CRTC24InterPS", cgfProf, vmacros, 0);
	LoadShader(SH_CONVERT16TO32PS, "Convert16to32PS", cgfProf, vmacros, 0);
	LoadShader(SH_CONVERT32TO16PS, "Convert32to16PS", cgfProf, vmacros, 0);

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

	for(i = 0; i < ArraySize(vsshaders); ++i) {
		for(int writedepth = 0; writedepth < 2; ++writedepth ) {

			if( writedepth ) vmacros.push_back("-DWRITE_DEPTH");
			LoadShader(vsshaders[i]|(writedepth?SH_WRITEDEPTH:0), pvsshaders[i], cgvProf, vmacros, 0);
			LoadShader(vsshaders[i]|(writedepth?SH_WRITEDEPTH:0)|SH_CONTEXT1, pvsshaders[i], cgvProf, vmacros, 1);
			if( writedepth ) vmacros.pop_back();
		}
	}

	const int psshaders[2] = { SH_REGULARPS, SH_REGULARFOGPS };
	const char* ppsshaders[2] = { "RegularPS", "RegularFogPS" };

	for(i = 0; i < ArraySize(psshaders); ++i) {
		for(int writedepth = 0; writedepth < 2; ++writedepth ) {
			if( writedepth ) vmacros.push_back("-DWRITE_DEPTH");
			LoadShader(psshaders[i]|(writedepth?SH_WRITEDEPTH:0), ppsshaders[i], cgfProf, vmacros, 0);
			if( writedepth ) vmacros.pop_back();
		}
	}

	printf("creating shaders, note that ctx0/ps2hw_ctx.fx, and ctx1/ps2hw_ctx.fx are required\n");
	vmacros.resize(0);
		
	for(int texwrap = 0; texwrap < NUM_TEXWRAPS; ++texwrap ) {

		if( g_pPsTexWrap[texwrap] != NULL )
			vmacros.push_back(g_pPsTexWrap[texwrap]);

		for(int context = 0; context < 2; ++context) {
			
			for(int texfilter = 0; texfilter < NUM_FILTERS; ++texfilter) {
				for(int fog = 0; fog < 2; ++fog ) {
					for(int writedepth = 0; writedepth < 2; ++writedepth ) {

						if( writedepth )
							vmacros.push_back("-DWRITE_DEPTH");

						for(int testaem = 0; testaem < 2; ++testaem ) {

							if( testaem )
								vmacros.push_back("-DTEST_AEM");

							for(int exactcolor = 0; exactcolor < 2; ++exactcolor ) {
								
								if( exactcolor )
									vmacros.push_back("-DEXACT_COLOR");

								// 32
								sprintf(str, "Texture%s%d_32PS", fog?"Fog":"", texfilter);

								vmacros.push_back("-DACCURATE_DECOMPRESSION");
								LoadShader(GET_SHADER_INDEX(0, texfilter, texwrap, fog, writedepth, testaem, exactcolor, context, SHADER_ACCURATE), str, cgfProf, vmacros, context);
								vmacros.pop_back();

								LoadShader(GET_SHADER_INDEX(0, texfilter, texwrap, fog, writedepth, testaem, exactcolor, context, 0), str, cgfProf, vmacros, context);

								if( texfilter == 0 ) {
									// tex32
									sprintf(str, "Texture%s%d_tex32PS", fog?"Fog":"", texfilter);

//									vmacros.push_back("-DACCURATE_DECOMPRESSION");
//									LoadShader(GET_SHADER_INDEX(1, texfilter, texwrap, fog, writedepth, testaem, exactcolor, context, SHADER_ACCURATE), str, cgfProf, vmacros, context);
//									vmacros.pop_back();

									LoadShader(GET_SHADER_INDEX(1, texfilter, texwrap, fog, writedepth, testaem, exactcolor, context, 0), str, cgfProf, vmacros, context);

									// clut32
									sprintf(str, "Texture%s%d_clut32PS", fog?"Fog":"", texfilter);

//									vmacros.push_back("-DACCURATE_DECOMPRESSION");
//									LoadShader(GET_SHADER_INDEX(2, texfilter, texwrap, fog, writedepth, testaem, exactcolor, context, SHADER_ACCURATE), str, cgfProf, vmacros, context);
//									vmacros.pop_back();

									LoadShader(GET_SHADER_INDEX(2, texfilter, texwrap, fog, writedepth, testaem, exactcolor, context, 0), str, cgfProf, vmacros, context);

									// tex32to16
									sprintf(str, "Texture%s%d_tex32to16PS", fog?"Fog":"", texfilter);

//									vmacros.push_back("-DACCURATE_DECOMPRESSION");
//									LoadShader(GET_SHADER_INDEX(3, texfilter, texwrap, fog, writedepth, testaem, exactcolor, context, SHADER_ACCURATE), str, cgfProf, vmacros, context);
//									vmacros.pop_back();

									LoadShader(GET_SHADER_INDEX(3, texfilter, texwrap, fog, writedepth, testaem, exactcolor, context, 0), str, cgfProf, vmacros, context);

									// tex16to8h
									sprintf(str, "Texture%s%d_tex16to8hPS", fog?"Fog":"", texfilter);

//									vmacros.push_back("-DACCURATE_DECOMPRESSION");
//									LoadShader(GET_SHADER_INDEX(4, texfilter, texwrap, fog, writedepth, testaem, exactcolor, context, SHADER_ACCURATE), str, cgfProf, vmacros, context);
//									vmacros.pop_back();

									LoadShader(GET_SHADER_INDEX(4, texfilter, texwrap, fog, writedepth, testaem, exactcolor, context, 0), str, cgfProf, vmacros, context);
								}

								if( exactcolor )
									vmacros.pop_back();
							}

							if( testaem )
								vmacros.pop_back();
						}

						if( writedepth )
							vmacros.pop_back();
					}
				}
			}
		}

		if( g_pPsTexWrap[texwrap] != NULL )
			vmacros.pop_back();
	}

	if( vmacros.size() != 0 )
		printf("error with macros!\n");

	// create the database

	int num = (int)mapShaders.size();
	
	// first compress
	vector<char> buffer;
	buffer.reserve(10000000); // 10mb
	buffer.resize(sizeof(SHADERHEADER)*num);

	i = 0;	
	for(map<int, SHADERINFO>::iterator it = mapShaders.begin(); it != mapShaders.end(); ++it, ++i) {
		SHADERHEADER h;
		h.index = it->first | it->second.type;
		h.offset = (int)buffer.size();
		h.size = (int)it->second.buf.size();

		memcpy(&buffer[0] + i*sizeof(SHADERHEADER), &h, sizeof(SHADERHEADER));

		size_t cur = buffer.size();
		buffer.resize(cur + it->second.buf.size());
		memcpy(&buffer[cur], &it->second.buf[0], it->second.buf.size());
	}

	int compressed_size;
	int real_size = (int)buffer.size();
	vector<char> dst;
	dst.resize(buffer.size());
	def(&buffer[0], &dst[0], (int)buffer.size(), &compressed_size);

	// write to file
	// fmt: num shaders, size of compressed, compressed data
	FILE* fdst = fopen(dstfilename, "wb");
	if( fdst == NULL ) {
		printf("failed to open %s\n", dstfilename);
		return 0;
	}

	fwrite(&num, 4, 1, fdst);
	fwrite(&compressed_size, 4, 1, fdst);
	fwrite(&real_size, 4, 1, fdst);
	fwrite(&dst[0], compressed_size, 1, fdst);

	fclose(fdst);

	printf("wrote %s\n", dstfilename);

	cgDestroyContext(g_cgcontext);

	return 0;
}
