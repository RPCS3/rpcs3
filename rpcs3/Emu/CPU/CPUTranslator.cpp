#ifdef LLVM_AVAILABLE

#include "CPUTranslator.h"

llvm::LLVMContext g_llvm_ctx;

cpu_translator::cpu_translator(llvm::Module* module, bool is_be)
    : m_context(g_llvm_ctx)
	, m_module(module)
	, m_is_be(is_be)
{

}

#endif