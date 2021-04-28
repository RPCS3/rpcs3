#pragma once
#include "Emu/RSX/GSRender.h"

class NullGSRender : public GSRender
{
public:
	u64 get_cycles() final;
	NullGSRender();

private:
	void end() override;
};
