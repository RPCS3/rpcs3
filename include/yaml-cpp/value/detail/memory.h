#ifndef VALUE_DETAIL_MEMORY_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define VALUE_DETAIL_MEMORY_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include "yaml-cpp/value/ptr.h"
#include <set>
#include <boost/shared_ptr.hpp>

namespace YAML
{
	namespace detail
	{
		class memory;
		
		class memory_holder {
		public:
			memory_holder();
			
			shared_node create_node();
			void merge(memory_holder& rhs);
			
		private:
			boost::shared_ptr<memory> m_pMemory;
		};
		
		class memory {
		public:
			shared_node create_node();
			void merge(const memory& rhs);
			
		private:
			typedef std::set<shared_node> Nodes;
			Nodes m_nodes;
		};
		
		inline memory_holder::memory_holder(): m_pMemory(new memory)
		{
		}
		
		inline shared_node memory_holder::create_node()
		{
			return m_pMemory->create_node();
		}
		
		inline void memory_holder::merge(memory_holder& rhs)
		{
			if(m_pMemory == rhs.m_pMemory)
				return;
			
			m_pMemory->merge(*rhs.m_pMemory);
			rhs.m_pMemory = m_pMemory;
		}

		shared_node memory::create_node()
		{
			shared_node pNode(new node);
			m_nodes.insert(pNode);
			return pNode;
		}

		inline void memory::merge(const memory& rhs)
		{
			m_nodes.insert(rhs.m_nodes.begin(), rhs.m_nodes.end());
		}
	}
}

#endif // VALUE_DETAIL_MEMORY_H_62B23520_7C8E_11DE_8A39_0800200C9A66
