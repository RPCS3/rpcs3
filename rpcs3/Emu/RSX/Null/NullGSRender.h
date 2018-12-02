#pragma once
#include "Emu/RSX/GSRender.h"

class NullGSRender : public GSRender
{
public:
	u64 get_cycles() override final;
	NullGSRender();

private:
	bool do_method(u32 cmd, u32 value) override final;
	void end() override;
};
