#pragma once

#define RESULT(x) SC_ARGS_1 = (x)

class func_caller
{
public:
	virtual void operator()() = 0;
};


template<bool is_in_sp, bool is_fp, bool is_ptr, typename T, int i>
struct get_arg;

template<typename T, int i>
struct get_arg<false, false, false, T, i> // not fp, not ptr, 1..8
{
	static __forceinline T func(PPUThread& CPU) { return (T&)CPU.GPR[i + 2]; }
};

template<bool is_fp, typename T, int i>
struct get_arg<false, is_fp, true, T, i> // ptr, 1..8
{
	static __forceinline T func(PPUThread& CPU) { return CPU.GPR[i + 2] ? (T)&Memory[CPU.GPR[i + 2]] : nullptr; }
};

template<bool is_in_sp, typename T, int i>
struct get_arg<is_in_sp, true, false, T, i> // fp, 1..12
{
	static __forceinline T func(PPUThread& CPU) { return CPU.FPR[i]; }
};

template<typename T, int i>
struct get_arg<true, false, false, T, i> // not fp, not ptr, 9..12
{
	static __forceinline T func(PPUThread& CPU) { u64 res = Memory.Read64(CPU.GPR[1] + 0x70 + 0x8 * (i - 9)); return (T&)res; }
};

template<bool is_fp, typename T, int i>
struct get_arg<true, is_fp, true, T, i> // ptr, 9..12
{
	static __forceinline T func(PPUThread& CPU) { u64 addr = Memory.Read64(CPU.GPR[1] + 0x70 + 0x8 * (i - 9)); return addr ? (T)&Memory[addr] : nullptr; }
};

#define ARG(n) get_arg<((n) > 8), std::is_floating_point<T##n>::value, std::is_pointer<T##n>::value, T##n, n>::func(CPU)

template<typename TR>
class binder_func_0 : public func_caller
{
	typedef TR (*func_t)();
	const func_t m_call;

public:
	binder_func_0(func_t call) : func_caller(), m_call(call) {}
	virtual void operator()() { declCPU(); RESULT(m_call()); }
};

template<>
class binder_func_0<void> : public func_caller
{
	typedef void (*func_t)();
	const func_t m_call;

public:
	binder_func_0(func_t call) : func_caller(), m_call(call) {}
	virtual void operator()() { declCPU(); m_call(); }
};

template<typename TR, typename T1>
class binder_func_1 : public func_caller
{
	typedef TR (*func_t)(T1);
	const func_t m_call;

public:
	binder_func_1(func_t call) : func_caller(), m_call(call) {}
	virtual void operator()() { declCPU(); RESULT(m_call(ARG(1))); }
};

template<typename T1>
class binder_func_1<void, T1> : public func_caller
{
	typedef void (*func_t)(T1);
	const func_t m_call;

public:
	binder_func_1(func_t call) : func_caller(), m_call(call) {}
	virtual void operator()() { declCPU(); m_call(ARG(1)); }
};

template<typename TR, typename T1, typename T2>
class binder_func_2 : public func_caller
{
	typedef TR (*func_t)(T1, T2);
	const func_t m_call;

public:
	binder_func_2(func_t call) : func_caller(), m_call(call) {}
	virtual void operator()() { declCPU(); RESULT(m_call(ARG(1), ARG(2))); }
};

template<typename T1, typename T2>
class binder_func_2<void, T1, T2> : public func_caller
{
	typedef void (*func_t)(T1, T2);
	const func_t m_call;

public:
	binder_func_2(func_t call) : func_caller(), m_call(call) {}
	virtual void operator()() { declCPU(); m_call(ARG(1), ARG(2)); }
};

template<typename TR, typename T1, typename T2, typename T3>
class binder_func_3 : public func_caller
{
	typedef TR (*func_t)(T1, T2, T3);
	const func_t m_call;

public:
	binder_func_3(func_t call) : func_caller(), m_call(call) {}
	virtual void operator()() { declCPU(); RESULT(m_call(ARG(1), ARG(2), ARG(3))); }
};

template<typename T1, typename T2, typename T3>
class binder_func_3<void, T1, T2, T3> : public func_caller
{
	typedef void (*func_t)(T1, T2, T3);
	const func_t m_call;

public:
	binder_func_3(func_t call) : func_caller(), m_call(call) {}
	virtual void operator()() { declCPU(); m_call(ARG(1), ARG(2), ARG(3)); }
};

template<typename TR, typename T1, typename T2, typename T3, typename T4>
class binder_func_4 : public func_caller
{
	typedef TR (*func_t)(T1, T2, T3, T4);
	const func_t m_call;

public:
	binder_func_4(func_t call) : func_caller(), m_call(call) {}
	virtual void operator()() { declCPU(); RESULT(m_call(ARG(1), ARG(2), ARG(3), ARG(4))); }
};

template<typename T1, typename T2, typename T3, typename T4>
class binder_func_4<void, T1, T2, T3, T4> : public func_caller
{
	typedef void (*func_t)(T1, T2, T3, T4);
	const func_t m_call;

public:
	binder_func_4(func_t call) : func_caller(), m_call(call) {}
	virtual void operator()() { declCPU(); m_call(ARG(1), ARG(2), ARG(3), ARG(4)); }
};

template<typename TR, typename T1, typename T2, typename T3, typename T4, typename T5>
class binder_func_5 : public func_caller
{
	typedef TR (*func_t)(T1, T2, T3, T4, T5);
	const func_t m_call;

public:
	binder_func_5(func_t call) : func_caller(), m_call(call) {}
	virtual void operator()() { declCPU(); RESULT(m_call(ARG(1), ARG(2), ARG(3), ARG(4), ARG(5))); }
};

template<typename T1, typename T2, typename T3, typename T4, typename T5>
class binder_func_5<void, T1, T2, T3, T4, T5> : public func_caller
{
	typedef void (*func_t)(T1, T2, T3, T4, T5);
	const func_t m_call;

public:
	binder_func_5(func_t call) : func_caller(), m_call(call) {}
	virtual void operator()() { declCPU(); m_call(ARG(1), ARG(2), ARG(3), ARG(4), ARG(5)); }
};

template<typename TR, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
class binder_func_6 : public func_caller
{
	typedef TR (*func_t)(T1, T2, T3, T4, T5, T6);
	const func_t m_call;

public:
	binder_func_6(func_t call) : func_caller(), m_call(call) {}
	virtual void operator()() { declCPU(); RESULT(m_call(ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6))); }
};

template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
class binder_func_6<void, T1, T2, T3, T4, T5, T6> : public func_caller
{
	typedef void (*func_t)(T1, T2, T3, T4, T5, T6);
	const func_t m_call;

public:
	binder_func_6(func_t call) : func_caller(), m_call(call) {}
	virtual void operator()() { declCPU(); m_call(ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6)); }
};

template<typename TR, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
class binder_func_7 : public func_caller
{
	typedef TR (*func_t)(T1, T2, T3, T4, T5, T6, T7);
	const func_t m_call;

public:
	binder_func_7(func_t call) : func_caller(), m_call(call) {}
	virtual void operator()() { declCPU(); RESULT(m_call(ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), ARG(7))); }
};

template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
class binder_func_7<void, T1, T2, T3, T4, T5, T6, T7> : public func_caller
{
	typedef void (*func_t)(T1, T2, T3, T4, T5, T6, T7);
	const func_t m_call;

public:
	binder_func_7(func_t call) : func_caller(), m_call(call) {}
	virtual void operator()() { declCPU(); m_call(ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), ARG(7)); }
};

template<typename TR, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
class binder_func_8 : public func_caller
{
	typedef TR (*func_t)(T1, T2, T3, T4, T5, T6, T7, T8);
	const func_t m_call;

public:
	binder_func_8(func_t call) : func_caller(), m_call(call) {}
	virtual void operator()() { declCPU(); RESULT(m_call(ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), ARG(7), ARG(8))); }
};

template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
class binder_func_8<void, T1, T2, T3, T4, T5, T6, T7, T8> : public func_caller
{
	typedef void (*func_t)(T1, T2, T3, T4, T5, T6, T7, T8);
	const func_t m_call;

public:
	binder_func_8(func_t call) : func_caller(), m_call(call) {}
	virtual void operator()() { declCPU(); m_call(ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), ARG(7), ARG(8)); }
};

template<typename TR, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
class binder_func_9 : public func_caller
{
	typedef TR (*func_t)(T1, T2, T3, T4, T5, T6, T7, T8, T9);
	const func_t m_call;

public:
	binder_func_9(func_t call) : func_caller(), m_call(call) {}
	virtual void operator()() { declCPU(); RESULT(m_call(ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), ARG(7), ARG(8), ARG(9))); }
};

template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
class binder_func_9<void, T1, T2, T3, T4, T5, T6, T7, T8, T9> : public func_caller
{
	typedef void (*func_t)(T1, T2, T3, T4, T5, T6, T7, T8, T9);
	const func_t m_call;

public:
	binder_func_9(func_t call) : func_caller(), m_call(call) {}
	virtual void operator()() { declCPU(); m_call(ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), ARG(7), ARG(8), ARG(9)); }
};

template<typename TR, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
class binder_func_10 : public func_caller
{
	typedef TR (*func_t)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10);
	const func_t m_call;

public:
	binder_func_10(func_t call) : func_caller(), m_call(call) {}
	virtual void operator()() { declCPU(); RESULT(m_call(ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), ARG(7), ARG(8), ARG(9), ARG(10))); }
};

template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
class binder_func_10<void, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10> : public func_caller
{
	typedef void (*func_t)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10);
	const func_t m_call;

public:
	binder_func_10(func_t call) : func_caller(), m_call(call) {}
	virtual void operator()() { declCPU(); m_call(ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), ARG(7), ARG(8), ARG(9), ARG(10)); }
};

template<typename TR, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11>
class binder_func_11 : public func_caller
{
	typedef TR (*func_t)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11);
	const func_t m_call;

public:
	binder_func_11(func_t call) : func_caller(), m_call(call) {}
	virtual void operator()() { declCPU(); RESULT(m_call(ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), ARG(7), ARG(8), ARG(9), ARG(10), ARG(11))); }
};

template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11>
class binder_func_11<void, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11> : public func_caller
{
	typedef void (*func_t)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11);
	const func_t m_call;

public:
	binder_func_11(func_t call) : func_caller(), m_call(call) {}
	virtual void operator()() { declCPU(); m_call(ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), ARG(7), ARG(8), ARG(9), ARG(10), ARG(11)); }
};

template<typename TR, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12>
class binder_func_12 : public func_caller
{
	typedef TR (*func_t)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12);
	const func_t m_call;

public:
	binder_func_12(func_t call) : func_caller(), m_call(call) {}
	virtual void operator()() { declCPU(); RESULT(m_call(ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), ARG(7), ARG(8), ARG(9), ARG(10), ARG(11), ARG(12))); }
};

template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12>
class binder_func_12<void, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12> : public func_caller
{
	typedef void (*func_t)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12);
	const func_t m_call;

public:
	binder_func_12(func_t call) : func_caller(), m_call(call) {}
	virtual void operator()() { declCPU(); m_call(ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), ARG(7), ARG(8), ARG(9), ARG(10), ARG(11), ARG(12)); }
};

#undef ARG

template<typename TR>
func_caller* bind_func(TR (*call)())
{
	return new binder_func_0<TR>(call);
}

template<typename TR, typename T1>
func_caller* bind_func(TR (*call)(T1))
{
	return new binder_func_1<TR, T1>(call);
}

template<typename TR, typename T1, typename T2>
func_caller* bind_func(TR (*call)(T1, T2))
{
	return new binder_func_2<TR, T1, T2>(call);
}

template<typename TR, typename T1, typename T2, typename T3>
func_caller* bind_func(TR (*call)(T1, T2, T3))
{
	return new binder_func_3<TR, T1, T2, T3>(call);
}

template<typename TR, typename T1, typename T2, typename T3, typename T4>
func_caller* bind_func(TR (*call)(T1, T2, T3, T4))
{
	return new binder_func_4<TR, T1, T2, T3, T4>(call);
}

template<typename TR, typename T1, typename T2, typename T3, typename T4, typename T5>
func_caller* bind_func(TR (*call)(T1, T2, T3, T4, T5))
{
	return new binder_func_5<TR, T1, T2, T3, T4, T5>(call);
}

template<typename TR, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
func_caller* bind_func(TR (*call)(T1, T2, T3, T4, T5, T6))
{
	return new binder_func_6<TR, T1, T2, T3, T4, T5, T6>(call);
}

template<typename TR, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
func_caller* bind_func(TR (*call)(T1, T2, T3, T4, T5, T6, T7))
{
	return new binder_func_7<TR, T1, T2, T3, T4, T5, T6, T7>(call);
}

template<typename TR, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
func_caller* bind_func(TR (*call)(T1, T2, T3, T4, T5, T6, T7, T8))
{
	return new binder_func_8<TR, T1, T2, T3, T4, T5, T6, T7, T8>(call);
}

template<typename TR, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
func_caller* bind_func(TR (*call)(T1, T2, T3, T4, T5, T6, T7, T8, T9))
{
	return new binder_func_9<TR, T1, T2, T3, T4, T5, T6, T7, T8, T9>(call);
}

template<typename TR, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
func_caller* bind_func(TR (*call)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10))
{
	return new binder_func_10<TR, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10>(call);
}

template<typename TR, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11>
func_caller* bind_func(TR (*call)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11))
{
	return new binder_func_11<TR, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11>(call);
}

template<typename TR, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12>
func_caller* bind_func(TR (*call)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12))
{
	return new binder_func_12<TR, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12>(call);
}
