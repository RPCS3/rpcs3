#pragma once

struct RSXShaderProgram
{
	u32 size;
	u32 addr;
	u32 offset;

	RSXShaderProgram()
		: size(0)
		, addr(0)
		, offset(0)
	{
	}
};