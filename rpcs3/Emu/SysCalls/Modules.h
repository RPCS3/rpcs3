#pragma once

#include "Emu/SysCalls/SC_FUNC.h"
#include "Emu/SysCalls/CB_FUNC.h"
#include "ErrorCodes.h"
#include "LogBase.h"

namespace vm { using namespace ps3; }

template<typename T = void> class Module;

struct ModuleFunc
{
	u32 id;
	u32 flags;
	Module<>* module;
	const char* name;
	ppu_func_caller func;
	vm::ptr<void()> lle_func;

	ModuleFunc()
	{
	}

	ModuleFunc(u32 id, u32 flags, Module<>* module, const char* name, ppu_func_caller func, vm::ptr<void()> lle_func = vm::null)
		: id(id)
		, flags(flags)
		, module(module)
		, name(name)
		, func(func)
		, lle_func(lle_func)
	{
	}
};

struct ModuleVariable
{
	u32 id;
	Module<>* module;
	const char* name;
	u32(*retrieve_addr)();
};

enum : u32
{
	SPET_MASKED_OPCODE,
	SPET_OPTIONAL_MASKED_OPCODE,
	SPET_LABEL,
	SPET_BRANCH_TO_LABEL,
	SPET_BRANCH_TO_FUNC,
};

struct SearchPatternEntry
{
	u32 type;
	be_t<u32> data;
	be_t<u32> mask;
	u32 num; // supplement info
};

struct StaticFunc
{
	u32 index;
	const char* name;
	std::vector<SearchPatternEntry> ops;
	u32 found;
	std::unordered_map<u32, u32> labels;
};

template<> class Module<void> : public LogBase
{
	friend class ModuleManager;

	std::string m_name;
	bool m_is_loaded;
	void(*m_init)();

protected:
	std::function<void()> on_alloc;

public:
	Module(const char* name, void(*init)());

	Module(const Module&) = delete; // Delete copy/move constructors and copy/move operators

	~Module();

	std::function<void()> on_load;
	std::function<void()> on_unload;
	std::function<void()> on_stop;
	std::function<void(s64 value, ModuleFunc* func)> on_error;

	void Init();
	void Load();
	void Unload();

	void SetLoaded(bool loaded = true);
	bool IsLoaded() const;

	virtual const std::string& GetName() const override;
	void SetName(const std::string& name);
};

// Module<> with an instance of specified type in PS3 memory
template<typename T> class Module : public Module<void>
{
	u32 m_addr;

public:
	Module(const char* name, void(*init)())
		: Module<void>(name, init)
	{
		on_alloc = [this]
		{
			static_assert(std::is_trivially_destructible<T>::value, "Module<> instance must be trivially destructible");
			//static_assert(std::is_trivially_copy_assignable<T>::value, "Module<> instance must be trivially copy-assignable");

			// Allocate module instance and call the default constructor
			new(vm::base(m_addr = vm::alloc(sizeof(T), vm::main))) T();
		};
	}

	T* operator ->() const
	{
		return vm::_ptr<T>(m_addr);
	}
};

u32 add_ppu_func(ModuleFunc func);
void add_variable(u32 nid, Module<>* module, const char* name, u32(*addr)());
ModuleFunc* get_ppu_func_by_nid(u32 nid, u32* out_index = nullptr);
ModuleFunc* get_ppu_func_by_index(u32 index);
ModuleVariable* get_variable_by_nid(u32 nid);
void execute_ppu_func_by_index(PPUThread& ppu, u32 id);
extern std::string get_ps3_function_name(u64 fid);
void clear_ppu_functions();
u32 get_function_id(const char* name);

u32 add_ppu_func_sub(StaticFunc sf);
u32 add_ppu_func_sub(const std::initializer_list<SearchPatternEntry>& ops, const char* name, Module<>* module, ppu_func_caller func);

void hook_ppu_funcs(vm::ptr<u32> base, u32 size);

bool patch_ppu_import(u32 addr, u32 index);

// Variable associated with registered HLE function
template<typename T, T Func> struct ppu_func_by_func { static u32 index; };

template<typename T, T Func> u32 ppu_func_by_func<T, Func>::index = 0xffffffffu;

template<typename T, T Func, typename... Args, typename RT = std::result_of_t<T(Args...)>> inline RT call_ppu_func(PPUThread& ppu, Args&&... args)
{
	const auto mfunc = get_ppu_func_by_index(ppu_func_by_func<T, Func>::index);

	if (mfunc && mfunc->lle_func && (mfunc->flags & MFF_FORCED_HLE) == 0 && (mfunc->flags & MFF_NO_RETURN) == 0)
	{
		const u32 pc = vm::read32(mfunc->lle_func.addr());
		const u32 rtoc = vm::read32(mfunc->lle_func.addr() + 4);

		return cb_call<RT, Args...>(ppu, pc, rtoc, std::forward<Args>(args)...);
	}
	else
	{
		return Func(std::forward<Args>(args)...);
	}
}

// Call specified function directly if LLE is not available, call LLE equivalent in callback style otherwise
#define CALL_FUNC(ppu, func, ...) call_ppu_func<decltype(&func), &func>(ppu, __VA_ARGS__)

#define REG_FNID(module, nid, func, ...) (ppu_func_by_func<decltype(&func), &func>::index = add_ppu_func(ModuleFunc(nid, { __VA_ARGS__ }, &module, #func, BIND_FUNC(func))))

#define REG_FUNC(module, func, ...) REG_FNID(module, get_function_id(#func), func, __VA_ARGS__)

#define REG_VNID(module, nid, var) add_variable(nid, &module, #var, []{ return vm::get_addr(&module->var); })

#define REG_VARIABLE(module, var) REG_VNID(module, get_function_id(#var), var)

#define REG_SUB(module, ns, name, ...) add_ppu_func_sub({ __VA_ARGS__ }, #name, &module, BIND_FUNC(ns::name))

#define SP_OP(type, op, sup) []() { s32 XXX = 0; SearchPatternEntry res = { (type), (op), 0, (sup) }; XXX = -1; res.mask = (op) ^ ~res.data; return res; }()
#define SP_I(op) SP_OP(SPET_MASKED_OPCODE, op, 0)
#define OPT_SP_I(op) SP_OP(SPET_OPTIONAL_MASKED_OPCODE, op, 0)
#define SET_LABEL(label) { SPET_LABEL, (label) }
#define SP_LABEL_BR(op, label) SP_OP(SPET_BRANCH_TO_LABEL, op, label)
#define SP_CALL(op, name) SP_OP(SPET_BRANCH_TO_FUNC, op, get_function_id(#name))

#define UNIMPLEMENTED_FUNC(module) module.Error("%s", __FUNCTION__)
