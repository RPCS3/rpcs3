#pragma once
#if defined(DX12_SUPPORT)
#include "Emu/RSX/RSXFragmentProgram.h"
#include <sstream>

#include "../Common/FragmentProgramDecompiler.h"

class D3D12FragmentDecompiler : public FragmentProgramDecompiler
{
protected:
	virtual std::string getFloatTypeName(size_t elementCount) override;
	virtual std::string getFunction(enum class FUNCTION) override;
	virtual std::string saturate(const std::string &code) override;

	virtual void insertHeader(std::stringstream &OS) override;
	virtual void insertIntputs(std::stringstream &OS) override;
	virtual void insertOutputs(std::stringstream &OS) override;
	virtual void insertConstants(std::stringstream &OS) override;
	virtual void insertMainStart(std::stringstream &OS) override;
	virtual void insertMainEnd(std::stringstream &OS) override;
public:
	D3D12FragmentDecompiler(u32 addr, u32& size, u32 ctrl);
};

#endif