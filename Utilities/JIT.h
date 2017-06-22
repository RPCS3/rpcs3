#pragma once

#ifdef LLVM_AVAILABLE

#include <memory>
#include <string>
#include <unordered_map>

#include "types.h"

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

extern llvm::LLVMContext g_llvm_ctx;

// Temporary compiler interface
class jit_compiler final
{
	// Execution instance
	std::unique_ptr<llvm::ExecutionEngine> m_engine;

	// Linkage cache
	std::unordered_map<std::string, u64> m_link;

	// Compiled functions
	std::unordered_map<std::string, u64> m_map;

	// Arch
	std::string m_cpu;

public:
	jit_compiler(std::unordered_map<std::string, u64>, std::string _cpu);
	~jit_compiler();

	// Add module
	void add(std::unique_ptr<llvm::Module> module, const std::string& path);

	// Finalize
	void fin(const std::string& path);

	// Add functions directly (name -> code)
	void add(std::unordered_map<std::string, std::string>);

	// Get compiled function address
	u64 get(const std::string& name) const
	{
		const auto found = m_map.find(name);
		
		if (found != m_map.end())
		{
			return found->second;
		}

		return m_engine->getFunctionAddress(name);
	}

	// Get CPU info
	const std::string& cpu() const
	{
		return m_cpu;
	}
};

#endif
