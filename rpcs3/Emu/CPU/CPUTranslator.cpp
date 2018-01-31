#ifdef LLVM_AVAILABLE

#include "CPUTranslator.h"

cpu_translator::cpu_translator(llvm::LLVMContext& context, llvm::Module* module, bool is_be)
    : m_context(context)
	, m_module(module)
	, m_is_be(is_be)
{

}

#endif