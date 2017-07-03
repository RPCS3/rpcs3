#pragma once
#include "Emu/RSX/GSRender.h"

class NullGSRender final : public GSRender
{
public:
	NullGSRender();

private:
	bool do_method(u32 cmd, u32 value) override;
};
