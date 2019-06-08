#pragma once
#include "Emu/RSX/GSRender.h"

class NullGSRender : public GSRender
{
public:
	u64 get_cycles() final;
	NullGSRender();

private:
	bool do_method(u32 cmd, u32 value) final;
	void end() override;
};
