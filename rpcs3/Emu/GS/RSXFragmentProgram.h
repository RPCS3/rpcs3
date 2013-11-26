#pragma once

struct RSXShaderProgram
{
	u32 size;
	u32 addr;
	u32 offset;
	u32 ctrl;

	RSXShaderProgram()
		: size(0)
		, addr(0)
		, offset(0)
		, ctrl(0)
	{
	}
};