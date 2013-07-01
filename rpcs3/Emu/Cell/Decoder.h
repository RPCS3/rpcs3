#pragma once

class Decoder
{
protected:
	class instr_caller
	{
	public:
		virtual ~instr_caller()
		{
		}

		virtual void operator ()() = 0;
	};

	instr_caller* m_instr_decoder;

	template<typename TO>
	class instr_binder_0 : public instr_caller
	{
		typedef void (TO::*func_t)();
		TO* m_op;
		func_t m_func;

	public:
		instr_binder_0(TO* op, func_t func)
			: instr_caller()
			, m_op(op)
			, m_func(func)
		{
		}

		virtual void operator ()()
		{
			(m_op->*m_func)();
		}
	};

	template<typename TO, typename TD, typename T1>
	class instr_binder_1 : public instr_caller
	{
		typedef void (TO::*func_t)(T1);
		typedef T1 (TD::*arg_1_t)();
		TO* m_op;
		TD* m_dec;
		func_t m_func;
		arg_1_t m_arg_func_1;

	public:
		instr_binder_1(TO* op, TD* dec, func_t func, arg_1_t arg_func_1)
			: instr_caller()
			, m_op(op)
			, m_dec(dec)
			, m_func(func)
			, m_arg_func_1(arg_func_1)
		{
		}

		virtual void operator ()()
		{
			(m_op->*m_func)((m_dec->*m_arg_func_1)());
		}
	};

	template<typename TO, typename TD, typename T1, typename T2>
	class instr_binder_2 : public instr_caller
	{
		typedef void (TO::*func_t)(T1, T2);
		typedef T1 (TD::*arg_1_t)();
		typedef T2 (TD::*arg_2_t)();
		TO* m_op;
		TD* m_dec;
		func_t m_func;
		arg_1_t m_arg_func_1;
		arg_2_t m_arg_func_2;

	public:
		instr_binder_2(TO* op, TD* dec, func_t func, arg_1_t arg_func_1, arg_2_t arg_func_2)
			: instr_caller()
			, m_op(op)
			, m_dec(dec)
			, m_func(func)
			, m_arg_func_1(arg_func_1)
			, m_arg_func_2(arg_func_2)
		{
		}

		virtual void operator ()()
		{
			(m_op->*m_func)(
				(m_dec->*m_arg_func_1)(),
				(m_dec->*m_arg_func_2)()
			);
		}
	};

	template<typename TO, typename TD, typename T1, typename T2, typename T3>
	class instr_binder_3 : public instr_caller
	{
		typedef void (TO::*func_t)(T1, T2, T3);
		typedef T1 (TD::*arg_1_t)();
		typedef T2 (TD::*arg_2_t)();
		typedef T3 (TD::*arg_3_t)();
		TO* m_op;
		TD* m_dec;
		func_t m_func;
		arg_1_t m_arg_func_1;
		arg_2_t m_arg_func_2;
		arg_3_t m_arg_func_3;

	public:
		instr_binder_3(TO* op, TD* dec, func_t func,
			arg_1_t arg_func_1,
			arg_2_t arg_func_2,
			arg_3_t arg_func_3)
			: instr_caller()
			, m_op(op)
			, m_dec(dec)
			, m_func(func)
			, m_arg_func_1(arg_func_1)
			, m_arg_func_2(arg_func_2)
			, m_arg_func_3(arg_func_3)
		{
		}

		virtual void operator ()()
		{
			(m_op->*m_func)(
				(m_dec->*m_arg_func_1)(),
				(m_dec->*m_arg_func_2)(),
				(m_dec->*m_arg_func_3)()
			);
		}
	};

	template<typename TO, typename TD, typename T1, typename T2, typename T3, typename T4>
	class instr_binder_4 : public instr_caller
	{
		typedef void (TO::*func_t)(T1, T2, T3, T4);
		typedef T1 (TD::*arg_1_t)();
		typedef T2 (TD::*arg_2_t)();
		typedef T3 (TD::*arg_3_t)();
		typedef T4 (TD::*arg_4_t)();
		TO* m_op;
		TD* m_dec;
		func_t m_func;
		arg_1_t m_arg_func_1;
		arg_2_t m_arg_func_2;
		arg_3_t m_arg_func_3;
		arg_4_t m_arg_func_4;

	public:
		instr_binder_4(TO* op, TD* dec, func_t func,
			arg_1_t arg_func_1,
			arg_2_t arg_func_2,
			arg_3_t arg_func_3,
			arg_4_t arg_func_4)
			: instr_caller()
			, m_op(op)
			, m_dec(dec)
			, m_func(func)
			, m_arg_func_1(arg_func_1)
			, m_arg_func_2(arg_func_2)
			, m_arg_func_3(arg_func_3)
			, m_arg_func_4(arg_func_4)
		{
		}

		virtual void operator ()()
		{
			(m_op->*m_func)(
				(m_dec->*m_arg_func_1)(),
				(m_dec->*m_arg_func_2)(),
				(m_dec->*m_arg_func_3)(),
				(m_dec->*m_arg_func_4)()
			);
		}
	};

	template<typename TO, typename TD, typename T1, typename T2, typename T3, typename T4, typename T5>
	class instr_binder_5 : public instr_caller
	{
		typedef void (TO::*func_t)(T1, T2, T3, T4, T5);
		typedef T1 (TD::*arg_1_t)();
		typedef T2 (TD::*arg_2_t)();
		typedef T3 (TD::*arg_3_t)();
		typedef T4 (TD::*arg_4_t)();
		typedef T5 (TD::*arg_5_t)();
		TO* m_op;
		TD* m_dec;
		func_t m_func;
		arg_1_t m_arg_func_1;
		arg_2_t m_arg_func_2;
		arg_3_t m_arg_func_3;
		arg_4_t m_arg_func_4;
		arg_5_t m_arg_func_5;

	public:
		instr_binder_5(TO* op, TD* dec, func_t func, 
			arg_1_t arg_func_1,
			arg_2_t arg_func_2,
			arg_3_t arg_func_3,
			arg_4_t arg_func_4,
			arg_5_t arg_func_5)
			: instr_caller()
			, m_op(op)
			, m_dec(dec)
			, m_func(func)
			, m_arg_func_1(arg_func_1)
			, m_arg_func_2(arg_func_2)
			, m_arg_func_3(arg_func_3)
			, m_arg_func_4(arg_func_4)
			, m_arg_func_5(arg_func_5)
		{
		}

		virtual void operator ()()
		{
			(m_op->*m_func)(
				(m_dec->*m_arg_func_1)(),
				(m_dec->*m_arg_func_2)(),
				(m_dec->*m_arg_func_3)(),
				(m_dec->*m_arg_func_4)(),
				(m_dec->*m_arg_func_5)()
			);
		}
	};

	template<typename TO, typename TD, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
	class instr_binder_6 : public instr_caller
	{
		typedef void (TO::*func_t)(T1, T2, T3, T4, T5, T6);
		typedef T1 (TD::*arg_1_t)();
		typedef T2 (TD::*arg_2_t)();
		typedef T3 (TD::*arg_3_t)();
		typedef T4 (TD::*arg_4_t)();
		typedef T5 (TD::*arg_5_t)();
		typedef T6 (TD::*arg_6_t)();
		TO* m_op;
		TD* m_dec;
		func_t m_func;
		arg_1_t m_arg_func_1;
		arg_2_t m_arg_func_2;
		arg_3_t m_arg_func_3;
		arg_4_t m_arg_func_4;
		arg_5_t m_arg_func_5;
		arg_6_t m_arg_func_6;

	public:
		instr_binder_6(TO* op, TD* dec, func_t func, 
			arg_1_t arg_func_1,
			arg_2_t arg_func_2,
			arg_3_t arg_func_3,
			arg_4_t arg_func_4,
			arg_5_t arg_func_5,
			arg_6_t arg_func_6)
			: instr_caller()
			, m_op(op)
			, m_dec(dec)
			, m_func(func)
			, m_arg_func_1(arg_func_1)
			, m_arg_func_2(arg_func_2)
			, m_arg_func_3(arg_func_3)
			, m_arg_func_4(arg_func_4)
			, m_arg_func_5(arg_func_5)
			, m_arg_func_6(arg_func_6)
		{
		}

		virtual void operator ()()
		{
			(m_op->*m_func)(
				(m_dec->*m_arg_func_1)(),
				(m_dec->*m_arg_func_2)(),
				(m_dec->*m_arg_func_3)(),
				(m_dec->*m_arg_func_4)(),
				(m_dec->*m_arg_func_5)(),
				(m_dec->*m_arg_func_6)()
			);
		}
	};

	template<int count, typename TD, typename T>
	class instr_list : public instr_caller
	{
		typedef T (TD::*func_t)();

		TD* m_parent;
		func_t m_func;
		instr_caller* m_instrs[count];
		instr_caller* m_error_func;

	public:
		instr_list(TD* parent, func_t func, instr_caller* error_func)
			: instr_caller()
			, m_parent(parent)
			, m_func(func)
			, m_error_func(error_func)
		{
			memset(m_instrs, 0, sizeof(instr_caller*) * count);
		}

		virtual ~instr_list()
		{
			for(int i=0; i<count; ++i)
			{
				delete m_instrs[i];
			}

			delete m_error_func;
		}

		void set_instr(uint pos, instr_caller* func)
		{
			assert(pos < count);
			m_instrs[pos] = func;
		}

		virtual void operator ()()
		{
			instr_caller* instr = m_instrs[(m_parent->*m_func)() & (count - 1)];

			if(instr)
			{
				(*instr)();
			}
			else if(m_error_func)
			{
				(*m_error_func)();
			}
		}
	};

	template<int count, typename TD, typename T>
	instr_list<count, TD, T>* new_list(TD* parent, T (TD::*func)(), instr_caller* error_func)
	{
		return new instr_list<count, TD, T>(parent, func, error_func);
	}

public:
	virtual void Decode(const u32 code)=0;
};