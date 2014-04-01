#pragma once
#include <algorithm>
#include "CPUInstrTable.h"
#pragma warning( disable : 4800 )

class CPUDecoder
{
public:
	virtual u8 DecodeMemory(const u64 address)=0;
};

template<typename TO>
class InstrCaller
{
public:
	virtual ~InstrCaller<TO>()
	{
	}

	virtual void operator ()(TO* op, u32 code) const = 0;

	virtual u32 operator [](u32) const
	{
		return 0;
	}
};

template<typename TO>
class InstrBinder_0 : public InstrCaller<TO>
{
	typedef void (TO::*func_t)();
	func_t m_func;

public:
	InstrBinder_0(func_t func)
		: InstrCaller<TO>()
		, m_func(func)
	{
	}

	virtual void operator ()(TO* op, u32 code) const
	{
		(op->*m_func)();
	}
};

template<typename TO, typename T1>
class InstrBinder_1 : public InstrCaller<TO>
{
	typedef void (TO::*func_t)(T1);
	func_t m_func;
	const CodeFieldBase& m_arg_func_1;

public:
	InstrBinder_1(func_t func, const CodeFieldBase& arg_func_1)
		: InstrCaller<TO>()
		, m_func(func)
		, m_arg_func_1(arg_func_1)
	{
	}

	virtual void operator ()(TO* op, u32 code) const
	{
		(op->*m_func)((T1)m_arg_func_1(code));
	}
};

template<typename TO, typename T1, typename T2>
class InstrBinder_2 : public InstrCaller<TO>
{
	typedef void (TO::*func_t)(T1, T2);
	func_t m_func;
	const CodeFieldBase& m_arg_func_1;
	const CodeFieldBase& m_arg_func_2;

public:
	InstrBinder_2(func_t func, const CodeFieldBase& arg_func_1, const CodeFieldBase& arg_func_2)
		: InstrCaller<TO>()
		, m_func(func)
		, m_arg_func_1(arg_func_1)
		, m_arg_func_2(arg_func_2)
	{
	}

	virtual void operator ()(TO* op, u32 code) const
	{
		(op->*m_func)(
			(T1)m_arg_func_1(code),
			(T2)m_arg_func_2(code)
		);
	}
};

template<typename TO, typename T1, typename T2, typename T3>
class InstrBinder_3 : public InstrCaller<TO>
{
	typedef void (TO::*func_t)(T1, T2, T3);
	func_t m_func;
	const CodeFieldBase& m_arg_func_1;
	const CodeFieldBase& m_arg_func_2;
	const CodeFieldBase& m_arg_func_3;

public:
	InstrBinder_3(func_t func,
		const CodeFieldBase& arg_func_1,
		const CodeFieldBase& arg_func_2,
		const CodeFieldBase& arg_func_3)
		: InstrCaller<TO>()
		, m_func(func)
		, m_arg_func_1(arg_func_1)
		, m_arg_func_2(arg_func_2)
		, m_arg_func_3(arg_func_3)
	{
	}

	virtual void operator ()(TO* op, u32 code) const
	{
		(op->*m_func)(
			(T1)m_arg_func_1(code),
			(T2)m_arg_func_2(code),
			(T3)m_arg_func_3(code)
		);
	}
};

template<typename TO, typename T1, typename T2, typename T3, typename T4>
class InstrBinder_4 : public InstrCaller<TO>
{
	typedef void (TO::*func_t)(T1, T2, T3, T4);
	func_t m_func;
	const CodeFieldBase& m_arg_func_1;
	const CodeFieldBase& m_arg_func_2;
	const CodeFieldBase& m_arg_func_3;
	const CodeFieldBase& m_arg_func_4;

public:
	InstrBinder_4(func_t func,
		const CodeFieldBase& arg_func_1,
		const CodeFieldBase& arg_func_2,
		const CodeFieldBase& arg_func_3,
		const CodeFieldBase& arg_func_4)
		: InstrCaller<TO>()
		, m_func(func)
		, m_arg_func_1(arg_func_1)
		, m_arg_func_2(arg_func_2)
		, m_arg_func_3(arg_func_3)
		, m_arg_func_4(arg_func_4)
	{
	}

	virtual void operator ()(TO* op, u32 code) const
	{
		(op->*m_func)(
			(T1)m_arg_func_1(code),
			(T2)m_arg_func_2(code),
			(T3)m_arg_func_3(code),
			(T4)m_arg_func_4(code)
		);
	}
};

template<typename TO, typename T1, typename T2, typename T3, typename T4, typename T5>
class InstrBinder_5 : public InstrCaller<TO>
{
	typedef void (TO::*func_t)(T1, T2, T3, T4, T5);
	func_t m_func;
	const CodeFieldBase& m_arg_func_1;
	const CodeFieldBase& m_arg_func_2;
	const CodeFieldBase& m_arg_func_3;
	const CodeFieldBase& m_arg_func_4;
	const CodeFieldBase& m_arg_func_5;

public:
	InstrBinder_5(func_t func, 
		const CodeFieldBase& arg_func_1,
		const CodeFieldBase& arg_func_2,
		const CodeFieldBase& arg_func_3,
		const CodeFieldBase& arg_func_4,
		const CodeFieldBase& arg_func_5)
		: InstrCaller<TO>()
		, m_func(func)
		, m_arg_func_1(arg_func_1)
		, m_arg_func_2(arg_func_2)
		, m_arg_func_3(arg_func_3)
		, m_arg_func_4(arg_func_4)
		, m_arg_func_5(arg_func_5)
	{
	}

	virtual void operator ()(TO* op, u32 code) const
	{
		(op->*m_func)(
			(T1)m_arg_func_1(code),
			(T2)m_arg_func_2(code),
			(T3)m_arg_func_3(code),
			(T4)m_arg_func_4(code),
			(T5)m_arg_func_5(code)
		);
	}
};

template<typename TO, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
class InstrBinder_6 : public InstrCaller<TO>
{
	typedef void (TO::*func_t)(T1, T2, T3, T4, T5, T6);
	func_t m_func;
	const CodeFieldBase& m_arg_func_1;
	const CodeFieldBase& m_arg_func_2;
	const CodeFieldBase& m_arg_func_3;
	const CodeFieldBase& m_arg_func_4;
	const CodeFieldBase& m_arg_func_5;
	const CodeFieldBase& m_arg_func_6;

public:
	InstrBinder_6(func_t func, 
		const CodeFieldBase& arg_func_1,
		const CodeFieldBase& arg_func_2,
		const CodeFieldBase& arg_func_3,
		const CodeFieldBase& arg_func_4,
		const CodeFieldBase& arg_func_5,
		const CodeFieldBase& arg_func_6)
		: InstrCaller<TO>()
		, m_func(func)
		, m_arg_func_1(arg_func_1)
		, m_arg_func_2(arg_func_2)
		, m_arg_func_3(arg_func_3)
		, m_arg_func_4(arg_func_4)
		, m_arg_func_5(arg_func_5)
		, m_arg_func_6(arg_func_6)
	{
	}

	virtual void operator ()(TO* op, u32 code) const
	{
		(op->*m_func)(
			(T1)m_arg_func_1(code),
			(T2)m_arg_func_2(code),
			(T3)m_arg_func_3(code),
			(T4)m_arg_func_4(code),
			(T5)m_arg_func_5(code),
			(T6)m_arg_func_6(code)
		);
	}
};

template<typename TO>
InstrCaller<TO>* instr_bind(void (TO::*func)())
{
	return new InstrBinder_0<TO>(func);
}

template<typename TO, typename T1>
InstrCaller<TO>* instr_bind(void (TO::*func)(T1), const CodeFieldBase& arg_func_1)
{
	return new InstrBinder_1<TO, T1>(func, arg_func_1);
}

template<typename TO, typename T1, typename T2>
InstrCaller<TO>* instr_bind(void (TO::*func)(T1, T2), 
	const CodeFieldBase& arg_func_1,
	const CodeFieldBase& arg_func_2)
{
	return new InstrBinder_2<TO, T1, T2>(func, arg_func_1, arg_func_2);
}

template<typename TO, typename T1, typename T2, typename T3>
InstrCaller<TO>* instr_bind(void (TO::*func)(T1, T2, T3), 
	const CodeFieldBase& arg_func_1,
	const CodeFieldBase& arg_func_2,
	const CodeFieldBase& arg_func_3)
{
	return new InstrBinder_3<TO, T1, T2, T3>(func, arg_func_1, arg_func_2, arg_func_3);
}

template<typename TO, typename T1, typename T2, typename T3, typename T4>
InstrCaller<TO>* instr_bind(void (TO::*func)(T1, T2, T3, T4), 
	const CodeFieldBase& arg_func_1,
	const CodeFieldBase& arg_func_2,
	const CodeFieldBase& arg_func_3,
	const CodeFieldBase& arg_func_4)
{
	return new InstrBinder_4<TO, T1, T2, T3, T4>(func, arg_func_1, arg_func_2, arg_func_3, arg_func_4);
}

template<typename TO, typename T1, typename T2, typename T3, typename T4, typename T5>
InstrCaller<TO>* instr_bind(void (TO::*func)(T1, T2, T3, T4, T5), 
	const CodeFieldBase& arg_func_1,
	const CodeFieldBase& arg_func_2,
	const CodeFieldBase& arg_func_3,
	const CodeFieldBase& arg_func_4,
	const CodeFieldBase& arg_func_5)
{
	return new InstrBinder_5<TO, T1, T2, T3, T4, T5>(func, arg_func_1, arg_func_2, arg_func_3, arg_func_4, arg_func_5);
}

template<typename TO, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
InstrCaller<TO>* instr_bind(void (TO::*func)(T1, T2, T3, T4, T5, T6), 
	const CodeFieldBase& arg_func_1,
	const CodeFieldBase& arg_func_2,
	const CodeFieldBase& arg_func_3,
	const CodeFieldBase& arg_func_4,
	const CodeFieldBase& arg_func_5,
	const CodeFieldBase& arg_func_6)
{
	return new InstrBinder_6<TO, T1, T2, T3, T4, T5, T6>(func, arg_func_1, arg_func_2, arg_func_3, arg_func_4, arg_func_5, arg_func_6);
}

template<typename TO>
class InstrBase : public InstrCaller<TO>
{
protected:
	std::string m_name;
	const u32 m_opcode;
	CodeFieldBase** m_args;
	const uint m_args_count;

public:
	InstrBase(const std::string& name, int opcode, uint args_count)
		: InstrCaller<TO>()
		, m_name(name)
		, m_opcode(opcode)
		, m_args_count(args_count)
		, m_args(args_count ? new CodeFieldBase*[args_count] : nullptr)
	{
			std::transform(
				name.begin(),
				name.end(),
				m_name.begin(),
				[](const char &a)
				{
					char b = tolower(a);
					if (b == '_') b = '.';
					return b;
				});
	}

	__forceinline const std::string& GetName() const
	{
		return m_name;
	}

	__forceinline const uint GetArgCount() const
	{
		return m_args_count;
	}

	__forceinline const CodeFieldBase& GetArg(uint index) const
	{
		assert(index < m_args_count);
		return *m_args[index];
	}

	void operator ()(TO* op, u32 code) const
	{
		decode(op, code);
	}

	u32 operator()(const Array<u32>& args) const
	{
		return encode(args);
	}

	virtual void decode(TO* op, u32 code) const=0;
	virtual u32 encode(const Array<u32>& args) const=0;
};

template<int _count, typename TO>
class InstrList : public InstrCaller<TO>
{
public:
	static const int count = _count;

protected:
	const CodeFieldBase& m_func;
	InstrCaller<TO>* m_instrs[count];
	InstrBase<TO>* m_instrs_info[count];
	InstrCaller<TO>* m_error_func;
	InstrCaller<TO>* m_parent;
	int m_opcode;

public:
	InstrList(const CodeFieldBase& func, InstrCaller<TO>* error_func)
		: InstrCaller<TO>()
		, m_func(func)
		, m_error_func(error_func)
		, m_parent(nullptr)
		, m_opcode(-1)
	{
		for(int i=0; i<count; ++i)
		{
			m_instrs[i] = error_func;
		}

		memset(m_instrs_info, 0, sizeof(InstrBase<TO>*) * count);
	}

	virtual ~InstrList()
	{
		for(int i=0; i<count; ++i)
		{
			delete m_instrs[i];
		}

		delete m_error_func;
	}

	void set_parent(InstrCaller<TO>* parent, int opcode)
	{
		m_opcode = opcode;
		m_parent = parent;
	}

	InstrCaller<TO>* get_parent() const
	{
		return m_parent;
	}

	u32 get_opcode() const
	{
		return m_opcode;
	}

	void set_error_func(InstrCaller<TO>* error_func)
	{
		for(int i=0; i<count; ++i)
		{
			if(m_instrs[i] == m_error_func || !m_instrs[i])
			{
				m_instrs[i] = error_func;
			}
		}

		m_error_func = error_func;
	}

	void set_instr(uint pos, InstrCaller<TO>* func, InstrBase<TO>* info = nullptr)
	{
		assert(pos < count);
		m_instrs[pos] = func;
		m_instrs_info[pos] = info;
	}

	InstrCaller<TO>* get_instr(int pos) const
	{
		assert(pos < count);
		return m_instrs[pos];
	}

	InstrBase<TO>* get_instr_info(int pos) const
	{
		assert(pos < count);
		return m_instrs_info[pos];
	}

	u32 encode(u32 entry) const
	{
		return m_func[entry] | (m_parent ? (*m_parent)[m_opcode] : 0);
	}

	void decode(TO* op, u32 entry, u32 code) const
	{
		(*m_instrs[entry])(op, code);
	}

	virtual void operator ()(TO* op, u32 code) const
	{
		decode(op, m_func(code) & (count - 1), code);
	}

	virtual u32 operator [](u32 entry) const
	{
		return encode(entry);
	}
};

template<int count1, int count2, typename TO>
static InstrList<count2, TO>* connect_list(InstrList<count1, TO>* parent, InstrList<count2, TO>* child, int opcode)
{
	parent->set_instr(opcode, child);
	child->set_parent(parent, opcode);
	return child;
}

template<int count1, int count2, typename TO>
static InstrList<count2, TO>* connect_list(InstrList<count1, TO>* parent, InstrList<count2, TO>* child)
{
	parent->set_error_func(child);
	child->set_parent(parent->get_parent(), parent->get_opcode());
	return child;
}

template<typename TO, int opcode, int count>
class Instr0 : public InstrBase<TO>
{
	InstrList<count, TO>& m_list;

public:
	Instr0(InstrList<count, TO>* list, const std::string& name,
			void (TO::*func)())
		: InstrBase<TO>(name, opcode, 0)
		, m_list(*list)
	{
		m_list.set_instr(opcode, instr_bind(func), this);
	}

	virtual void decode(TO* op, u32 code) const
	{
		m_list.decode(op, opcode, code);
	}

	virtual u32 encode(const Array<u32>& args) const
	{
		assert(args.GetCount() == InstrBase<TO>::m_args_count);
		return m_list.encode(opcode);
	}

	u32 encode() const
	{
		return m_list.encode(opcode);
	}
	
	u32 operator()() const
	{
		return encode();
	}
};

template<typename TO, int opcode, int count, typename T1>
class Instr1 : public InstrBase<TO>
{
	InstrList<count, TO>& m_list;

public:
	Instr1(InstrList<count, TO>* list, const std::string& name,
			void (TO::*func)(T1),
			CodeFieldBase& arg_1)
		: InstrBase<TO>(name, opcode, 1)
		, m_list(*list)
	{
		InstrBase<TO>::m_args[0] = &arg_1;

		m_list.set_instr(opcode, instr_bind(func, arg_1), this);
	}

	virtual void decode(TO* op, u32 code) const
	{
		m_list.decode(op, opcode, code);
	}

	virtual u32 encode(const Array<u32>& args) const
	{
		assert(args.GetCount() == InstrBase<TO>::m_args_count);
		return m_list.encode(opcode) | (*InstrBase<TO>::m_args[0])[args[0]];
	}

	u32 encode(T1 a1) const
	{
		return m_list.encode(opcode) | (*InstrBase<TO>::m_args[0])[a1];
	}
	
	u32 operator()(T1 a1) const
	{
		return encode(a1);
	}
};

template<typename TO, int opcode, int count, typename T1, typename T2>
class Instr2 : public InstrBase<TO>
{
	InstrList<count, TO>& m_list;

public:
	Instr2(InstrList<count, TO>* list, const std::string& name,
			void (TO::*func)(T1, T2),
			CodeFieldBase& arg_1,
			CodeFieldBase& arg_2)
		: InstrBase<TO>(name, opcode, 2)
		, m_list(*list)
	{
		InstrBase<TO>::m_args[0] = &arg_1;
		InstrBase<TO>::m_args[1] = &arg_2;

		m_list.set_instr(opcode, instr_bind(func, arg_1, arg_2), this);
	}

	virtual void decode(TO* op, u32 code) const
	{
		m_list.decode(op, opcode, code);
	}

	virtual u32 encode(const Array<u32>& args) const
	{
		assert(args.GetCount() == InstrBase<TO>::m_args_count);
		return m_list.encode(opcode) | (*InstrBase<TO>::m_args[0])[args[0]] | (*InstrBase<TO>::m_args[1])[args[1]];
	}

	u32 encode(T1 a1, T2 a2) const
	{
		return m_list.encode(opcode) | (*InstrBase<TO>::m_args[0])[a1] | (*InstrBase<TO>::m_args[1])[a2];
	}
	
	u32 operator()(T1 a1, T2 a2) const
	{
		return encode(a1, a2);
	}
};

template<typename TO, int opcode, int count, typename T1, typename T2, typename T3>
class Instr3 : public InstrBase<TO>
{
	InstrList<count, TO>& m_list;

public:
	Instr3(InstrList<count, TO>* list, const std::string& name,
			void (TO::*func)(T1, T2, T3),
			CodeFieldBase& arg_1,
			CodeFieldBase& arg_2,
			CodeFieldBase& arg_3)
		: InstrBase<TO>(name, opcode, 3)
		, m_list(*list)
	{
		InstrBase<TO>::m_args[0] = &arg_1;
		InstrBase<TO>::m_args[1] = &arg_2;
		InstrBase<TO>::m_args[2] = &arg_3;

		m_list.set_instr(opcode, instr_bind(func, arg_1, arg_2, arg_3), this);
	}

	virtual void decode(TO* op, u32 code) const
	{
		m_list.decode(op, opcode, code);
	}

	virtual u32 encode(const Array<u32>& args) const
	{
		assert(args.GetCount() == InstrBase<TO>::m_args_count);
		return m_list.encode(opcode) | (*InstrBase<TO>::m_args[0])[args[0]] | (*InstrBase<TO>::m_args[1])[args[1]] | (*InstrBase<TO>::m_args[2])[args[2]];
	}

	u32 encode(T1 a1, T2 a2, T3 a3) const
	{
		return m_list.encode(opcode) | (*InstrBase<TO>::m_args[0])[a1] | (*InstrBase<TO>::m_args[1])[a2] | (*InstrBase<TO>::m_args[2])[a3];
	}
	
	u32 operator()(T1 a1, T2 a2, T3 a3) const
	{
		return encode(a1, a2, a3);
	}
};

template<typename TO, int opcode, int count, typename T1, typename T2, typename T3, typename T4>
class Instr4 : public InstrBase<TO>
{
	InstrList<count, TO>& m_list;

public:
	Instr4(InstrList<count, TO>* list, const std::string& name,
			void (TO::*func)(T1, T2, T3, T4),
			CodeFieldBase& arg_1,
			CodeFieldBase& arg_2,
			CodeFieldBase& arg_3,
			CodeFieldBase& arg_4)
		: InstrBase<TO>(name, opcode, 4)
		, m_list(*list)
	{
		InstrBase<TO>::m_args[0] = &arg_1;
		InstrBase<TO>::m_args[1] = &arg_2;
		InstrBase<TO>::m_args[2] = &arg_3;
		InstrBase<TO>::m_args[3] = &arg_4;

		m_list.set_instr(opcode, instr_bind(func, arg_1, arg_2, arg_3, arg_4), this);
	}

	virtual void decode(TO* op, u32 code) const
	{
		m_list.decode(op, opcode, code);
	}

	virtual u32 encode(const Array<u32>& args) const
	{
		assert(args.GetCount() == InstrBase<TO>::m_args_count);
		return m_list.encode(opcode) | 
			(*InstrBase<TO>::m_args[0])[args[0]] |
			(*InstrBase<TO>::m_args[1])[args[1]] |
			(*InstrBase<TO>::m_args[2])[args[2]] |
			(*InstrBase<TO>::m_args[3])[args[3]];
	}

	u32 encode(T1 a1, T2 a2, T3 a3, T4 a4) const
	{
		return m_list.encode(opcode) |
			(*InstrBase<TO>::m_args[0])[a1] |
			(*InstrBase<TO>::m_args[1])[a2] |
			(*InstrBase<TO>::m_args[2])[a3] |
			(*InstrBase<TO>::m_args[3])[a4];
	}
	
	u32 operator()(T1 a1, T2 a2, T3 a3, T4 a4) const
	{
		return encode(a1, a2, a3, a4);
	}
};

template<typename TO, int opcode, int count, typename T1, typename T2, typename T3, typename T4, typename T5>
class Instr5 : public InstrBase<TO>
{
	InstrList<count, TO>& m_list;

public:
	Instr5(InstrList<count, TO>* list, const std::string& name,
			void (TO::*func)(T1, T2, T3, T4, T5),
			CodeFieldBase& arg_1,
			CodeFieldBase& arg_2,
			CodeFieldBase& arg_3,
			CodeFieldBase& arg_4,
			CodeFieldBase& arg_5)
		: InstrBase<TO>(name, opcode, 5)
		, m_list(*list)
	{
		InstrBase<TO>::m_args[0] = &arg_1;
		InstrBase<TO>::m_args[1] = &arg_2;
		InstrBase<TO>::m_args[2] = &arg_3;
		InstrBase<TO>::m_args[3] = &arg_4;
		InstrBase<TO>::m_args[4] = &arg_5;

		m_list.set_instr(opcode, instr_bind(func, arg_1, arg_2, arg_3, arg_4, arg_5), this);
	}

	virtual void decode(TO* op, u32 code) const
	{
		m_list.decode(op, opcode, code);
	}

	virtual u32 encode(const Array<u32>& args) const
	{
		assert(args.GetCount() == InstrBase<TO>::m_args_count);
		return m_list.encode(opcode) | 
			(*InstrBase<TO>::m_args[0])[args[0]] |
			(*InstrBase<TO>::m_args[1])[args[1]] |
			(*InstrBase<TO>::m_args[2])[args[2]] |
			(*InstrBase<TO>::m_args[3])[args[3]] |
			(*InstrBase<TO>::m_args[4])[args[4]];
	}

	u32 encode(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5) const
	{
		return m_list.encode(opcode) |
			(*InstrBase<TO>::m_args[0])[a1] |
			(*InstrBase<TO>::m_args[1])[a2] |
			(*InstrBase<TO>::m_args[2])[a3] |
			(*InstrBase<TO>::m_args[3])[a4] |
			(*InstrBase<TO>::m_args[4])[a5];
	}
	
	u32 operator()(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5) const
	{
		return encode(a1, a2, a3, a4, a5);
	}
};

template<typename TO, int opcode, int count, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
class Instr6 : public InstrBase<TO>
{
	InstrList<count, TO>& m_list;

public:
	Instr6(InstrList<count, TO>* list, const std::string& name,
			void (TO::*func)(T1, T2, T3, T4, T5, T6),
			CodeFieldBase& arg_1,
			CodeFieldBase& arg_2,
			CodeFieldBase& arg_3,
			CodeFieldBase& arg_4,
			CodeFieldBase& arg_5,
			CodeFieldBase& arg_6)
		: InstrBase<TO>(name, opcode, 6)
		, m_list(*list)
	{
		InstrBase<TO>::m_args[0] = &arg_1;
		InstrBase<TO>::m_args[1] = &arg_2;
		InstrBase<TO>::m_args[2] = &arg_3;
		InstrBase<TO>::m_args[3] = &arg_4;
		InstrBase<TO>::m_args[4] = &arg_5;
		InstrBase<TO>::m_args[5] = &arg_6;

		m_list.set_instr(opcode, instr_bind(func, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6), this);
	}

	virtual void decode(TO* op, u32 code) const
	{
		m_list.decode(op, opcode, code);
	}

	virtual u32 encode(const Array<u32>& args) const
	{
		assert(args.GetCount() == InstrBase<TO>::m_args_count);
		return m_list.encode(opcode) | 
			(*InstrBase<TO>::m_args[0])[args[0]] |
			(*InstrBase<TO>::m_args[1])[args[1]] |
			(*InstrBase<TO>::m_args[2])[args[2]] |
			(*InstrBase<TO>::m_args[3])[args[3]] |
			(*InstrBase<TO>::m_args[4])[args[4]] |
			(*InstrBase<TO>::m_args[5])[args[5]];
	}

	u32 encode(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T5 a6) const
	{
		return m_list.encode(opcode) |
			(*InstrBase<TO>::m_args[0])[a1] |
			(*InstrBase<TO>::m_args[1])[a2] |
			(*InstrBase<TO>::m_args[2])[a3] |
			(*InstrBase<TO>::m_args[3])[a4] |
			(*InstrBase<TO>::m_args[4])[a5] |
			(*InstrBase<TO>::m_args[5])[a6];
	}
	
	u32 operator()(T1 a1, T2 a2, T3 a3, T4 a4, T4 a5, T4 a6) const
	{
		return encode(a1, a2, a3, a4, a5, a6);
	}
};

template<int opcode, typename TO, int count>
static Instr0<TO, opcode, count>& make_instr(InstrList<count, TO>* list, const std::string& name, void (TO::*func)())
{
	return *new Instr0<TO, opcode, count>(list, name, func);
}

template<int opcode, typename TO, int count, typename T1>
static Instr1<TO, opcode, count, T1>& make_instr(InstrList<count, TO>* list, const std::string& name,
	void (TO::*func)(T1),
	CodeFieldBase& arg_1)
{
	return *new Instr1<TO, opcode, count, T1>(list, name, func, arg_1);
}

template<int opcode, typename TO, int count, typename T1, typename T2>
static Instr2<TO, opcode, count, T1, T2>& make_instr(InstrList<count, TO>* list, const std::string& name,
	void (TO::*func)(T1, T2),
	CodeFieldBase& arg_1,
	CodeFieldBase& arg_2)
{
	return *new Instr2<TO, opcode, count, T1, T2>(list, name, func, arg_1, arg_2);
}

template<int opcode, typename TO, int count, typename T1, typename T2, typename T3>
static Instr3<TO, opcode, count, T1, T2, T3>& make_instr(InstrList<count, TO>* list, const std::string& name,
	void (TO::*func)(T1, T2, T3),
	CodeFieldBase& arg_1,
	CodeFieldBase& arg_2,
	CodeFieldBase& arg_3)
{
	return *new Instr3<TO, opcode, count, T1, T2, T3>(list, name, func, arg_1, arg_2, arg_3);
}

template<int opcode, typename TO, int count, typename T1, typename T2, typename T3, typename T4>
static Instr4<TO, opcode, count, T1, T2, T3, T4>& make_instr(InstrList<count, TO>* list, const std::string& name,
	void (TO::*func)(T1, T2, T3, T4),
	CodeFieldBase& arg_1,
	CodeFieldBase& arg_2,
	CodeFieldBase& arg_3,
	CodeFieldBase& arg_4)
{
	return *new Instr4<TO, opcode, count, T1, T2, T3, T4>(list, name, func, arg_1, arg_2, arg_3, arg_4);
}

template<int opcode, typename TO, int count, typename T1, typename T2, typename T3, typename T4, typename T5>
static Instr5<TO, opcode, count, T1, T2, T3, T4, T5>& make_instr(InstrList<count, TO>* list, const std::string& name,
	void (TO::*func)(T1, T2, T3, T4, T5),
	CodeFieldBase& arg_1,
	CodeFieldBase& arg_2,
	CodeFieldBase& arg_3,
	CodeFieldBase& arg_4,
	CodeFieldBase& arg_5)
{
	return *new Instr5<TO, opcode, count, T1, T2, T3, T4, T5>(list, name, func, arg_1, arg_2, arg_3, arg_4, arg_5);
}

template<int opcode, typename TO, int count, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
static Instr6<TO, opcode, count, T1, T2, T3, T4, T5, T6>& make_instr(InstrList<count, TO>* list, const std::string& name,
	void (TO::*func)(T1, T2, T3, T4, T5, T6),
	CodeFieldBase& arg_1,
	CodeFieldBase& arg_2,
	CodeFieldBase& arg_3,
	CodeFieldBase& arg_4,
	CodeFieldBase& arg_5,
	CodeFieldBase& arg_6)
{
	return *new Instr6<TO, opcode, count, T1, T2, T3, T4, T5, T6>(list, name, func, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
}
