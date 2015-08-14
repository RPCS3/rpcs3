#pragma once
#if defined(DX12_SUPPORT)
#include <vector>
#include <sstream>
#include "../Common/VertexProgramDecompiler.h"

struct D3D12VertexProgramDecompiler : public VertexProgramDecompiler
{
protected:
	virtual std::string getFloatTypeName(size_t elementCount) override;
	virtual std::string getFunction(enum class FUNCTION) override;
	virtual std::string compareFunction(enum class COMPARE, const std::string &, const std::string &) override;

	virtual void insertHeader(std::stringstream &OS);
	virtual void insertInputs(std::stringstream &OS, const std::vector<ParamType> &inputs);
	virtual void insertConstants(std::stringstream &OS, const std::vector<ParamType> &constants);
	virtual void insertOutputs(std::stringstream &OS, const std::vector<ParamType> &outputs);
	virtual void insertMainStart(std::stringstream &OS);
	virtual void insertMainEnd(std::stringstream &OS);
public:
	D3D12VertexProgramDecompiler(std::vector<u32>& data);
};
#endif