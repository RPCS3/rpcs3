#pragma once
#include "Emu/CPU/CPUDecoder.h"
#include "PPCInstrTable.h"

class PPCDecoder : public CPUDecoder
{
public:
	virtual void Decode(const u32 code)=0;

	virtual void DecodeMemory(const u64 address);
};


template<typename TO, uint from, uint to>
static InstrList<1 << CodeField<from, to>::size, TO>* new_list(const CodeField<from, to>& func, InstrCaller<TO>* error_func = nullptr)
{
	return new InstrList<1 << CodeField<from, to>::size, TO>(func, error_func);
}

template<int count, typename TO, uint from, uint to>
static InstrList<1 << CodeField<from, to>::size, TO>* new_list(InstrList<count, TO>* parent, int opcode, const CodeField<from, to>& func, InstrCaller<TO>* error_func = nullptr)
{
	return connect_list(parent, new InstrList<1 << CodeField<from, to>::size, TO>(func, error_func), opcode);
}

template<int count, typename TO, uint from, uint to>
static InstrList<1 << CodeField<from, to>::size, TO>* new_list(InstrList<count, TO>* parent, const CodeField<from, to>& func, InstrCaller<TO>* error_func = nullptr)
{
	return connect_list(parent, new InstrList<1 << CodeField<from, to>::size, TO>(func, error_func));
}