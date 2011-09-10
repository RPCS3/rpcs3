#ifndef VALUE_VALUEEVENTS_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define VALUE_VALUEEVENTS_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include "yaml-cpp/anchor.h"
#include "yaml-cpp/value/ptr.h"
#include <multiset>
#include <vector>

namespace YAML
{
	class Value;
	
	class ValueEvents
	{
	public:
		ValueEvents(const Value& value);
		
	private:
		void Visit(detail::node& node);
		
	private:
		detail::shared_memory_holder m_pMemory;
		detail::node& m_root;
		
		typedef std::map<const detail::node_ref *, int> RefCount;
		RefCount m_refCount;
	};
}

#endif // VALUE_VALUEEVENTS_H_62B23520_7C8E_11DE_8A39_0800200C9A66

