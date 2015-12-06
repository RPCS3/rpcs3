#pragma once
#include "instr_table.h"
#include <common/basic_types.h>

namespace rpcs3
{
	using namespace common;

	namespace cell
	{
		class decoder
		{
		public:
			virtual void decode(const u32 code) = 0;
			virtual u32 decode_memory(const u32 address);
		};

		template<typename TO, uint from, uint to>
		static InstrList<(1 << (CodeField<from, to>::size)), TO>* new_list(const CodeField<from, to>& func, InstrCaller<TO>* error_func = nullptr)
		{
			return new InstrList<(1 << (CodeField<from, to>::size)), TO>(func, error_func);
		}

		template<int count, typename TO, uint from, uint to>
		static InstrList<(1 << (CodeField<from, to>::size)), TO>* new_list(InstrList<count, TO>* parent, int opcode, const CodeField<from, to>& func, InstrCaller<TO>* error_func = nullptr)
		{
			return connect_list(parent, new InstrList<(1 << (CodeField<from, to>::size)), TO>(func, error_func), opcode);
		}

		template<int count, typename TO, uint from, uint to>
		static InstrList<(1 << (CodeField<from, to>::size)), TO>* new_list(InstrList<count, TO>* parent, const CodeField<from, to>& func, InstrCaller<TO>* error_func = nullptr)
		{
			return connect_list(parent, new InstrList<(1 << (CodeField<from, to>::size)), TO>(func, error_func));
		}
	}
}