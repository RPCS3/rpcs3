#pragma once
#include "Emu/RSX/GSRender.h"

class NullGSRender : public GSRender
{
public:
	u64 get_cycles() final;

	NullGSRender(utils::serial* ar) noexcept;
	NullGSRender(utils::serial& ar) noexcept : NullGSRender(std::addressof(ar)) {}
	NullGSRender() noexcept : NullGSRender(nullptr) {}

private:
	void end() override;
};
