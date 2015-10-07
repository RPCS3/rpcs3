#pragma once
#include "Emu/RSX/GSRender.h"

class NullGSRender final : public GSRender
{
public:
	NullGSRender();

private:
	void onexit_thread() override;
	bool domethod(u32 cmd, u32 value) override;
};
