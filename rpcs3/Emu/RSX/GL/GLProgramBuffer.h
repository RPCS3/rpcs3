#pragma once
#include "GLProgram.h"

struct GLBufferInfo
{
	u32 prog_id;
	u32 fp_id;
	u32 vp_id;
	std::vector<u8> fp_data;
	std::vector<u32> vp_data;
	std::string fp_shader;
	std::string vp_shader;
};

//TODO