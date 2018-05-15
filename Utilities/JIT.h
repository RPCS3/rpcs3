#pragma once

#ifdef LLVM_AVAILABLE

#include <memory>
#include <string>
#include <unordered_map>

#include "types.h"
#include "mutex.h"

#include "restore_new.h"
#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include "define_new_memleakdetect.h"

// Temporary compiler interface
class jit_compiler final
{
	// Local LLVM context
	llvm::LLVMContext m_context;

	// JIT Event Listener
	std::unique_ptr<struct EventListener> m_jit_el;

	// Execution instance
	std::unique_ptr<llvm::ExecutionEngine> m_engine;

	// Link table
	std::unordered_map<std::string, u64> m_link;

	// Arch
	std::string m_cpu;

public:
	jit_compiler(const std::unordered_map<std::string, u64>& _link, const std::string& _cpu, bool large = false);
	~jit_compiler();

	// Get LLVM context
	auto& get_context()
	{
		return m_context;
	}

	auto& get_engine() const
	{
		return *m_engine;
	}

	// Add module (path to obj cache dir)
	void add(std::unique_ptr<llvm::Module> module, const std::string& path);

	// Add module (not cached)
	void add(std::unique_ptr<llvm::Module> module);

	// Add object (path to obj file)
	void add(const std::string& path);

	// Finalize
	void fin();

	// Get compiled function address
	u64 get(const std::string& name);

	// Add functions directly to the memory manager (name -> code)
	static std::unordered_map<std::string, u64> add(std::unordered_map<std::string, std::string>);

	// Get CPU info
	static std::string cpu(const std::string& _cpu);

	// Check JIT purpose
	bool is_primary() const
	{
		return !m_link.empty();
	}
};

#endif
