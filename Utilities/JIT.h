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

	// Compiled functions
	std::unordered_map<std::string, std::uintptr_t> m_map;

	// Linkage cache
	std::unordered_map<std::string, std::uintptr_t> m_link;

	// Arch
	std::string m_cpu;

	// Internal
	void init();

public:
	jit_compiler(std::unordered_map<std::string, std::uintptr_t>, std::string _cpu);
	~jit_compiler();

	// Compile module
	void make(std::unique_ptr<llvm::Module>, std::string);

	// Load object
	void load(std::unique_ptr<llvm::Module>, std::unique_ptr<llvm::object::ObjectFile>);

	// Get compiled function address
	std::uintptr_t get(const std::string& name) const
	{
		const auto found = m_map.find(name);
		
		if (found != m_map.end())
		{
			return found->second;
		}

		return 0;
	}

	// Get CPU info
	const std::string& cpu() const
	{
		return m_cpu;
	}
};

#endif
